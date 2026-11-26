/*
 * BasicTouchDemo.ino - AMK_CYD_Universal_TFT
 * Sample sketch demonstrating universal touch, smooth FreeFonts,
 * and balanced symmetrical UI alignment across all CYD boards.
 * Works out-of-the-box on ANY CYD board (2.4", 2.8", 3.2", 3.5", 4.0")
 * resistive & capacitive, with standard Rotation 3 landscape.
 */

#include <Arduino.h>
#include <AMK_CYD_Universal_TFT.h>

// ═══════════════════════════════════════════════════════════════════════════════
// 1. GLOBAL OBJECT DECLARATIONS
// ═══════════════════════════════════════════════════════════════════════════════
AMK_display display;
AMK_touch   touch;
TFT_eSPI    tft;

// Dynamic layout coordinates
static int btnW = 100;
static int btnH = 26;
static int btnX = 0;
static int btnY = 0;
static int canvasTop = 100;

// ═══════════════════════════════════════════════════════════════════════════════
// 2. SETUP
// ═══════════════════════════════════════════════════════════════════════════════
void setup() {
  Serial.begin(115200);
  delay(200);

  // 1. Universal 1-line auto-setup (v1.6.0)
  // Automatically detects and configures panel, rotation, backlight PWM,
  // LDR moving average filter, Active-LOW RGB LED, and touch controller!
  tft.autoSetup(3);
  tft.autoCalibrate(); // Auto-calibrates uncalibrated resistive touch (< 1 ms if already saved; 0 ms if capacitive)

  // 2. Draw balanced, centered responsive interface
  int sw = tft.width();
  int sh = tft.height();

  tft.fillScreen(TFT_BLACK);

  // Header bar with FreeFont (centered, zero overflow)
  int headerH = (sh >= 320) ? 32 : 26;
  tft.fillRect(0, 0, sw, headerH, TFT_NAVY);
  tft.drawFastHLine(0, headerH, sw, TFT_CYAN);

  const char* title = (sw >= 480) ? "AMK Universal TFT Touch Demo" : "AMK Universal Touch Demo";
  tft.setFreeFont(&FreeSansBold9pt7b);
  if (tft.textWidth(title) > sw - 16) {
    title = "AMK Touch Demo";
  }
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_WHITE, TFT_NAVY);
  tft.drawString(title, sw / 2, headerH / 2);

  // Hardware info card with perfectly equal margins (top/bottom/left/right)
  int margin = (sw >= 480) ? 14 : 10;
  int boxX   = margin;
  int boxW   = sw - (margin * 2);
  int boxY   = headerH + 6;
  int boxH   = (sh >= 320) ? 78 : 66;

  tft.fillRoundRect(boxX, boxY, boxW, boxH, 6, 0x10A2); // Dark slate fill
  tft.drawRoundRect(boxX, boxY, boxW, boxH, 6, 0x2965); // High-tech border

  // Centered vertical rhythm inside the card with unified FreeSans typography
  int l1_Y = boxY + (boxH * 22 / 100); // Line 1
  int l2_Y = boxY + (boxH * 50 / 100); // Line 2 (center)
  int l3_Y = boxY + (boxH * 78 / 100); // Line 3

  tft.setTextDatum(MC_DATUM);
  tft.setFreeFont(&FreeSans9pt7b);

  // Line 1: Board & Panel Info (balanced string fits FreeSans perfectly with generous side margins)
  char buf1[64];
  snprintf(buf1, sizeof(buf1), "%s | %s", display.modelName(), display.panelName());
  tft.setTextColor(TFT_GREEN, 0x10A2);
  tft.drawString(buf1, sw / 2, l1_Y);

  // Line 2: Touch Engine Info
  char buf2[64];
  snprintf(buf2, sizeof(buf2), "Touch: %s (%s)", touch.chipName(), touch.capacitive() ? "Capacitive" : "Resistive");
  tft.setTextColor(TFT_CYAN, 0x10A2);
  tft.drawString(buf2, sw / 2, l2_Y);

  // Line 3: Interactive Instruction Prompt (harmonious size matching Line 1 & Line 2)
  tft.setTextColor(TFT_YELLOW, 0x10A2);
  tft.drawString("Touch screen to draw dots", sw / 2, l3_Y);
  tft.setFreeFont(nullptr);

  // Responsive, centered Clear button at bottom
  canvasTop = boxY + boxH + 6;
  btnW = (sw >= 480) ? 120 : 100;
  btnH = (sh >= 320) ? 32 : 26;
  btnX = (sw - btnW) / 2;
  btnY = sh - btnH - 8; // Equal 8px bottom margin

  tft.fillRoundRect(btnX, btnY, btnW, btnH, 5, TFT_MAROON);
  tft.drawRoundRect(btnX, btnY, btnW, btnH, 5, TFT_WHITE);
  tft.setFreeFont(&FreeSansBold9pt7b);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_WHITE, TFT_MAROON);
  tft.drawString("CLEAR", sw / 2, btnY + (btnH / 2));
  tft.setFreeFont(nullptr);
}

// ═══════════════════════════════════════════════════════════════════════════════
// 3. LOOP
// ═══════════════════════════════════════════════════════════════════════════════
void loop() {
  // Built-in ambient light auto-dimming (first line of loop)
  tft.adjustBrightness();

  uint16_t x = 0, y = 0;

  // Read touch (returns true while screen is touched)
  if (tft.getTouch(&x, &y)) {
    // Check if the "CLEAR" button at the bottom was tapped
    if (x >= btnX && x <= btnX + btnW && y >= btnY && y <= btnY + btnH) {
      // Clear drawing canvas area
      tft.fillRect(0, canvasTop, tft.width(), btnY - canvasTop - 4, TFT_BLACK);
      tft.setRgbLed(true, false, false); // Red pulse on clear
      delay(150); // debounce tap
      tft.turnLedOff();
    } else if (y >= canvasTop && y < btnY - 4) {
      // Draw touch dot in the canvas area
      tft.fillCircle(x, y, 3, TFT_CYAN);
      tft.setRgbLed(false, true, true); // Neon Cyan on touch
    }
  } else {
    tft.turnLedOff();
  }

  delay(5);
}
