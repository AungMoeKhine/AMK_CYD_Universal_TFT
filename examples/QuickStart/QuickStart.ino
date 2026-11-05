/*
 * QuickStart.ino - AMK_CYD_Universal_TFT
 * Minimal, complete boilerplate template for any ESP32 CYD board.
 * Features balanced, centered FreeFont alignment with equal margins.
 * Works out-of-the-box across resistive & capacitive models with Rotation 3.
 */

#include <Arduino.h>
#include <AMK_CYD_Universal_TFT.h>

// 1. Global driver objects
AMK_display display;
AMK_touch   touch;
TFT_eSPI    tft;

void setup() {
  Serial.begin(115200);

  // 2. Initialize display (auto-probes panel & board wiring)
  tft.init();
  tft.setRotation(3);       // Landscape (Rotation 3: USB left, SD card top)
  tft.setBacklight(100);    // Backlight brightness (0 - 100%)

  // 3. Initialize touch controller
  touch.begin(display);
  touch.setRotation(3);     // Match display rotation
  if (!touch.capacitive()) {
    touch.loadCalibration(); // Load NVS calibration if previously saved
    touch.loadFilters();
  }

  // 4. Draw UI with smooth FreeFonts and balanced centered layout
  int sw = tft.width();
  int sh = tft.height();
  tft.fillScreen(TFT_BLACK);

  // Top Title
  const char* title = "AMK QuickStart Demo";
  tft.setFreeFont(&FreeSansBold9pt7b);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setTextDatum(MC_DATUM);
  tft.drawString(title, sw / 2, 24);

  // Balanced Info Card with equal side and vertical margins
  int margin = (sw >= 480) ? 16 : 10;
  int boxX   = margin;
  int boxW   = sw - (margin * 2);
  int boxY   = 44;
  int boxH   = (sh >= 320) ? 90 : 76;

  tft.fillRoundRect(boxX, boxY, boxW, boxH, 6, 0x10A2); // Charcoal card fill
  tft.drawRoundRect(boxX, boxY, boxW, boxH, 6, 0x2965); // High-tech border

  // Centered vertical rhythm inside the card with unified FreeSans typography
  int l1_Y = boxY + (boxH * 22 / 100); // Line 1
  int l2_Y = boxY + (boxH * 50 / 100); // Line 2 (center)
  int l3_Y = boxY + (boxH * 78 / 100); // Line 3

  tft.setTextDatum(MC_DATUM);
  tft.setFreeFont(&FreeSans9pt7b);

  // Line 1: Board & Panel Info
  char info1[64];
  snprintf(info1, sizeof(info1), "%s | %s", display.modelName(), display.panelName());
  tft.setTextColor(TFT_GREEN, 0x10A2);
  tft.drawString(info1, sw / 2, l1_Y);

  // Line 2: Touch Engine Info
  char info2[64];
  snprintf(info2, sizeof(info2), "Touch: %s (%s)", touch.chipName(), touch.capacitive() ? "Capacitive" : "Resistive");
  tft.setTextColor(TFT_YELLOW, 0x10A2);
  tft.drawString(info2, sw / 2, l2_Y);

  // Line 3: Interactive Prompt
  tft.setTextColor(0x9CD3, 0x10A2);
  tft.drawString("Touch screen to draw dots", sw / 2, l3_Y);
  tft.setFreeFont(nullptr);
}

void loop() {
  uint16_t x = 0, y = 0;

  // 5. Read touch coordinates (returns true while pressed)
  if (tft.getTouch(&x, &y)) {
    tft.fillCircle(x, y, 4, TFT_YELLOW); // Draw dot at touch position
  }

  delay(5);
}
