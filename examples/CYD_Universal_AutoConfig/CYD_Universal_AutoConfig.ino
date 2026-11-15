/***************************************************************************************
 * CYD_Universal_AutoConfig.ino - Master Auto-Configuration Template for ESP32 CYD
 * 
 * Author: Aung Moe Khine (AMK)
 * Library: AMK_CYD_Universal_TFT (v1.6.6)
 * Repository: https://github.com/aungmoekhine/AMK_CYD_Universal_TFT
 * 
 * This master reference sketch automatically discovers and configures 100% of your
 * ESP32 Cheap Yellow Display (CYD) hardware features entirely at runtime.
 * 
 * ZERO HARDCODED PINS. ZERO MANUAL SWITCHES. PURE DYNAMIC PROBING.
 * 
 * FEATURES:
 * 1. Clean Top Header Bar with Live LDR Status Pill Badge (No redundant ticker)
 * 2. Responsive 6-Card Diagnostic Grid (2 Columns x 3 Rows - Small Capital Fonts)
 * 3. Hardcore LDR Ambient Dimming (map 0-1024 -> 255-20, live on-screen updates)
 * 4. Auto-Detection of Non-LDR Boards (Comfortable 180 PWM fallback, bypass dimming)
 * 5. Color Verification Swatches (7 Factory MADCTL Colors)
 * 6. Interactive Touch Canvas with Real-time X/Y/Z Coordinate Telemetry
 * 7. Interactive RGB LED Feedback & BOOT Button (GPIO 0) Canvas Clear
 ***************************************************************************************/

#include <Arduino.h>
#include <AMK_CYD_Universal_TFT.h>

#define BOOT_PIN 0          // Onboard BOOT button for clearing touch canvas
#define LDR_PIN  34         // Standard CYD analog input for LDR or Battery Divider
#define NUM_LDR_READINGS 10 // Moving average sample size

// =====================================================================================
// 1. GLOBAL DRIVER OBJECTS
// =====================================================================================
AMK_display display;
AMK_touch   touch;
TFT_eSPI    tft;

// =====================================================================================
// 2. DYNAMIC HARDWARE STATE (POPULATED AT RUNTIME BY AUTO-DETECTION)
// =====================================================================================
struct UniversalHardwareState {
  uint8_t blPin;             // Auto-detected Backlight pin (21 or 27)
  int8_t  redLedPin;         // Auto-detected Red LED pin (4 or 22, -1 if none)
  int8_t  greenLedPin;       // Auto-detected Green LED pin (16 or 17, -1 if none)
  int8_t  blueLedPin;        // Auto-detected Blue LED pin (17 or 16, -1 if none)
  int8_t  gpio34Role;        // 0 = LDR light sensor, 1 = Battery monitor, -1 = Unknown
  bool    hasLdr;            // True if GPIO 34 operates as ambient light sensor
  bool    autoBrightness;    // Backlight auto-dimming active
  int     activeBrightness;  // Current PWM level (20 - 255)
  int     lastLdrRaw;        // Latest raw analog reading
} hwState = {0, -1, -1, -1, -1, false, true, 160, 0};

// Palette Definition (RGB565)
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

// Layout Coordinates (Calculated dynamically in renderDashboard())
static int ldrCardX   = 0;
static int ldrCardY   = 0;
static int ldrCardW   = 0;
static int ldrCardH   = 0;
static int canvasBarY = 145;
static int canvasY    = 162;
static int barHeight  = 16;

// Dynamic Telemetry Cache
static char lastCoordBuf[32] = "";
static int  lastLdrVal = -999;
static int  lastAppliedPwm = -999;

static uint16_t lastTouchX = 0, lastTouchY = 0;
static bool wasTouching = false;

// =====================================================================================
// 3. REUSABLE RGB LED CONTROLLERS (ACTIVE-LOW LOGIC)
// =====================================================================================
void setRgbLed(bool redOn, bool greenOn, bool blueOn) {
  if (hwState.redLedPin >= 0)   digitalWrite(hwState.redLedPin,   redOn   ? LOW : HIGH);
  if (hwState.greenLedPin >= 0) digitalWrite(hwState.greenLedPin, greenOn ? LOW : HIGH);
  if (hwState.blueLedPin >= 0)  digitalWrite(hwState.blueLedPin,  blueOn  ? LOW : HIGH);
}

void turnLedOff() {
  setRgbLed(false, false, false);
}

void flashLedSequence() {
  setRgbLed(true, false, false); delay(60);
  setRgbLed(false, true, false); delay(60);
  setRgbLed(false, false, true); delay(60);
  turnLedOff();
}

// =====================================================================================
// 4. USER-GLOBAL-RULE COMPLIANT HARDCORE LDR AUTO-DIMMING ENGINE
// =====================================================================================
void adjustBrightness() {
  if (hwState.blPin == 0 || !hwState.hasLdr || !hwState.autoBrightness) return;

  static int readings[NUM_LDR_READINGS] = {0};
  static int readIndex = 0;
  static long total = 0;
  static unsigned long lastReadTime = 0;
  static int lastAppliedBrightness = -1;

  if (millis() - lastReadTime > 80) {
    lastReadTime = millis();
    int rawAdc = analogRead(LDR_PIN);
    hwState.lastLdrRaw = rawAdc;

    total = total - readings[readIndex];
    readings[readIndex] = rawAdc;
    total = total + rawAdc;
    readIndex = (readIndex + 1) % NUM_LDR_READINGS;

    long average = total / NUM_LDR_READINGS;
    // FROZEN HARDCORE STANDARD: map(average, 0, 1024, 255, 20)
    int target = constrain(map(average, 0, 1024, 255, 20), 20, 255);

    // 4-unit hysteresis deadband prevents light sensor flicker & PWM thrashing
    if (abs(target - lastAppliedBrightness) >= 4) {
      lastAppliedBrightness = target;
      hwState.activeBrightness = target;
      analogWrite(hwState.blPin, target);
    }
  }
}

// =====================================================================================
// 5. MASTER HARDWARE AUTO-SETUP ROUTINE (DROP-IN FOR ANY SKETCH)
// =====================================================================================
void setupUniversalHardware(uint8_t rotation = 3) {
  tft.init();
  tft.setRotation(rotation);
  tft.setSwapBytes(true);
  tft.fillScreen(C_BG);

  hwState.blPin = display.backlightPin();
  pinMode(hwState.blPin, OUTPUT);
  analogWriteResolution(hwState.blPin, 8);
  hwState.activeBrightness = 160;
  analogWrite(hwState.blPin, hwState.activeBrightness);

  hwState.gpio34Role = display.gpio34IsBattery();
  hwState.hasLdr = tft.hasLdrSensor();

  if (hwState.hasLdr) {
    pinMode(LDR_PIN, INPUT);
    for (int i = 0; i < NUM_LDR_READINGS; i++) {
      adjustBrightness();
      delay(10);
    }
  } else {
    hwState.activeBrightness = tft.getProperNonLdrBacklight();
    analogWrite(hwState.blPin, hwState.activeBrightness);
  }

  hwState.redLedPin   = display.redLedPin();
  hwState.greenLedPin = display.greenLedPin();
  hwState.blueLedPin  = display.blueLedPin();

  if (hwState.redLedPin >= 0)   { pinMode(hwState.redLedPin, OUTPUT);   digitalWrite(hwState.redLedPin, HIGH); }
  if (hwState.greenLedPin >= 0) { pinMode(hwState.greenLedPin, OUTPUT); digitalWrite(hwState.greenLedPin, HIGH); }
  if (hwState.blueLedPin >= 0)  { pinMode(hwState.blueLedPin, OUTPUT);  digitalWrite(hwState.blueLedPin, HIGH); }

  flashLedSequence();

  touch.begin(display);
  touch.setRotation(rotation);
  if (!touch.capacitive()) {
    touch.loadCalibration();
    touch.loadFilters();
  }
}

// =====================================================================================
// 6. SERIAL HARDWARE AUDIT REPORTER
// =====================================================================================
void printHardwareReport() {
  Serial.println("\n=================================================================");
  Serial.println("         AMK_CYD_Universal_TFT Universal Hardware Report         ");
  Serial.println("=================================================================");
  Serial.printf(" Board Model:         %s\n", display.modelName());
  Serial.printf(" Display Panel:       %s\n", display.panelName());
  Serial.printf(" Native Resolution:   %d x %d pixels\n", display.nativeWidth(), display.nativeHeight());
  Serial.printf(" Active Resolution:   %d x %d pixels (Rotation %d)\n", tft.width(), tft.height(), tft.getRotation());
  if (display.diagonalTenths() > 0) {
    Serial.printf(" Screen Diagonal:     %.1f inches\n", display.diagonalTenths() / 10.0f);
  }
  Serial.printf(" Backlight Pin:       GPIO %u (Auto-detected by library)\n", hwState.blPin);
  Serial.printf(" Backlight Control:   8-bit PWM (Active Level: %d / 255)\n", hwState.activeBrightness);
  Serial.printf(" Color Order:         %s (Factory MADCTL verified)\n", display.isBGR() ? "BGR" : "RGB");
  Serial.printf(" 16-bit Byte Swap:    %s\n", tft.getSwapBytes() ? "Enabled (setSwapBytes: true)" : "Disabled");
  Serial.printf(" Touch Controller:    %s (%s)\n", touch.chipName(), touch.capacitive() ? "Capacitive I2C" : "Resistive SPI");
#if !defined(AMK_BACKEND_ESP32_S3)
  Serial.printf(" Bus Architecture:    %s\n", display.board() == CYDR_BOARD_OWN_TOUCH ? "Dedicated Touch Bus" : "Shared Display SPI Bus");
  Serial.printf(" Display Readback:    %s\n", tft.getReadbackName());
  Serial.printf(" RGB LED Pins:        Red: %d | Green: %d | Blue: %d (Active-LOW)\n", hwState.redLedPin, hwState.greenLedPin, hwState.blueLedPin);
  if (hwState.gpio34Role == 1) {
    Serial.println(" GPIO 34 Role:        Battery Voltage Divider (LDR auto-dim disabled)");
  } else if (hwState.hasLdr) {
    Serial.println(" GPIO 34 Role:        Light Dependent Resistor (LDR Auto-Dimming Active)");
  } else {
    Serial.printf(" GPIO 34 Role:        No LDR Detected -> Auto-Set Backlight (%d / 255, ~%d%%)\n",
                  hwState.activeBrightness, (int)(hwState.activeBrightness * 100 / 255));
  }
  const char* hint = display.powerOffHint();
  if (hint) Serial.printf(" Power-Off Hint:      %s\n", hint);
#endif
  Serial.println("=================================================================\n");
}

// =====================================================================================
// 7. RESPONSIVE DASHBOARD & SMALL-CAPITAL FONT GUI RENDERER
// =====================================================================================
void drawCardValue(int x, int y, int w, int h, const char* val, uint16_t valColor) {
  int valAreaY = y + 11;
  int valAreaH = h - 13;
  tft.fillRect(x + 2, valAreaY, w - 4, valAreaH, C_CARD);

  tft.setFreeFont(nullptr);
  tft.setTextFont(2);
  tft.setTextSize(1);
  tft.setTextColor(valColor, C_CARD);
  tft.setTextDatum(MC_DATUM);

  int centerY = y + (h / 2) + 5;

  if (tft.textWidth(val) <= w - 8) {
    tft.drawString(val, x + (w / 2), centerY);
  } else {
    tft.setTextFont(1);
    tft.drawString(val, x + (w / 2), centerY);
  }
}

void drawInfoCard(int x, int y, int w, int h, const char* label, const char* val, uint16_t valColor) {
  tft.fillRoundRect(x, y, w, h, 4, C_CARD);
  tft.drawRoundRect(x, y, w, h, 4, C_BORDER);

  tft.setFreeFont(nullptr);
  tft.setTextFont(1);
  tft.setTextSize(1);
  tft.setTextColor(C_GRAY, C_CARD);
  tft.setTextDatum(TC_DATUM);
  tft.drawString(label, x + (w / 2), y + 2);

  drawCardValue(x, y, w, h, val, valColor);
}

void renderDashboard() {
  int sw = tft.width();
  int sh = tft.height();

  tft.fillScreen(C_BG);

  int headerH = (sh >= 320) ? 30 : 24;
  int gap     = 4;
  int colW    = (sw - (gap * 3)) / 2;
  int cardH   = (sh >= 320) ? 40 : 30;

  // --- 1. Clean Top Header Bar (No redundant duplicate ticker) ---
  tft.fillRect(0, 0, sw, headerH, C_HEADER);
  tft.drawFastHLine(0, headerH - 1, sw, C_CYAN);

  const char* badgeText = hwState.hasLdr ? "LDR ACTIVE" : "NO LDR (180)";
  uint16_t badgeColor = hwState.hasLdr ? C_GREEN : C_YELLOW;

  int pillW = (sw >= 480) ? 96 : 84;
  int pillH = headerH - 6;
  int pillX = sw - pillW - 4;
  int pillY = (headerH - pillH) / 2;
  tft.fillRoundRect(pillX, pillY, pillW, pillH, 4, C_PILL_BG);
  tft.drawRoundRect(pillX, pillY, pillW, pillH, 4, badgeColor);
  tft.setTextColor(badgeColor, C_PILL_BG);
  tft.setTextDatum(MC_DATUM);
  tft.setFreeFont(nullptr);
  tft.setTextFont(1);
  tft.setTextSize(1);
  tft.drawString(badgeText, pillX + (pillW / 2), pillY + (pillH / 2));

  tft.setTextColor(C_CYAN, C_HEADER);
  tft.setTextDatum(ML_DATUM);
  tft.setFreeFont(nullptr);
  tft.setTextFont(2);
  tft.setTextSize(1);
  const char* title = (sw >= 480) ? "AMK MASTER AUTOCONFIG" : "AMK AUTOCONFIG";
  tft.drawString(title, 8, headerH / 2);

  const char* model = display.modelName();
  char modelShort[32];
  if (strncmp(model, "ESP32-", 6) == 0) {
    strncpy(modelShort, model + 6, sizeof(modelShort));
  } else {
    strncpy(modelShort, model, sizeof(modelShort));
  }

  // --- 2. Six Diagnostic Hardware Cards (3 Rows x 2 Columns - Zero Collision Layout) ---
  int y0 = headerH + gap;
  int x0 = gap;
  int x1 = gap * 2 + colW;

  // Row 1: Model & Display Panel
  drawInfoCard(x0, y0, colW, cardH, "BOARD MODEL", modelShort, C_WHITE);
  drawInfoCard(x1, y0, colW, cardH, "DISPLAY PANEL", display.panelName(), C_GREEN);

  // Row 2: Touch Engine & Backlight Pin
  int y1 = y0 + cardH + gap;
  char touchBuf[32];
  snprintf(touchBuf, sizeof(touchBuf), "%s", touch.chipName());
  drawInfoCard(x0, y1, colW, cardH, "TOUCH ENGINE", touchBuf, C_CYAN);

  char blBuf[32];
  snprintf(blBuf, sizeof(blBuf), "GPIO %u (PWM)", hwState.blPin);
  drawInfoCard(x1, y1, colW, cardH, "BACKLIGHT PIN", blBuf, C_YELLOW);

  // Row 3: Hardcore LDR Dim (Live Dynamic Hero Card) & RGB LED Pins
  int y2 = y1 + cardH + gap;
  ldrCardX = x0;
  ldrCardY = y2;
  ldrCardW = colW;
  ldrCardH = cardH;

  char ldrBuf[32];
  if (hwState.hasLdr) {
    snprintf(ldrBuf, sizeof(ldrBuf), "%d -> %u PWM", hwState.lastLdrRaw, hwState.activeBrightness);
    drawInfoCard(ldrCardX, ldrCardY, ldrCardW, ldrCardH, "LDR AMBIENT DIM", ldrBuf, C_GREEN);
  } else {
    snprintf(ldrBuf, sizeof(ldrBuf), "NO LDR (180 PWM)");
    drawInfoCard(ldrCardX, ldrCardY, ldrCardW, ldrCardH, "LDR AMBIENT DIM", ldrBuf, C_YELLOW);
  }

  char ledBuf[32];
  if (hwState.redLedPin >= 0) {
    snprintf(ledBuf, sizeof(ledBuf), "R: %d  G: %d  B: %d", hwState.redLedPin, hwState.greenLedPin, hwState.blueLedPin);
  } else {
    snprintf(ledBuf, sizeof(ledBuf), "N/A (S3 RGB)");
  }
  drawInfoCard(x1, y2, colW, cardH, "RGB LED PINS", ledBuf, C_WHITE);

  // --- 3. Visual Color Order Verification Swatches (7 Colors) ---
  int swatchY = y2 + cardH + gap;
  int swatchH = (sh >= 320) ? 14 : 11;
  const uint16_t swatches[7] = {0xF800, 0x07E0, 0x001F, 0x07FF, 0xF81F, 0xFFE0, 0xFFFF};
  const char* swatchLabels[7] = {"R", "G", "B", "C", "M", "Y", "W"};
  int swatchW = (sw - (gap * 8)) / 7;

  for (int i = 0; i < 7; i++) {
    int sx = gap + i * (swatchW + gap);
    tft.fillRect(sx, swatchY, swatchW, swatchH, swatches[i]);
    tft.drawRect(sx, swatchY, swatchW, swatchH, C_BORDER);
    tft.setTextColor((i == 6 || i == 5 || i == 3) ? C_BG : C_WHITE, swatches[i]);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(nullptr);
    tft.setTextFont(1);
    tft.setTextSize(1);
    tft.drawString(swatchLabels[i], sx + (swatchW / 2), swatchY + (swatchH / 2));
  }

  // --- 4. Touch Canvas Header Bar ---
  barHeight = (sh >= 320) ? 20 : 16;
  canvasBarY = swatchY + swatchH + gap;
  tft.fillRect(0, canvasBarY, sw, barHeight, C_HEADER);
  tft.drawFastHLine(0, canvasBarY, sw, C_BORDER);
  tft.drawFastHLine(0, canvasBarY + barHeight, sw, C_BORDER);

  tft.setTextColor(C_GRAY, C_HEADER);
  tft.setTextDatum(ML_DATUM);
  tft.setFreeFont(nullptr);
  tft.setTextFont(1);
  tft.setTextSize(1);
  tft.drawString(" TOUCH CANVAS & RGB LED [BOOT: CLEAR]", 6, canvasBarY + (barHeight / 2));

  canvasY = canvasBarY + barHeight + 1;
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

void updateLdrTelemetry() {
  int curLdr = hwState.hasLdr ? hwState.lastLdrRaw : -1;
  int curPwm = hwState.activeBrightness;

  if (curLdr != lastLdrVal || curPwm != lastAppliedPwm) {
    lastLdrVal = curLdr;
    lastAppliedPwm = curPwm;

    char cardBuf[32];
    if (hwState.hasLdr) {
      snprintf(cardBuf, sizeof(cardBuf), "%d -> %u PWM", curLdr, curPwm);
      drawCardValue(ldrCardX, ldrCardY, ldrCardW, ldrCardH, cardBuf, C_GREEN);
    } else {
      snprintf(cardBuf, sizeof(cardBuf), "NO LDR (180 PWM)");
      drawCardValue(ldrCardX, ldrCardY, ldrCardW, ldrCardH, cardBuf, C_YELLOW);
    }
  }
}

// =====================================================================================
// 8. SETUP & LOOP
// =====================================================================================
void setup() {
  Serial.begin(115200);
  delay(200);

  pinMode(BOOT_PIN, INPUT_PULLUP);

  setupUniversalHardware(3);
  printHardwareReport();
  renderDashboard();
}

void loop() {
  // -----------------------------------------------------------------------------------
  // 1. FIRST-LINE EXECUTION: Hardcore LDR Ambient Dimming (Strict Hardware Standard)
  // -----------------------------------------------------------------------------------
  adjustBrightness();

  // -----------------------------------------------------------------------------------
  // 2. REAL-TIME HARDWARE TELEMETRY UPDATE (ZERO GHOSTING)
  // -----------------------------------------------------------------------------------
  static unsigned long lastTeleCheck = 0;
  if (millis() - lastTeleCheck >= 80) {
    lastTeleCheck = millis();
    updateLdrTelemetry();
  }

  // -----------------------------------------------------------------------------------
  // 3. CLEAR CANVAS ON BOOT BUTTON (GPIO 0) PRESS
  // -----------------------------------------------------------------------------------
  if (digitalRead(BOOT_PIN) == LOW) {
    delay(20);
    if (digitalRead(BOOT_PIN) == LOW) {
      tft.fillRect(0, canvasY, tft.width(), tft.height() - canvasY, C_BG);
      updateCoordTelemetry(0, 0, 0);
      turnLedOff();
      while (digitalRead(BOOT_PIN) == LOW) delay(10);
    }
  }

  // -----------------------------------------------------------------------------------
  // 4. INTERACTIVE TOUCH TRACKING & DYNAMIC RGB LED ILLUMINATION
  // -----------------------------------------------------------------------------------
  uint16_t tx = 0, ty = 0;
  if (tft.getTouch(&tx, &ty)) {
    uint16_t tz = tft.getTouchRawZ();
    updateCoordTelemetry(tx, ty, tz);

    if (ty >= canvasY) {
      if (wasTouching) {
        tft.drawLine(lastTouchX, lastTouchY, tx, ty, C_CYAN);
        tft.fillCircle(tx, ty, 2, C_YELLOW);
      } else {
        tft.fillCircle(tx, ty, 3, C_GREEN);
      }
      lastTouchX = tx;
      lastTouchY = ty;
      wasTouching = true;
    }

    setRgbLed(false, true, true);
  } else {
    if (wasTouching) {
      wasTouching = false;
      turnLedOff();
    }
  }

  delay(5);
}
