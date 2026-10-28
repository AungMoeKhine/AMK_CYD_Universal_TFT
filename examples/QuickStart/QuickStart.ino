/*
 * QuickStart.ino - AMK_CYD_Universal_TFT
 * Minimal, complete boilerplate template for any ESP32 CYD board.
 */

#include <Arduino.h>
#include <AMK_CYD_Universal_TFT.h>

// 1. Global driver objects
AMK_CYD_display display;
AMK_CYD_touch   touch;
TFT_eSPI        tft;

void setup() {
  Serial.begin(115200);

  // 2. Initialize display (auto-probes panel & board wiring)
  // Optional manual overrides (if needed for custom panels):
  // tft.setPanel(CYDR_PANEL_ILI9341); // Force ILI9341 or CYDR_PANEL_ST7789 before init()
  tft.init();
  tft.setRotation(1);       // 0 = Portrait, 1 = Landscape, 2 = Flip Port, 3 = Flip Land
  // tft.setMirrorX(true);  // Invert horizontal axis if ever mirrored
  tft.setBacklight(100);    // Backlight brightness (0 - 100%)

  // 3. Initialize touch controller
  touch.begin(display);
  touch.setRotation(1);     // Match display rotation
  touch.loadCalibration();  // Load NVS calibration if previously saved
  touch.loadFilters();

  // 4. Draw UI
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("AMK_CYD_Universal_TFT", tft.width() / 2, 40, 1);

  // Show detected hardware specs
  char info[96];
  snprintf(info, sizeof(info), "Model: %s | %s (%dx%d)", display.modelName(), display.panelName(), tft.width(), tft.height());
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.drawString(info, tft.width() / 2, 70, 1);
}

void loop() {
  uint16_t x = 0, y = 0;

  // 5. Read touch coordinates (returns true while pressed)
  if (tft.getTouch(&x, &y)) {
    tft.fillCircle(x, y, 4, TFT_YELLOW); // Draw dot at touch position
  }

  delay(5);
}
