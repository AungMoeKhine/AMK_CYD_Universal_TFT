/*
 * BasicTouchDemo.ino
 * Sample sketch demonstrating the AMK_CYD_Universal_TFT library.
 * Works out-of-the-box on ANY CYD resistive board (2.4", 2.8", 3.2", 3.5", 4.0")
 */

#include <Arduino.h>
#include <AMK_CYD_Universal_TFT.h>

// ═══════════════════════════════════════════════════════════════════════════════
// 1. GLOBAL OBJECT DECLARATIONS
// ═══════════════════════════════════════════════════════════════════════════════
// Driver objects
AMK_CYD_display display;
AMK_CYD_touch   touch;
TFT_eSPI        tft;

// ═══════════════════════════════════════════════════════════════════════════════
// 2. SETUP
// ═══════════════════════════════════════════════════════════════════════════════
void setup() {
  Serial.begin(115200);
  delay(200);

  // 1. Initialize display (auto-detects board wiring and panel controller)
  tft.init();

  // 2. Set rotation: 0 = Portrait, 1 = Landscape, 2 = Portrait Flip, 3 = Landscape Flip
  tft.setRotation(3);

  // 3. Set backlight brightness (0 to 100%). Automatically uses GPIO21 or GPIO27
  tft.setBacklight(100);

  // 4. Initialize touch and sync orientation with the display
  touch.begin(display);
  touch.setRotation(3);

  // 5. Load calibration and noise filters from NVS flash (saved by CYD_TouchCal)
  if (touch.loadCalibration()) {
    Serial.println("Loaded touch calibration from NVS flash.");
  } else {
    Serial.println("No saved calibration found; using hardware defaults.");
  }
  touch.loadFilters();

  // 6. Draw demo interface
  tft.fillScreen(TFT_BLACK);

  // Header bar
  tft.fillRect(0, 0, tft.width(), 26, TFT_NAVY);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_WHITE, TFT_NAVY);
  tft.drawString("AMK_CYD_Universal_TFT Demo", tft.width() / 2, 13, 1);

  // Hardware info box
  tft.drawRect(8, 34, tft.width() - 16, 56, TFT_DARKGREY);
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);

  char buf[64];
  snprintf(buf, sizeof(buf), "Panel: %s (%dx%d)", display.panelName(), tft.width(), tft.height());
  tft.drawString(buf, 16, 40, 1);

  snprintf(buf, sizeof(buf), "Wiring: %s", display.board() == CYDR_BOARD_SHARED_TOUCH ? "Shared Touch (SPI)" : "Own Touch (Pins)");
  tft.drawString(buf, 16, 54, 1);

  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.drawString("Touch anywhere below to draw dots!", 16, 70, 1);

  // Clear button at bottom
  tft.fillRoundRect(tft.width() / 2 - 40, tft.height() - 28, 80, 22, 4, TFT_MAROON);
  tft.drawRoundRect(tft.width() / 2 - 40, tft.height() - 28, 80, 22, 4, TFT_WHITE);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_WHITE, TFT_MAROON);
  tft.drawString("CLEAR", tft.width() / 2, tft.height() - 17, 1);
}

// ═══════════════════════════════════════════════════════════════════════════════
// 3. LOOP
// ═══════════════════════════════════════════════════════════════════════════════
void loop() {
  uint16_t x = 0, y = 0;

  // Read touch (returns true while screen is touched)
  // Both tft.getTouch(&x, &y) and touch.read(x, y) work:
  if (tft.getTouch(&x, &y)) {
    // Check if the "CLEAR" button at the bottom was tapped
    int btnX = tft.width() / 2 - 40;
    int btnY = tft.height() - 28;
    if (x >= btnX && x <= btnX + 80 && y >= btnY && y <= btnY + 22) {
      // Clear drawing area
      tft.fillRect(0, 94, tft.width(), tft.height() - 94 - 32, TFT_BLACK);
      delay(150); // debounce tap
    } else if (y > 94 && y < tft.height() - 30) {
      // Draw touch dot in the canvas area
      tft.fillCircle(x, y, 3, TFT_CYAN);
    }
  }

  delay(5);
}
