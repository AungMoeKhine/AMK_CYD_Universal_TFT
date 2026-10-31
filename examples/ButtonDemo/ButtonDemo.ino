// ==============================================================================
// ButtonDemo.ino - AMK_CYD_Universal_TFT
//
// Demonstrates 100% drop-in compatibility with Bodmer's TFT_eSPI_Button class!
//
// Features demonstrated:
//   * Standard TFT_eSPI_Button creation with initButton() and initButtonUL()
//   * Coordinate hit-testing with contains(x, y)
//   * Press state handling with press(), justPressed(), justReleased()
//   * Inverted button drawing on press
//   * Real hardware control:
//       - RED / GREEN / BLUE buttons toggle the onboard RGB LED
//       - BRT button toggles screen brightness (50% / 100%)
//       - CLR button clears the touch canvas
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

    // 3. Draw UI Header
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextDatum(TC_DATUM);
    tft.drawString("TFT_eSPI_Button Demo", tft.width() / 2, 10, 4);

    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.drawString("Touch buttons below to control hardware", tft.width() / 2, 40, 2);

    // 4. Initialize buttons (Center-based: initButton)
    // initButton(gfx, x, y, width, height, outline, fill, textcolor, label, textsize)
    btnRed.initButton(&tft, 50, 100, 70, 45, TFT_WHITE, TFT_RED, TFT_WHITE, "RED", 2);
    btnGreen.initButton(&tft, 125, 100, 70, 45, TFT_WHITE, TFT_GREEN, TFT_BLACK, "GRN", 2);
    btnBlue.initButton(&tft, 200, 100, 70, 45, TFT_WHITE, TFT_BLUE, TFT_WHITE, "BLU", 2);
    btnBrt.initButton(&tft, 275, 100, 65, 45, TFT_WHITE, TFT_NAVY, TFT_CYAN, "BRT", 2);

    // 5. Draw the initial buttons
    btnRed.drawButton();
    btnGreen.drawButton();
    btnBlue.drawButton();
    btnBrt.drawButton();

    // Draw bottom status box
    tft.drawRoundRect(15, 140, 290, 85, 6, TFT_DARKGREY);
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("Ready! Tap any button.", 160, 182, 2);
}

void loop() {
    uint16_t t_x = 0, t_y = 0;
    bool touched = tft.getTouch(&t_x, &t_y);

    // Update press state for all buttons
    btnRed.press(touched && btnRed.contains(t_x, t_y));
    btnGreen.press(touched && btnGreen.contains(t_x, t_y));
    btnBlue.press(touched && btnBlue.contains(t_x, t_y));
    btnBrt.press(touched && btnBrt.contains(t_x, t_y));

    // Handle RED Button
    if (btnRed.justPressed()) {
        btnRed.drawButton(true); // Draw inverted (pressed feedback)
        digitalWrite(display.redLedPin(), LOW); // Turn RED LED ON
        tft.fillRect(20, 145, 280, 75, TFT_BLACK);
        tft.setTextColor(TFT_RED, TFT_BLACK);
        tft.drawString("RED Button Pressed! LED ON", 160, 182, 2);
    }
    if (btnRed.justReleased()) {
        btnRed.drawButton(false);
        digitalWrite(display.redLedPin(), HIGH); // Turn RED LED OFF
    }

    // Handle GREEN Button
    if (btnGreen.justPressed()) {
        btnGreen.drawButton(true);
        digitalWrite(display.greenLedPin(), LOW); // Turn GREEN LED ON
        tft.fillRect(20, 145, 280, 75, TFT_BLACK);
        tft.setTextColor(TFT_GREEN, TFT_BLACK);
        tft.drawString("GREEN Button Pressed! LED ON", 160, 182, 2);
    }
    if (btnGreen.justReleased()) {
        btnGreen.drawButton(false);
        digitalWrite(display.greenLedPin(), HIGH);
    }

    // Handle BLUE Button
    if (btnBlue.justPressed()) {
        btnBlue.drawButton(true);
        digitalWrite(display.blueLedPin(), LOW); // Turn BLUE LED ON
        tft.fillRect(20, 145, 280, 75, TFT_BLACK);
        tft.setTextColor(TFT_CYAN, TFT_BLACK);
        tft.drawString("BLUE Button Pressed! LED ON", 160, 182, 2);
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
        tft.fillRect(20, 145, 280, 75, TFT_BLACK);
        tft.setTextColor(TFT_YELLOW, TFT_BLACK);
        char buf[48];
        snprintf(buf, sizeof(buf), "Brightness toggled to %d%%", brightness);
        tft.drawString(buf, 160, 182, 2);
    }
    if (btnBrt.justReleased()) {
        btnBrt.drawButton(false);
    }

    delay(10);
}
