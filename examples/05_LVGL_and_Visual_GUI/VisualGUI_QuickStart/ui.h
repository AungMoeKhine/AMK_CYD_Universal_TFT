#ifndef _SQUARELINE_UI_H
#define _SQUARELINE_UI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

// Screen: ui_ScreenMain
extern lv_obj_t * ui_ScreenMain;
extern lv_obj_t * ui_HeaderPanel;
extern lv_obj_t * ui_TitleLabel;
extern lv_obj_t * ui_BadgeLabel;

// Temperature Arc Widget
extern lv_obj_t * ui_ArcTemp;
extern lv_obj_t * ui_LabelTempTitle;
extern lv_obj_t * ui_LabelTempValue;

// Backlight Brightness Slider
extern lv_obj_t * ui_SliderBright;
extern lv_obj_t * ui_LabelBrightTitle;
extern lv_obj_t * ui_LabelBrightValue;

// Onboard LED Switch
extern lv_obj_t * ui_SwitchLed;
extern lv_obj_t * ui_LabelLed;

// Fan / Action Button
extern lv_obj_t * ui_BtnAction;
extern lv_obj_t * ui_LabelBtnAction;

// Event functions (custom hardware callbacks)
void ui_event_slider_brightness(lv_event_t * e);
void ui_event_switch_led(lv_event_t * e);
void ui_event_btn_action(lv_event_t * e);
void ui_event_arc_temp(lv_event_t * e);

// Initialize all UI components
void ui_init(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
