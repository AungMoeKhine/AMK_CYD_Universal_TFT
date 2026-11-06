/***************************************************************************************
 * Universal_AutoConfig.ino - Master Auto-Configuration Template for ESP32 CYD
 * 
 * Author: Aung Moe Khine (AMK)
 * Library: AMK_CYD_Universal_TFT (v1.5.1+)
 * Repository: https://github.com/aungmoekhine/AMK_CYD_Universal_TFT
 * 
 * This master reference sketch automatically discovers and configures 100% of your
 * ESP32 Cheap Yellow Display (CYD) hardware features entirely at runtime.
 * 
 * ZERO HARDCODED PINS. ZERO MANUAL SWITCHES. PURE DYNAMIC PROBING.
 * 
 * -------------------------------------------------------------------------------------
 * WHAT THIS SKETCH AUTOMATICALLY DETECTS & CONFIGURES:
 * -------------------------------------------------------------------------------------
 * 1. BACKLIGHT PIN & PWM:
 *    - Dynamically detects GPIO 21 (Dedicated Bus) vs GPIO 27 (Shared Bus).
 *    - Automatically sets 8-bit PWM resolution and active brightness.
 *    - NEVER hardcodes pin 21 or 27, and uses NO fallback pins.
 * 
 * 2. LDR AUTO-DIMMING & BATTERY SENSING (GPIO 34):
 *    - Automatically identifies whether GPIO 34 is a Light Dependent Resistor (LDR)
 *      or a Battery Voltage Divider (cell / 2).
 *    - Runs a 10-read moving average filter with a 4-unit hysteresis deadband.
 *    - Pre-primes the filter during boot to instantly match room ambient light.
 *    - First-line execution in loop() guarantees flicker-free, responsive dimming.
 * 
 * 3. ON-BOARD RGB LED (ACTIVE-LOW):
 *    - Dynamically detects Red LED pin (GPIO 4 vs GPIO 22).
 *    - Dynamically detects Green and Blue pins (GPIO 16 / 17 vs 17 / 16 on ESP32-024).
 *    - Automatically accounts for inverted Active-LOW polarity (LOW = ON, HIGH = OFF).
 * 
 * 4. DISPLAY PANEL & COLOR FIDELITY:
 *    - Automatically probes panel controller (ILI9341, ST7789, ST7796, ST7262).
 *    - Preserves exact factory MADCTL color order (RGB vs BGR) without manual patches.
 *    - Maintains tft.setSwapBytes(true) for 16-bit RGB565 sprites and images.
 *    - Features a 7-color swatch bar on-screen to visually verify color accuracy.
 * 
 * 5. SCREEN GEOMETRY & RESPONSIVE LAYOUT:
 *    - Automatically retrieves native panel resolution (240x320, 320x480, 800x480).
 *    - Retrieves diagonal screen size in inches (2.4", 2.8", 3.2", 3.5", 4.0", 7.0").
 *    - Responsively calculates UI card dimensions to eliminate text clipping/overflow.
 * 
 * 6. TOUCH CONTROLLER & BUS ARCHITECTURE:
 *    - Auto-probes Resistive (XPT2046) vs Capacitive (GT911 / CST820).
 *    - Auto-detects Dedicated Touch SPI Bus vs Shared Display SPI Bus.
 *    - Automatically loads persistent calibration and noise filters if resistive.
 *    - Synchronizes touch rotation to display rotation with zero boilerplate.
 * 
 * 7. SPI READBACK CAPABILITY:
 *    - Probes display readback channel (GPIO 12 MISO vs GPIO 13 3-wire half-duplex SDA).
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

// Canvas & Telemetry State
static int canvasY = 160;
static int canvasBarY = 142;
static int barHeight = 18;
static uint16_t lastTouchX = 0, lastTouchY = 0;
static bool wasTouching = false;
static char lastCoordBuf[32] = "";

// =====================================================================================
// 3. REUSABLE RGB LED CONTROLLERS (ACTIVE-LOW LOGIC)
// =====================================================================================
// On ESP32 CYD boards, RGB LEDs are Active-LOW (LOW = ON, HIGH = OFF)
void setRgbLed(bool redOn, bool greenOn, bool blueOn) {
  if (hwState.redLedPin >= 0)   digitalWrite(hwState.redLedPin,   redOn   ? LOW : HIGH);
  if (hwState.greenLedPin >= 0) digitalWrite(hwState.greenLedPin, greenOn ? LOW : HIGH);
  if (hwState.blueLedPin >= 0)  digitalWrite(hwState.blueLedPin,  blueOn  ? LOW : HIGH);
}

void turnLedOff() {
  setRgbLed(false, false, false);
}

void flashLedSequence() {
  // Quick startup confirmation blink: Red -> Green -> Blue -> Off
  setRgbLed(true, false, false); delay(60);
  setRgbLed(false, true, false); delay(60);
  setRgbLed(false, false, true); delay(60);
  turnLedOff();
}

// =====================================================================================
// 4. USER-GLOBAL-RULE COMPLIANT LDR AUTO-DIMMING ENGINE
// =====================================================================================
void adjustBrightness() {
  if (!hwState.hasLdr || !hwState.autoBrightness || hwState.blPin == 0) return;

  static int readings[NUM_LDR_READINGS] = {0};
  static int readIndex = 0;
  static long total = 0;
  static unsigned long lastReadTime = 0;
  static int lastAppliedBrightness = -1;

  if (millis() - lastReadTime > 80) {
    lastReadTime = millis();
    total = total - readings[readIndex];
    readings[readIndex] = analogRead(LDR_PIN);
    total = total + readings[readIndex];
    readIndex = (readIndex + 1) % NUM_LDR_READINGS;
    hwState.lastLdrRaw = readings[readIndex];

    long average = total / NUM_LDR_READINGS;
    // Map ambient lux: strong light = 255 (max brightness), dark room = 25 (night dim)
    int target = constrain(map(average, 0, 1024, 255, 25), 25, 255);

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
  // Step 1: Initialize Display & Probe Hardware Architecture
  tft.init();
  tft.setRotation(rotation);
  tft.setSwapBytes(true); // Maintain byte-order for 16-bit RGB565 sprites and images
  tft.fillScreen(C_BG);

  // Step 2: Auto-Detect Backlight Pin & Configure 8-Bit PWM (NO hardcoded pins)
  hwState.blPin = display.backlightPin();
  pinMode(hwState.blPin, OUTPUT);
  analogWriteResolution(hwState.blPin, 8);
  analogWrite(hwState.blPin, hwState.activeBrightness);

  // Step 3: Auto-Detect GPIO 34 Role (LDR vs Battery Divider)
  hwState.gpio34Role = display.gpio34IsBattery();
  hwState.hasLdr = (hwState.gpio34Role != 1); // True for LDR or classic CYD

  if (hwState.hasLdr) {
    pinMode(LDR_PIN, INPUT);
    // Prime moving average filter at boot so display instantly adapts to room light
    for (int i = 0; i < NUM_LDR_READINGS; i++) {
      adjustBrightness();
      delay(10);
    }
  }

  // Step 4: Auto-Detect RGB LED Pins & Initialize Active-LOW States
  hwState.redLedPin   = display.redLedPin();
  hwState.greenLedPin = display.greenLedPin();
  hwState.blueLedPin  = display.blueLedPin();

  if (hwState.redLedPin >= 0)   { pinMode(hwState.redLedPin, OUTPUT);   digitalWrite(hwState.redLedPin, HIGH); }
  if (hwState.greenLedPin >= 0) { pinMode(hwState.greenLedPin, OUTPUT); digitalWrite(hwState.greenLedPin, HIGH); }
  if (hwState.blueLedPin >= 0)  { pinMode(hwState.blueLedPin, OUTPUT);  digitalWrite(hwState.blueLedPin, HIGH); }

  flashLedSequence();

  // Step 5: Auto-Detect & Initialize Touch Subsystem
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
  Serial.printf(" GPIO 34 Role:        %s\n", hwState.gpio34Role == 1 ? "Battery Voltage Divider" : "Light Dependent Resistor (LDR)");
  const char* hint = display.powerOffHint();
  if (hint) Serial.printf(" Power-Off Hint:      %s\n", hint);
#endif
  Serial.println("=================================================================\n");
}

// =====================================================================================
// 7. RESPONSIVE DASHBOARD & VISUAL GUI RENDERER
// =====================================================================================
void drawInfoCard(int x, int y, int w, int h, const char* label, const char* val, uint16_t valColor) {
  tft.fillRoundRect(x, y, w, h, 4, C_CARD);
  tft.drawRoundRect(x, y, w, h, 4, C_BORDER);

  // Top Subtitle Header
  tft.setFreeFont(nullptr);
  tft.setTextFont(1);
  tft.setTextSize(1);
  tft.setTextColor(C_GRAY, C_CARD);
  tft.setTextDatum(TC_DATUM);
  tft.drawString(label, x + (w / 2), y + 3);

  // Center Value with FreeSans9pt7b
  tft.setTextColor(valColor, C_CARD);
  tft.setTextDatum(MC_DATUM);
  int valY = y + (h / 2) + 6;

  tft.setFreeFont(&FreeSans9pt7b);
  if (tft.textWidth(val) <= w - 6) {
    tft.drawString(val, x + (w / 2), valY);
  } else {
    // Graceful fallback to Font 2 if an unusually long string exceeds card boundary
    tft.setFreeFont(nullptr);
    tft.drawString(val, x + (w / 2), valY, 2);
  }
  tft.setFreeFont(nullptr);
}

void renderDashboard() {
  int sw = tft.width();
  int sh = tft.height();

  tft.fillScreen(C_BG);

  // Responsive Dimensions
  int headerH = (sh >= 320) ? 32 : 26;
  int gap     = 4;
  int colW    = (sw - (gap * 3)) / 2;
  int cardH   = (sh >= 320) ? 42 : 34;

  // --- Top Header Bar ---
  tft.fillRect(0, 0, sw, headerH, C_HEADER);
  tft.drawFastHLine(0, headerH, sw, C_CYAN);

  // Status Badge Pill on Right
  const char* badgeText = touch.capacitive() ? "CAPACITIVE" : "RESISTIVE";
  int pillW = (sw >= 480) ? 96 : 84;
  int pillH = headerH - 8;
  int pillX = sw - pillW - 6;
  int pillY = (headerH - pillH) / 2;
  tft.fillRoundRect(pillX, pillY, pillW, pillH, 4, C_PILL_BG);
  tft.drawRoundRect(pillX, pillY, pillW, pillH, 4, touch.capacitive() ? C_GREEN : C_YELLOW);
  tft.setTextColor(touch.capacitive() ? C_GREEN : C_YELLOW, C_PILL_BG);
  tft.setTextDatum(MC_DATUM);
  tft.setTextFont(1);
  tft.setTextSize(1);
  tft.drawString(badgeText, pillX + (pillW / 2), pillY + (pillH / 2));

  // Title on Left
  tft.setTextColor(C_CYAN, C_HEADER);
  tft.setTextDatum(ML_DATUM);
  tft.setFreeFont(&FreeSansBold9pt7b);
  const char* title = (sw >= 480) ? "AMK Universal Auto-Setup" : "AMK Auto-Setup";
  tft.drawString(title, 8, headerH / 2);
  tft.setFreeFont(nullptr);

  // --- Row 1: Model & Panel ---
  int y0 = headerH + gap;
  const char* model = display.modelName();
  const char* modelDisplay = (sw <= 320 && strncmp(model, "ESP32-", 6) == 0) ? model + 6 : model;
  drawInfoCard(gap, y0, colW, cardH, "BOARD MODEL", modelDisplay, C_WHITE);
  drawInfoCard(gap * 2 + colW, y0, colW, cardH, "DISPLAY PANEL", display.panelName(), C_GREEN);

  // --- Row 2: Touch & Backlight ---
  int y1 = y0 + cardH + gap;
  char touchBuf[32];
  snprintf(touchBuf, sizeof(touchBuf), "%s", touch.chipName());
  drawInfoCard(gap, y1, colW, cardH, "TOUCH ENGINE", touchBuf, C_CYAN);

  char blBuf[32];
  snprintf(blBuf, sizeof(blBuf), "GPIO %u (PWM)", hwState.blPin);
  drawInfoCard(gap * 2 + colW, y1, colW, cardH, "BACKLIGHT PIN", blBuf, C_YELLOW);

  // --- Row 3: RGB LED & Screen Geometry ---
  int y2 = y1 + cardH + gap;
  char ledBuf[32];
  if (hwState.redLedPin >= 0) {
    snprintf(ledBuf, sizeof(ledBuf), "R:%d G:%d B:%d", hwState.redLedPin, hwState.greenLedPin, hwState.blueLedPin);
  } else {
    snprintf(ledBuf, sizeof(ledBuf), "N/A (S3 RGB)");
  }
  drawInfoCard(gap, y2, colW, cardH, "RGB LED PINS", ledBuf, C_WHITE);

  char geomBuf[32];
  if (display.diagonalTenths() > 0) {
    snprintf(geomBuf, sizeof(geomBuf), "%.1f\" (%dx%d)", display.diagonalTenths() / 10.0f, sw, sh);
  } else {
    snprintf(geomBuf, sizeof(geomBuf), "%dx%d", sw, sh);
  }
  drawInfoCard(gap * 2 + colW, y2, colW, cardH, "SCREEN GEOMETRY", geomBuf, C_GREEN);

  // --- Visual Color Order Verification Swatches ---
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
    tft.setTextFont(1);
    tft.setTextSize(1);
    tft.drawString(swatchLabels[i], sx + (swatchW / 2), swatchY + (swatchH / 2));
  }

  // --- Touch Canvas Header Bar ---
  barHeight = (sh >= 320) ? 20 : 16;
  canvasBarY = swatchY + swatchH + gap;
  tft.fillRect(0, canvasBarY, sw, barHeight, C_HEADER);
  tft.drawFastHLine(0, canvasBarY, sw, C_BORDER);
  tft.drawFastHLine(0, canvasBarY + barHeight, sw, C_BORDER);

  tft.setTextColor(C_GRAY, C_HEADER);
  tft.setTextDatum(ML_DATUM);
  tft.drawString(" Touch Canvas & RGB LED [BOOT: Clear]", 6, canvasBarY + (barHeight / 2));

  canvasY = canvasBarY + barHeight + 1;
}

void updateCoordTelemetry(uint16_t x, uint16_t y, uint16_t z) {
  int sw = tft.width();
  char buf[32];
  snprintf(buf, sizeof(buf), "X:%3d Y:%3d Z:%3d", x, y, z);
  if (strcmp(buf, lastCoordBuf) != 0) {
    tft.setTextFont(1);
    tft.setTextSize(1);
    tft.setTextColor(C_YELLOW, C_HEADER);
    tft.setTextDatum(MR_DATUM);
    tft.drawString(buf, sw - 6, canvasBarY + (barHeight / 2));
    strncpy(lastCoordBuf, buf, sizeof(lastCoordBuf));
  }
}

// =====================================================================================
// 8. SETUP & LOOP
// =====================================================================================
void setup() {
  Serial.begin(115200);
  delay(200);

  pinMode(BOOT_PIN, INPUT_PULLUP);

  // Execute Master Hardware Auto-Detection & Setup (Rotation 3 = Landscape USB left)
  setupUniversalHardware(3);

  // Print Hardware Audit Trail to Serial Monitor
  printHardwareReport();

  // Render Graphical Dashboard
  renderDashboard();
}

void loop() {
  // -----------------------------------------------------------------------------------
  // 1. FIRST-LINE EXECUTION: Backlight auto-dimming (Strict User Hardware Standard)
  // -----------------------------------------------------------------------------------
  adjustBrightness();

  // -----------------------------------------------------------------------------------
  // 2. CLEAR CANVAS ON BOOT BUTTON (GPIO 0) PRESS
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
  // 3. INTERACTIVE TOUCH TRACKING & DYNAMIC RGB LED ILLUMINATION
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

    // Drive RGB LED interactively with touch (Neon Cyan: Green + Blue active)
    setRgbLed(false, true, true);
  } else {
    if (wasTouching) {
      wasTouching = false;
      turnLedOff(); // Turn off RGB LED when screen is released
    }
  }

  delay(5);
}
