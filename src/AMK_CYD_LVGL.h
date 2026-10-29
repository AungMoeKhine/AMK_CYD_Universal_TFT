#pragma once
// ═══════════════════════════════════════════════════════════════════════════════
// AMK_CYD_LVGL.h - High-Speed Zero-Boilerplate LVGL Integration Helper
//                 for AMK_CYD_Universal_TFT (Supports LVGL v8.x)
//
// Automatically manages:
//  1. Display flushing via hardware DMA / high-speed SPI (flushRegion)
//  2. Dynamic RGB565 byte-swapping (setSwapBytes)
//  3. Touch coordinate mapping & pressure detection (touch.read)
//  4. DMA-capable draw buffer allocation in ESP32 internal SRAM
// ═══════════════════════════════════════════════════════════════════════════════

#include <Arduino.h>
#include "AMK_CYD_Universal_TFT.h"
#include <lvgl.h>

class AMK_CYD_LVGL {
private:
    AMK_CYD_display* _display = nullptr;
    AMK_CYD_touch*   _touch   = nullptr;

    // 1/10th of screen height buffer (~15 KB RAM): fast DMA, low memory footprint
    static const uint16_t BUFFER_ROWS = 24;
    lv_color_t*        _buf1 = nullptr;
    lv_disp_draw_buf_t _draw_buf;
    lv_disp_drv_t      _disp_drv;
    lv_indev_drv_t     _indev_drv;

    static void _disp_flush_cb(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
        AMK_CYD_LVGL* instance = (AMK_CYD_LVGL*)disp->user_data;
        if (instance && instance->_display) {
            uint32_t w = (area->x2 - area->x1 + 1);
            uint32_t h = (area->y2 - area->y1 + 1);
            instance->_display->flushRegion(area->x1, area->y1, w, h, (const uint16_t*)color_p);
        }
        lv_disp_flush_ready(disp);
    }

    static void _touch_read_cb(lv_indev_drv_t *indev, lv_indev_data_t *data) {
        AMK_CYD_LVGL* instance = (AMK_CYD_LVGL*)indev->user_data;
        if (instance && instance->_touch) {
            uint16_t x = 0, y = 0;
            if (instance->_touch->read(x, y)) {
                data->state = LV_INDEV_STATE_PR;
                data->point.x = x;
                data->point.y = y;
            } else {
                data->state = LV_INDEV_STATE_REL;
            }
        }
    }

public:
    AMK_CYD_LVGL() = default;
    ~AMK_CYD_LVGL() {
        if (_buf1) {
            free(_buf1);
            _buf1 = nullptr;
        }
    }

    bool begin(AMK_CYD_display& display, AMK_CYD_touch& touch) {
        _display = &display;
        _touch   = &touch;

        // Initialize core LVGL engine
        lv_init();

        // Enable RGB565 byte-swapping for native LVGL pixel endianness
        _display->setSwapBytes(true);

        uint16_t w = _display->width();
        uint16_t h = _display->height();

        // Allocate DMA-capable draw buffer in internal SRAM
        size_t buf_size = w * BUFFER_ROWS;
        if (!_buf1) {
            _buf1 = (lv_color_t*)heap_caps_malloc(buf_size * sizeof(lv_color_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
            if (!_buf1) {
                _buf1 = (lv_color_t*)malloc(buf_size * sizeof(lv_color_t));
            }
        }
        if (!_buf1) return false;

        // Initialize LVGL draw buffer
        lv_disp_draw_buf_init(&_draw_buf, _buf1, NULL, buf_size);

        // Register display driver
        lv_disp_drv_init(&_disp_drv);
        _disp_drv.hor_res   = w;
        _disp_drv.ver_res   = h;
        _disp_drv.flush_cb  = _disp_flush_cb;
        _disp_drv.draw_buf  = &_draw_buf;
        _disp_drv.user_data = this;
        lv_disp_drv_register(&_disp_drv);

        // Register touch input driver
        lv_indev_drv_init(&_indev_drv);
        _indev_drv.type      = LV_INDEV_TYPE_POINTER;
        _indev_drv.read_cb   = _touch_read_cb;
        _indev_drv.user_data = this;
        lv_indev_drv_register(&_indev_drv);

        return true;
    }

    void tick() {
        lv_timer_handler();
    }
};
