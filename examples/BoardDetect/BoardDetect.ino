/*******************************************************************************
 * BoardDetect.ino - Universal Hardware Detection & Diagnostic Tool
 * 
 * Part of AMK_CYD_Universal_TFT library.
 * Works seamlessly across both classic ESP32 CYD (resistive & capacitive)
 * and ESP32-S3 parallel RGB capacitive displays.
 * 
 * Demonstrates:
 *   1. Hardware Auto-Detection:
 *      - Board Model (ESP32-2432S028, ESP32-32E, ESP32-024, ESP32-035, 
 *                     ESP32-2432S032 / S032C, ESP32-2432S024C, JC2432W328, ESP32-3248S035C, etc.)
 *      - Panel Controller (ILI9341, ST7789, ST7796, ST7262)
 *      - Native Resolution & Diagonal Screen Size
 *      - Touch Controller (XPT2046 Resistive vs GT911 / CST820 Capacitive)
 *      - Bus Wiring Architecture (Dedicated vs Shared SPI)
 *      - Display Readback Capability (GPIO12 MISO, GPIO13 3-wire half-duplex SDA, or None)
 *      - On-Board RGB LED Pin Assignments & GPIO34 role (Battery vs LDR)
 *   2. Responsive Typography with FreeFont Support:
 *      - Proportional FreeSans vector fonts (FreeSans9pt7b & FreeSansBold9pt7b)
 *      - Dynamic auto-scaling & font fitting to prevent text clipping/overflow
 *        across any screen geometry (320x240, 480x320, 800x480).
 *   3. Interactive Live Touch Canvas:
 *      - Real-time touch coordinates & pressure tracking.
 *      - Press on-board BOOT button (GPIO 0) to clear canvas.
 ******************************************************************************/

#include <Arduino.h>
#include <AMK_CYD_Universal_TFT.h>

#define BOOT_PIN 0

// Driver objects
AMK_display display;
AMK_touch   touch;
TFT_eSPI    tft;

// High-Tech Color Palette (RGB565)
const uint16_t C_BG        = 0x0000; // Pitch Black
const uint16_t C_HEADER    = 0x08A3; // Dark Slate Navy Header
const uint16_t C_CARD      = 0x10A2; // Deep Charcoal Card Fill
const uint16_t C_BORDER    = 0x2965; // High-Tech Border Outline
const uint16_t C_CYAN      = 0x07FF; // Electric Cyan
const uint16_t C_GREEN     = 0x07E0; // Neon Green
const uint16_t C_YELLOW    = 0xFFE0; // Bright Yellow
const uint16_t C_WHITE     = 0xFFFF; // Crisp Pure White
const uint16_t C_GRAY      = 0x9CD3; // Cool Silver Grey
const uint16_t C_PILL_BG   = 0x0185; // Deep Cobalt for Status Badge

static int canvasBarY = 0;
static int canvasY = 158;
static int barHeight = 16;
static uint16_t lastX = 0, lastY = 0;
static bool wasTouching = false;
static char lastCoordBuf[32] = "";

// Draw an adaptive info card with unified typography and guaranteed non-overlapping layout
void drawInfoCard(int x, int y, int w, int h, const char* label, const char* val, uint16_t valColor) {
  tft.fillRoundRect(x, y, w, h, 4, C_CARD);
  tft.drawRoundRect(x, y, w, h, 4, C_BORDER);

  // Label: Monospace 5x7 header (Font 1) - Centered with clean top margin
  tft.setFreeFont(nullptr);
  tft.setTextFont(1);
  tft.setTextSize(1);
  tft.setTextColor(C_GRAY, C_CARD);
  tft.setTextDatum(TC_DATUM);
  tft.drawString(label, x + (w / 2), y + 4);

  // Value: Proportional vector FreeSans9pt7b centered with guaranteed vertical separation
  tft.setTextColor(valColor, C_CARD);
  tft.setTextDatum(MC_DATUM);
  int valY = y + 26;

  tft.setFreeFont(&FreeSans9pt7b);
  if (tft.textWidth(val) <= w - 8) {
    tft.drawString(val, x + (w / 2), valY);
  } else {
    // Graceful fallback to Font 2 if an unusually long custom model string exceeds box
    tft.setFreeFont(nullptr);
    tft.drawString(val, x + (w / 2), valY, 2);
  }
  tft.setFreeFont(nullptr);
}

void printSerialReport() {
  Serial.println("\n========================================================");
  Serial.println("         AMK_CYD_Universal_TFT Hardware Audit           ");
  Serial.println("========================================================");
  Serial.printf(" Model Name:        %s\n", display.modelName());
  Serial.printf(" Panel Type:        %s (%dx%d native)\n", display.panelName(), display.nativeWidth(), display.nativeHeight());
  if (display.diagonalTenths() > 0) {
    Serial.printf(" Screen Diagonal:   %.1f inches\n", display.diagonalTenths() / 10.0f);
  }
  Serial.printf(" Touch Controller:  %s (%s)\n", touch.chipName(), touch.capacitive() ? "Capacitive I2C" : "Resistive SPI");
  Serial.printf(" Display Readback:  %s\n", tft.getReadbackName());
#if !defined(AMK_BACKEND_ESP32_S3)
  Serial.printf(" Bus Wiring:        %s\n", display.board() == CYDR_BOARD_OWN_TOUCH ? "Dedicated Touch Bus" : "Shared SPI Bus");
  Serial.printf(" RGB LED Pins:      Red: %d, Green: %d, Blue: %d\n", display.redLedPin(), display.greenLedPin(), display.blueLedPin());
  const char* hint = display.powerOffHint();
  if (hint) Serial.printf(" Power Off Hint:    %s\n", hint);
#endif
  Serial.println("========================================================\n");
}

void updateCoordTelemetry(uint16_t x, uint16_t y, uint16_t z) {
  int sw = tft.width();
  char buf[32];
  snprintf(buf, sizeof(buf), "X:%3d Y:%3d Z:%3d", x, y, z);
  if (strcmp(buf, lastCoordBuf) != 0) {
    tft.setFreeFont(nullptr);
    tft.setTextFont(1);
    tft.setTextSize(1);
    tft.setTextColor(C_YELLOW, C_HEADER);
    tft.setTextDatum(MR_DATUM);
    tft.drawString(buf, sw - 6, canvasBarY + (barHeight / 2));
    strncpy(lastCoordBuf, buf, sizeof(lastCoordBuf));
  }
}

void renderDashboard() {
  int sw = tft.width();
  int sh = tft.height();

  tft.fillScreen(C_BG);

  // Responsive sizing based on resolution: generous heights guarantee zero text overlapping
  int headerH = (sh >= 320) ? 34 : 28;
  barHeight   = (sh >= 320) ? 20 : 18;
  int gap     = 5;
  int colW    = (sw - (gap * 3)) / 2;
  int cardH   = (sh >= 320) ? 46 : 40;

  // Top Header Bar
  tft.fillRect(0, 0, sw, headerH, C_HEADER);
  tft.drawFastHLine(0, headerH, sw, C_CYAN);

  // Status Badge Pill on Right with comfortable padding
  const char* badgeText = touch.capacitive() ? "CAPACITIVE" : "RESISTIVE";
  int pillW = (sw >= 480) ? 96 : 84;
  int pillH = headerH - 8;
  int pillX = sw - pillW - 6;
  int pillY = (headerH - pillH) / 2;
  tft.fillRoundRect(pillX, pillY, pillW, pillH, 4, C_PILL_BG);
  tft.drawRoundRect(pillX, pillY, pillW, pillH, 4, touch.capacitive() ? C_GREEN : C_YELLOW);
  tft.setTextColor(touch.capacitive() ? C_GREEN : C_YELLOW, C_PILL_BG);
  tft.setTextDatum(MC_DATUM);
  tft.setFreeFont(nullptr);
  tft.setTextFont(1);
  tft.setTextSize(1);
  tft.drawString(badgeText, pillX + (pillW / 2), pillY + (pillH / 2));

  // Left Title with FreeSansBold
  tft.setTextColor(C_CYAN, C_HEADER);
  tft.setTextDatum(ML_DATUM);
  int titleMaxW = pillX - 12;
  const char* titleStr = (sw >= 480) ? "AMK Universal Diagnostics" : "AMK Diagnostics";
  tft.setFreeFont(&FreeSansBold9pt7b);
  if (tft.textWidth(titleStr) <= titleMaxW) {
    tft.drawString(titleStr, 8, headerH / 2);
  } else {
    tft.setFreeFont(nullptr);
    tft.drawString("AMK Diagnostics", 8, headerH / 2, 2);
  }
  tft.setFreeFont(nullptr);

  // Dynamic Card Grid calculation with unified typography hierarchy
  int y0 = headerH + gap;
  // Card 1: Board Model (on 320px screens, trim redundant "ESP32-" prefix so FreeSans fits with generous padding)
  const char* model = display.modelName();
  const char* modelDisplay = (sw <= 320 && strncmp(model, "ESP32-", 6) == 0) ? model + 6 : model;
  drawInfoCard(gap, y0, colW, cardH, "BOARD MODEL", modelDisplay, C_WHITE);

  // Card 2: Panel Controller
  drawInfoCard(gap * 2 + colW, y0, colW, cardH, "DISPLAY PANEL", display.panelName(), C_GREEN);

  int y1 = y0 + cardH + gap;
  // Card 3: Touch Controller
  char touchBuf[32];
  snprintf(touchBuf, sizeof(touchBuf), "%s (%s)", touch.chipName(), touch.capacitive() ? "Cap" : "Res");
  drawInfoCard(gap, y1, colW, cardH, "TOUCH ENGINE", touchBuf, C_CYAN);

  // Card 4: Display Readback
  drawInfoCard(gap * 2 + colW, y1, colW, cardH, "SPI READBACK", tft.getReadbackName(), C_YELLOW);

  int y2 = y1 + cardH + gap;
  // Card 5: LED Pins
  char ledBuf[32];
#if !defined(AMK_BACKEND_ESP32_S3)
  snprintf(ledBuf, sizeof(ledBuf), "R:%d G:%d B:%d", display.redLedPin(), display.greenLedPin(), display.blueLedPin());
#else
  snprintf(ledBuf, sizeof(ledBuf), "N/A (S3 RGB)");
#endif
  drawInfoCard(gap, y2, colW, cardH, "RGB LED PINS", ledBuf, C_WHITE);

  // Card 6: Screen Size & Resolution
  char sizeBuf[32];
  if (display.diagonalTenths() > 0) {
    snprintf(sizeBuf, sizeof(sizeBuf), "%.1f\" (%dx%d)", display.diagonalTenths() / 10.0f, sw, sh);
  } else {
    snprintf(sizeBuf, sizeof(sizeBuf), "%dx%d", sw, sh);
  }
  drawInfoCard(gap * 2 + colW, y2, colW, cardH, "SCREEN GEOMETRY", sizeBuf, C_GREEN);

  // Interactive Canvas Bar
  canvasBarY = y2 + cardH + gap + 1;
  tft.fillRect(0, canvasBarY, sw, barHeight, C_HEADER);
  tft.drawFastHLine(0, canvasBarY, sw, C_BORDER);
  tft.drawFastHLine(0, canvasBarY + barHeight, sw, C_BORDER);
  
  tft.setFreeFont(nullptr);
  tft.setTextFont(1);
  tft.setTextSize(1);
  tft.setTextColor(C_GRAY, C_HEADER);
  tft.setTextDatum(ML_DATUM);
  tft.drawString(" Touch Canvas [BOOT: Clear]", 6, canvasBarY + (barHeight / 2));

  canvasY = canvasBarY + barHeight + 1;
  updateCoordTelemetry(0, 0, 0);
}

void setup() {
  Serial.begin(115200);
  delay(250);

  pinMode(BOOT_PIN, INPUT_PULLUP);

  // 1-Line Universal Auto-Setup (v1.6.0)
  tft.autoSetup(3, false);
  tft.autoCalibrate(); // Auto-calibrates uncalibrated resistive touch (< 1 ms if already saved; 0 ms if capacitive)

  printSerialReport();
  renderDashboard();
}

void loop() {
  tft.adjustBrightness(); // Built-in ambient light auto-dimming

  // Clear canvas when BOOT button is pressed
  if (digitalRead(BOOT_PIN) == LOW) {
    delay(20);
    if (digitalRead(BOOT_PIN) == LOW) {
      tft.fillRect(0, canvasY, tft.width(), tft.height() - canvasY, C_BG);
      updateCoordTelemetry(0, 0, 0);
      while (digitalRead(BOOT_PIN) == LOW) delay(10);
    }
  }

  uint16_t tx = 0, ty = 0;
  if (tft.getTouch(&tx, &ty)) {
    uint16_t tz = tft.getTouchRawZ();
    updateCoordTelemetry(tx, ty, tz);

    if (ty >= canvasY) {
      if (wasTouching) {
        tft.drawLine(lastX, lastY, tx, ty, C_CYAN);
        tft.fillCircle(tx, ty, 2, C_YELLOW);
      } else {
        tft.fillCircle(tx, ty, 3, C_GREEN);
      }
      lastX = tx;
      lastY = ty;
      wasTouching = true;
    }
  } else {
    wasTouching = false;
  }
  delay(10);
}
