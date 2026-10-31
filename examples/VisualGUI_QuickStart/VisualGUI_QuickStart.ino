// ==============================================================================
// VisualGUI_QuickStart.ino - AMK_CYD_Universal_TFT
//
// Demonstrates seamless integration of visual drag-and-drop UI designers
// (SquareLine Studio, EEZ Studio, etc.) running on any ESP32 CYD board.
//
// Features demonstrated:
//   * 1-Line LVGL setup with AMK_CYD_LVGL (hardware DMA double-buffering)
//   * Interactive UI widgets designed visually (Arc Dial, Slider, Switch, Button)
//   * Real hardware interaction:
//       - Dragging the UI Brightness Slider adjusts CYD physical backlight PWM!
//       - Toggling the UI Switch turns the CYD onboard RGB LED on/off!
//       - Adjusting the Temperature Arc updates live telemetry!
//
// Compatible with both LVGL v8.x and v9.x!
// ==============================================================================

#include <Arduino.h>
#include <AMK_CYD_Universal_TFT.h>
#include <AMK_CYD_LVGL.h>
#include "ui.h"

// Global driver instances
AMK_CYD_display display;
AMK_CYD_touch   touch;
TFT_eSPI        tft;
AMK_CYD_LVGL    lvgl;

// ------------------------------------------------------------------------------
// SquareLine Studio Event Callbacks (Linking UI to Real CYD Hardware)
// ------------------------------------------------------------------------------

// Backlight Slider Event: Adjusts the actual CYD LCD backlight PWM!
void ui_event_slider_brightness(lv_event_t * e) {
    lv_obj_t * slider = (lv_obj_t *)lv_event_get_target(e);
    int32_t val = lv_slider_get_value(slider);

    // Update physical backlight (0-100%)
    display.setBacklight((uint8_t)val);

    // Update UI readout label
    char buf[16];
    snprintf(buf, sizeof(buf), "%d%%", (int)val);
    lv_label_set_text(ui_LabelBrightValue, buf);
}

// Onboard RGB LED Switch Event: Toggles the CYD onboard Red LED!
void ui_event_switch_led(lv_event_t * e) {
    lv_obj_t * sw = (lv_obj_t *)lv_event_get_target(e);
    bool state = lv_obj_has_state(sw, LV_STATE_CHECKED);

    // CYD onboard RGB LED is active-LOW
    uint8_t pin = display.redLedPin();
    pinMode(pin, OUTPUT);
    digitalWrite(pin, state ? LOW : HIGH); // LOW = ON, HIGH = OFF
    Serial.printf("[VisualGUI] RGB LED: %s (Pin %d)\n", state ? "ON" : "OFF", pin);
}

// Action Button Event
void ui_event_btn_action(lv_event_t * e) {
    Serial.println("[VisualGUI] Action button pressed!");
    // Optional: add audio beep on GPIO 26 here if speaker is attached
}

// Temperature Arc Event: Updates label as user drags the dial
void ui_event_arc_temp(lv_event_t * e) {
    lv_obj_t * arc = (lv_obj_t *)lv_event_get_target(e);
    int32_t val = lv_arc_get_value(arc);

    char buf[16];
    snprintf(buf, sizeof(buf), "%d C", (int)val);
    lv_label_set_text(ui_LabelTempValue, buf);
}

// ------------------------------------------------------------------------------
// Arduino Setup & Loop
// ------------------------------------------------------------------------------

void setup() {
    Serial.begin(115200);
    delay(200);
    Serial.println("\n[AMK_CYD_Universal_TFT] Starting VisualGUI_QuickStart...");

    // 1. Initialize display (auto-probes panel: ST7789, ILI9341, ST7796)
    tft.init();
    tft.setRotation(3);      // Standard Landscape (Rotation 3)
    tft.setBacklight(100);   // Default full brightness

    // 2. Initialize touch and sync rotation
    touch.begin(display);
    touch.setRotation(3);
    touch.loadCalibration(); // Loads calibrated coordinates from NVS flash

    // 3. Initialize LVGL bridge (Allocates DMA double buffers in internal SRAM)
    if (!lvgl.begin(display, touch)) {
        Serial.println("[ERROR] Failed to allocate LVGL DMA buffers!");
        while (1) delay(1000);
    }

    // 4. Initialize Visual Studio UI!
    ui_init();

    // Configure onboard RGB LED pin as output
    pinMode(display.redLedPin(), OUTPUT);
    digitalWrite(display.redLedPin(), HIGH); // Default OFF (active-low)

    Serial.println("[AMK_CYD_Universal_TFT] Visual GUI ready!");
}

void loop() {
    // Keep LVGL animations, timers, and touch events running smoothly
    lvgl.tick();
    delay(5);
}
