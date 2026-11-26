// SquareLine Studio UI Implementation for AMK_CYD_Universal_TFT
// Auto-generated style structure compatible with LVGL v8.x and v9.x

#include "ui.h"
#include <stdio.h>

// Screen & Objects
lv_obj_t * ui_ScreenMain        = NULL;
lv_obj_t * ui_HeaderPanel       = NULL;
lv_obj_t * ui_TitleLabel        = NULL;
lv_obj_t * ui_BadgeLabel        = NULL;
lv_obj_t * ui_ArcTemp           = NULL;
lv_obj_t * ui_LabelTempTitle    = NULL;
lv_obj_t * ui_LabelTempValue    = NULL;
lv_obj_t * ui_SliderBright      = NULL;
lv_obj_t * ui_LabelBrightTitle  = NULL;
lv_obj_t * ui_LabelBrightValue  = NULL;
lv_obj_t * ui_SwitchLed         = NULL;
lv_obj_t * ui_LabelLed          = NULL;
lv_obj_t * ui_BtnAction         = NULL;
lv_obj_t * ui_LabelBtnAction    = NULL;

void ui_ScreenMain_screen_init(void) {
    // 1. Create main screen (320x240 dark slate theme)
    ui_ScreenMain = lv_obj_create(NULL);
    lv_obj_remove_flag(ui_ScreenMain, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(ui_ScreenMain, lv_color_hex(0x0F172A), LV_PART_MAIN | LV_STATE_DEFAULT); // Slate 900
    lv_obj_set_style_bg_opa(ui_ScreenMain, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    // 2. Header Panel (Title bar)
    ui_HeaderPanel = lv_obj_create(ui_ScreenMain);
    lv_obj_set_size(ui_HeaderPanel, 320, 36);
    lv_obj_set_pos(ui_HeaderPanel, 0, 0);
    lv_obj_remove_flag(ui_HeaderPanel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(ui_HeaderPanel, lv_color_hex(0x1E293B), LV_PART_MAIN | LV_STATE_DEFAULT); // Slate 800
    lv_obj_set_style_border_side(ui_HeaderPanel, LV_BORDER_SIDE_BOTTOM, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui_HeaderPanel, lv_color_hex(0x334155), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_HeaderPanel, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui_HeaderPanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Title Label
    ui_TitleLabel = lv_label_create(ui_HeaderPanel);
    lv_obj_set_pos(ui_TitleLabel, 10, 4);
    lv_label_set_text(ui_TitleLabel, "CYD Smart Dashboard");
    lv_obj_set_style_text_color(ui_TitleLabel, lv_color_hex(0xF8FAFC), LV_PART_MAIN | LV_STATE_DEFAULT);

    // Badge Label (Online)
    ui_BadgeLabel = lv_label_create(ui_HeaderPanel);
    lv_obj_align(ui_BadgeLabel, LV_ALIGN_RIGHT_MID, -12, 0);
    lv_label_set_text(ui_BadgeLabel, "LVGL LIVE");
    lv_obj_set_style_text_color(ui_BadgeLabel, lv_color_hex(0x22C55E), LV_PART_MAIN | LV_STATE_DEFAULT); // Green 500

    // -------------------------------------------------------------
    // LEFT COLUMN: Temperature Arc Gauge
    // -------------------------------------------------------------
    ui_ArcTemp = lv_arc_create(ui_ScreenMain);
    lv_obj_set_size(ui_ArcTemp, 110, 110);
    lv_obj_set_pos(ui_ArcTemp, 14, 46);
    lv_arc_set_range(ui_ArcTemp, 15, 35);
    lv_arc_set_value(ui_ArcTemp, 24);
    lv_arc_set_bg_angles(ui_ArcTemp, 135, 45);
    lv_obj_set_style_arc_color(ui_ArcTemp, lv_color_hex(0x334155), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_arc_color(ui_ArcTemp, lv_color_hex(0x38BDF8), LV_PART_INDICATOR | LV_STATE_DEFAULT); // Sky 400
    lv_obj_set_style_arc_width(ui_ArcTemp, 9, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_arc_width(ui_ArcTemp, 9, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(ui_ArcTemp, ui_event_arc_temp, LV_EVENT_VALUE_CHANGED, NULL);

    // Inside Arc Labels
    ui_LabelTempTitle = lv_label_create(ui_ArcTemp);
    lv_obj_align(ui_LabelTempTitle, LV_ALIGN_CENTER, 0, -10);
    lv_label_set_text(ui_LabelTempTitle, "ROOM");
    lv_obj_set_style_text_color(ui_LabelTempTitle, lv_color_hex(0x94A3B8), LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_LabelTempValue = lv_label_create(ui_ArcTemp);
    lv_obj_align(ui_LabelTempValue, LV_ALIGN_CENTER, 0, 10);
    lv_label_set_text(ui_LabelTempValue, "24 C");
    lv_obj_set_style_text_color(ui_LabelTempValue, lv_color_hex(0xF8FAFC), LV_PART_MAIN | LV_STATE_DEFAULT);

    // -------------------------------------------------------------
    // RIGHT COLUMN: Controls (Switch & Action Button)
    // -------------------------------------------------------------

    // Onboard LED Switch Container
    lv_obj_t * pnl_switch = lv_obj_create(ui_ScreenMain);
    lv_obj_set_size(pnl_switch, 165, 44);
    lv_obj_set_pos(pnl_switch, 140, 48);
    lv_obj_remove_flag(pnl_switch, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(pnl_switch, lv_color_hex(0x1E293B), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(pnl_switch, lv_color_hex(0x334155), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(pnl_switch, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(pnl_switch, 8, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_LabelLed = lv_label_create(pnl_switch);
    lv_obj_align(ui_LabelLed, LV_ALIGN_LEFT_MID, 6, 0);
    lv_label_set_text(ui_LabelLed, "RGB LED");
    lv_obj_set_style_text_color(ui_LabelLed, lv_color_hex(0xE2E8F0), LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_SwitchLed = lv_switch_create(pnl_switch);
    lv_obj_align(ui_SwitchLed, LV_ALIGN_RIGHT_MID, -4, 0);
    lv_obj_add_event_cb(ui_SwitchLed, ui_event_switch_led, LV_EVENT_VALUE_CHANGED, NULL);

    // Action Button
    ui_BtnAction = lv_btn_create(ui_ScreenMain);
    lv_obj_set_size(ui_BtnAction, 165, 44);
    lv_obj_set_pos(ui_BtnAction, 140, 102);
    lv_obj_set_style_bg_color(ui_BtnAction, lv_color_hex(0x2563EB), LV_PART_MAIN | LV_STATE_DEFAULT); // Blue 600
    lv_obj_set_style_radius(ui_BtnAction, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(ui_BtnAction, ui_event_btn_action, LV_EVENT_CLICKED, NULL);

    ui_LabelBtnAction = lv_label_create(ui_BtnAction);
    lv_obj_center(ui_LabelBtnAction);
    lv_label_set_text(ui_LabelBtnAction, "BEEP / BZZ");
    lv_obj_set_style_text_color(ui_LabelBtnAction, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);

    // -------------------------------------------------------------
    // BOTTOM: Backlight Brightness Slider (Controls CYD Hardware!)
    // -------------------------------------------------------------
    lv_obj_t * pnl_slider = lv_obj_create(ui_ScreenMain);
    lv_obj_set_size(pnl_slider, 292, 64);
    lv_obj_set_pos(pnl_slider, 14, 162);
    lv_obj_remove_flag(pnl_slider, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(pnl_slider, lv_color_hex(0x1E293B), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(pnl_slider, lv_color_hex(0x334155), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(pnl_slider, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(pnl_slider, 8, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_LabelBrightTitle = lv_label_create(pnl_slider);
    lv_obj_set_pos(ui_LabelBrightTitle, 6, 2);
    lv_label_set_text(ui_LabelBrightTitle, "Screen Brightness");
    lv_obj_set_style_text_color(ui_LabelBrightTitle, lv_color_hex(0x94A3B8), LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_LabelBrightValue = lv_label_create(pnl_slider);
    lv_obj_align(ui_LabelBrightValue, LV_ALIGN_TOP_RIGHT, -6, 2);
    lv_label_set_text(ui_LabelBrightValue, "100%");
    lv_obj_set_style_text_color(ui_LabelBrightValue, lv_color_hex(0x38BDF8), LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_SliderBright = lv_slider_create(pnl_slider);
    lv_obj_set_size(ui_SliderBright, 260, 10);
    lv_obj_align(ui_SliderBright, LV_ALIGN_BOTTOM_MID, 0, -6);
    lv_slider_set_range(ui_SliderBright, 10, 100);
    lv_slider_set_value(ui_SliderBright, 100, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(ui_SliderBright, lv_color_hex(0x334155), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_SliderBright, lv_color_hex(0x38BDF8), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_SliderBright, lv_color_hex(0x0284C7), LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(ui_SliderBright, ui_event_slider_brightness, LV_EVENT_VALUE_CHANGED, NULL);
}

void ui_init(void) {
    ui_ScreenMain_screen_init();
#if LVGL_VERSION_MAJOR >= 9
    lv_screen_load(ui_ScreenMain);
#else
    lv_disp_load_scr(ui_ScreenMain);
#endif
}
