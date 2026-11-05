// ==============================================================================
// ButtonDemo.ino - AMK_CYD_Universal_TFT
//
// Demonstrates 100% drop-in compatibility with Bodmer's TFT_eSPI_Button class!
//
// Features demonstrated:
//   * Standard TFT_eSPI_Button creation with initButton()
//   * Coordinate hit-testing with contains(x, y)
//   * Press state handling with press(), justPressed(), justReleased()
//   * Inverted button drawing on touch feedback
//   * Perfectly centered, balanced FreeFont status card with equal margins
//   * Real hardware control:
//       - RED / GREEN / BLUE buttons toggle the onboard RGB LED
//       - BRT button toggles screen brightness (40% / 100%)
//
// Compatible with all Cheap Yellow Display (CYD) screen sizes & orientations!
// ==============================================================================

#include <Arduino.h>
#include <AMK_CYD_Universal_TFT.h>

AMK_display display;
AMK_touch   touch;
TFT_eSPI    tft;

// Create 4 buttons using the drop-in TFT_eSPI_Button class
TFT_eSPI_Button btnRed;
TFT_eSPI_Button btnGreen;
TFT_eSPI_Button btnBlue;
TFT_eSPI_Button btnBrt;

uint8_t brightness = 100;

// Dimensions of the status card
int16_t box_x;
int16_t box_y;
int16_t box_w;
int16_t box_h;

// Helper: Clears the status card interior and draws smooth, centered FreeFont text
void updateStatus(uint16_t color, const char* line1, const char* line2) {
    // Clear entire inner area to guarantee zero ghost artifacts
    tft.fillRoundRect(box_x + 2, box_y + 2, box_w - 4, box_h - 4, 4, TFT_BLACK);

    // CRITICAL: Reset textSize to 1 because TFT_eSPI_Button::drawButton() leaves textSize set to 2!
    tft.setTextSize(1);
    tft.setTextDatum(MC_DATUM);

    // Line 1: Status tag (smooth FreeSansBold9pt7b) centered with equal vertical padding
    tft.setFreeFont(&FreeSansBold9pt7b);
    tft.setTextColor(color, TFT_BLACK);
    tft.drawString(line1, tft.width() / 2, box_y + (box_h * 36 / 100));

    // Line 2: Detail message (smooth FreeSans9pt7b) centered with equal vertical padding
    tft.setFreeFont(&FreeSans9pt7b);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    if (tft.textWidth(line2) <= box_w - 12) {
        tft.drawString(line2, tft.width() / 2, box_y + (box_h * 68 / 100));
    } else {
        // Safe fallback to Font 1 if an extra long string exceeds box width
        tft.setFreeFont(nullptr);
        tft.drawString(line2, tft.width() / 2, box_y + (box_h * 68 / 100), 1);
    }
    tft.setFreeFont(nullptr);
}

void setup() {
    Serial.begin(115200);
    delay(200);
    Serial.println("\n[AMK_CYD_Universal_TFT] Starting ButtonDemo (TFT_eSPI_Button)...");

    // 1. Initialize display & touch (Rotation 3: Standard Landscape)
    tft.init();
    tft.setRotation(3);
    tft.setBacklight(100);

    touch.begin(display);
    touch.setRotation(3);
    if (!touch.capacitive()) {
        touch.loadCalibration();
        touch.loadFilters();
    }

    // 2. Setup onboard RGB LED pins (active-low)
#if !defined(AMK_BACKEND_ESP32_S3)
    pinMode(display.redLedPin(), OUTPUT);
    pinMode(display.greenLedPin(), OUTPUT);
    pinMode(display.blueLedPin(), OUTPUT);
    digitalWrite(display.redLedPin(), HIGH);
    digitalWrite(display.greenLedPin(), HIGH);
    digitalWrite(display.blueLedPin(), HIGH);
#endif

    int sw = tft.width();
    int sh = tft.height();

    // 3. Draw Header with centered FreeFont alignment
    tft.fillScreen(TFT_BLACK);
    tft.setFreeFont(&FreeSansBold9pt7b);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.drawString("TFT_eSPI_Button Demo", sw / 2, 14);

    tft.setFreeFont(nullptr);
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.drawString("Touch buttons below to control hardware", sw / 2, 34, 1);

    // 4. Calculate mathematically balanced button layout with identical margins
    int16_t gap        = (sw >= 480) ? 12 : 6;
    int16_t totalBtnsW = sw - (gap * 5);
    int16_t btn_w      = totalBtnsW / 4;
    int16_t rem        = totalBtnsW % 4;
    int16_t sideMargin = gap + (rem / 2); // Perfectly equal left & right margin

    int16_t btn_h = (sh >= 320) ? 50 : 38;
    int16_t btn_y = 52 + (btn_h / 2);

    int16_t x1 = sideMargin + (btn_w / 2);
    int16_t x2 = x1 + btn_w + gap;
    int16_t x3 = x2 + btn_w + gap;
    int16_t x4 = x3 + btn_w + gap;

    // initButton(gfx, x, y, width, height, outline, fill, textcolor, label, textsize)
    btnRed.initButton(&tft,   x1, btn_y, btn_w, btn_h, TFT_WHITE, TFT_RED,   TFT_WHITE, "RED", 2);
    btnGreen.initButton(&tft, x2, btn_y, btn_w, btn_h, TFT_WHITE, TFT_GREEN, TFT_BLACK, "GRN", 2);
    btnBlue.initButton(&tft,  x3, btn_y, btn_w, btn_h, TFT_WHITE, TFT_BLUE,  TFT_WHITE, "BLU", 2);
    btnBrt.initButton(&tft,   x4, btn_y, btn_w, btn_h, TFT_WHITE, TFT_NAVY,  TFT_CYAN,  "BRT", 2);

    btnRed.drawButton();
    btnGreen.drawButton();
    btnBlue.drawButton();
    btnBrt.drawButton();
    tft.setTextSize(1); // Restore standard text scale after drawing buttons

    // 5. Draw bottom status card matching the exact button strip width & side margins
    box_x = sideMargin;
    box_y = 52 + btn_h + 8;
    box_w = sw - (sideMargin * 2);
    box_h = sh - box_y - 8; // Equal 8px bottom margin

    tft.drawRoundRect(box_x, box_y, box_w, box_h, 6, TFT_DARKGREY);
    updateStatus(TFT_GREENYELLOW, "[ READY ]", "Tap any button to test");
}

void loop() {
    uint16_t t_x = 0, t_y = 0;
    bool touched = tft.getTouch(&t_x, &t_y);

    // Update press states
    btnRed.press(touched && btnRed.contains(t_x, t_y));
    btnGreen.press(touched && btnGreen.contains(t_x, t_y));
    btnBlue.press(touched && btnBlue.contains(t_x, t_y));
    btnBrt.press(touched && btnBrt.contains(t_x, t_y));

    // Handle RED Button
    if (btnRed.justPressed()) {
        btnRed.drawButton(true);                // Invert button
        tft.setTextSize(1);
#if !defined(AMK_BACKEND_ESP32_S3)
        digitalWrite(display.redLedPin(), LOW); // Turn RED LED ON
#endif
        updateStatus(TFT_RED, "[ RED BUTTON ]", "Onboard Red LED: ON");
    }
    if (btnRed.justReleased()) {
        btnRed.drawButton(false);
        tft.setTextSize(1);
#if !defined(AMK_BACKEND_ESP32_S3)
        digitalWrite(display.redLedPin(), HIGH); // Turn RED LED OFF
#endif
    }

    // Handle GREEN Button
    if (btnGreen.justPressed()) {
        btnGreen.drawButton(true);
        tft.setTextSize(1);
#if !defined(AMK_BACKEND_ESP32_S3)
        digitalWrite(display.greenLedPin(), LOW); // Turn GREEN LED ON
#endif
        updateStatus(TFT_GREEN, "[ GREEN BUTTON ]", "Onboard Green LED: ON");
    }
    if (btnGreen.justReleased()) {
        btnGreen.drawButton(false);
        tft.setTextSize(1);
#if !defined(AMK_BACKEND_ESP32_S3)
        digitalWrite(display.greenLedPin(), HIGH);
#endif
    }

    // Handle BLUE Button
    if (btnBlue.justPressed()) {
        btnBlue.drawButton(true);
        tft.setTextSize(1);
#if !defined(AMK_BACKEND_ESP32_S3)
        digitalWrite(display.blueLedPin(), LOW); // Turn BLUE LED ON
#endif
        updateStatus(TFT_CYAN, "[ BLUE BUTTON ]", "Onboard Blue LED: ON");
    }
    if (btnBlue.justReleased()) {
        btnBlue.drawButton(false);
        tft.setTextSize(1);
#if !defined(AMK_BACKEND_ESP32_S3)
        digitalWrite(display.blueLedPin(), HIGH);
#endif
    }

    // Handle BRIGHTNESS Button
    if (btnBrt.justPressed()) {
        btnBrt.drawButton(true);
        tft.setTextSize(1);
        brightness = (brightness == 100) ? 40 : 100;
        display.setBacklight(brightness);

        char buf[32];
        snprintf(buf, sizeof(buf), "LCD Backlight PWM: %d%%", brightness);
        updateStatus(TFT_YELLOW, "[ BACKLIGHT ]", buf);
    }
    if (btnBrt.justReleased()) {
        btnBrt.drawButton(false);
        tft.setTextSize(1);
    }

    delay(10);
}
