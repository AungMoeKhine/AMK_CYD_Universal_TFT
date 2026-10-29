/*
 * LVGL_QuickStart.ino - AMK_CYD_Universal_TFT
 * Minimal boilerplate demonstrating LVGL integration in under 5 lines!
 *
 * Requirements:
 * Install the "lvgl" library (v8.x or v9.x) via Arduino IDE Library Manager.
 */

#include <Arduino.h>
#include <AMK_CYD_Universal_TFT.h>
#include <AMK_CYD_LVGL.h>

AMK_CYD_display display;
AMK_CYD_touch   touch;
TFT_eSPI        tft;
AMK_CYD_LVGL    lvgl;

static void btn_event_cb(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * btn = (lv_obj_t *)lv_event_get_target(e);
    if (code == LV_EVENT_CLICKED) {
        static uint8_t count = 0;
        count++;
        lv_obj_t * label = lv_obj_get_child(btn, 0);
        lv_label_set_text_fmt(label, "Clicks: %d", count);
    }
}

void setup() {
    Serial.begin(115200);
    delay(100);
    Serial.println("\n[AMK_CYD_Universal_TFT] Starting LVGL_QuickStart...");

    // 1. Initialize Display & Touch (Rotation 3: Landscape, USB left, SD top)
    tft.init();
    tft.setRotation(3);
    tft.setBacklight(100);

    touch.begin(display);
    touch.setRotation(3);
    touch.loadCalibration();
    touch.loadFilters();

    // 2. Initialize LVGL in 1 LINE!
    if (!lvgl.begin(display, touch)) {
        Serial.println("LVGL allocation failed!");
        return;
    }

    // 3. Build UI with standard LVGL widgets:
    // Header Label
    lv_obj_t * title = lv_label_create(lv_scr_act());
    lv_label_set_text(title, "AMK_CYD Universal - LVGL Demo");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 15);

    // Interactive Button
    lv_obj_t * btn = lv_btn_create(lv_scr_act());
    lv_obj_align(btn, LV_ALIGN_CENTER, 0, -10);
    lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_ALL, NULL);

    lv_obj_t * btn_lbl = lv_label_create(btn);
    lv_label_set_text(btn_lbl, "Click Me!");
    lv_obj_center(btn_lbl);

    // Interactive Slider Widget
    lv_obj_t * slider = lv_slider_create(lv_scr_act());
    lv_obj_align(slider, LV_ALIGN_BOTTOM_MID, 0, -25);
    lv_slider_set_value(slider, 50, LV_ANIM_OFF);
}

void loop() {
    lvgl.tick(); // Process LVGL timers, animations & touch events
    delay(5);
}
