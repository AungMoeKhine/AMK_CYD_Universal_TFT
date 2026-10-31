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
//   * Clean status card with zero overflow and zero ghost artifacts
//   * Real hardware control:
//       - RED / GREEN / BLUE buttons toggle the onboard RGB LED
//       - BRT button toggles screen brightness (40% / 100%)
//
// Compatible with all existing TFT_eSPI sketches using TFT_eSPI_Button!
// ==============================================================================

#include <Arduino.h>
#include <AMK_CYD_Universal_TFT.h>

AMK_CYD_display display;
AMK_CYD_touch   touch;
TFT_eSPI        tft;

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

// Helper: Clears the status card interior and draws crisp, non-overflowing text
void updateStatus(uint16_t color, const char* line1, const char* line2) {
    // Clear entire inner area to guarantee zero ghost artifacts
    tft.fillRoundRect(box_x + 2, box_y + 2, box_w - 4, box_h - 4, 4, TFT_BLACK);

    tft.setTextDatum(MC_DATUM);

    // Line 1: Status tag (Font 2)
    tft.setTextColor(color, TFT_BLACK);
    tft.drawString(line1, tft.width() / 2, box_y + 26, 2);

    // Line 2: Detail message (Font 2)
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString(line2, tft.width() / 2, box_y + 58, 2);
}

void setup() {
    Serial.begin(115200);
    delay(200);
    Serial.println("\n[AMK_CYD_Universal_TFT] Starting ButtonDemo (TFT_eSPI_Button)...");

    // 1. Initialize display & touch
    tft.init();
    tft.setRotation(3);      // Standard Landscape (Rotation 3)
    tft.setBacklight(100);

    touch.begin(display);
    touch.setRotation(3);
    touch.loadCalibration();

    // 2. Setup onboard RGB LED pins (active-low)
    pinMode(display.redLedPin(), OUTPUT);
    pinMode(display.greenLedPin(), OUTPUT);
    pinMode(display.blueLedPin(), OUTPUT);
    digitalWrite(display.redLedPin(), HIGH);
    digitalWrite(display.greenLedPin(), HIGH);
    digitalWrite(display.blueLedPin(), HIGH);

    // 3. Draw Header (Font 2 fits 320x240 cleanly with generous side padding)
    tft.fillScreen(TFT_BLACK);
    tft.setTextDatum(TC_DATUM);
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.drawString("TFT_eSPI_Button Demo", tft.width() / 2, 8, 2);

    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.drawString("Touch buttons below to control hardware", tft.width() / 2, 30, 1);

    // 4. Calculate responsive button layout
    int16_t btn_w = 66;
    int16_t btn_h = 40;
    int16_t btn_y = 75;
    int16_t gap   = (tft.width() - (btn_w * 4)) / 5;

    int16_t x1 = gap + (btn_w / 2);
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

    // 5. Draw bottom status card
    box_x = 12;
    box_y = 118;
    box_w = tft.width() - 24;
    box_h = tft.height() - box_y - 12;

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
        digitalWrite(display.redLedPin(), LOW); // Turn RED LED ON
        updateStatus(TFT_RED, "[ RED BUTTON ]", "Onboard Red LED: ON");
    }
    if (btnRed.justReleased()) {
        btnRed.drawButton(false);
        digitalWrite(display.redLedPin(), HIGH); // Turn RED LED OFF
    }

    // Handle GREEN Button
    if (btnGreen.justPressed()) {
        btnGreen.drawButton(true);
        digitalWrite(display.greenLedPin(), LOW); // Turn GREEN LED ON
        updateStatus(TFT_GREEN, "[ GREEN BUTTON ]", "Onboard Green LED: ON");
    }
    if (btnGreen.justReleased()) {
        btnGreen.drawButton(false);
        digitalWrite(display.greenLedPin(), HIGH);
    }

    // Handle BLUE Button
    if (btnBlue.justPressed()) {
        btnBlue.drawButton(true);
        digitalWrite(display.blueLedPin(), LOW); // Turn BLUE LED ON
        updateStatus(TFT_CYAN, "[ BLUE BUTTON ]", "Onboard Blue LED: ON");
    }
    if (btnBlue.justReleased()) {
        btnBlue.drawButton(false);
        digitalWrite(display.blueLedPin(), HIGH);
    }

    // Handle BRIGHTNESS Button
    if (btnBrt.justPressed()) {
        btnBrt.drawButton(true);
        brightness = (brightness == 100) ? 40 : 100;
        display.setBacklight(brightness);

        char buf[32];
        snprintf(buf, sizeof(buf), "LCD Backlight PWM: %d%%", brightness);
        updateStatus(TFT_YELLOW, "[ BACKLIGHT ]", buf);
    }
    if (btnBrt.justReleased()) {
        btnBrt.drawButton(false);
    }

    delay(10);
}
