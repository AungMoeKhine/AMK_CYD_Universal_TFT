#pragma once
// ═══════════════════════════════════════════════════════════════════════════════
// AMK_CYD_LVGL.h - High-Speed Zero-Boilerplate LVGL Integration Helper
//                 for AMK_CYD_Universal_TFT (Supports LVGL v8.x and v9.x)
//
// Automatically manages:
//  1. Display flushing via hardware DMA / high-speed SPI (flushRegion + waitFlush)
//  2. Dynamic RGB565 byte-swapping (setSwapBytes)
//  3. Touch coordinate mapping & pressure detection (touch.read)
//  4. DMA-capable draw buffer allocation in ESP32 internal SRAM (Double-Buffered)
//  5. Automatic tick registration & timer handler
// ═══════════════════════════════════════════════════════════════════════════════

#include <Arduino.h>
#include "AMK_CYD_Universal_TFT.h"
#include <lvgl.h>

class AMK_CYD_LVGL {
private:
    AMK_CYD_display* _display = nullptr;
    AMK_CYD_touch*   _touch   = nullptr;

    // Buffer rows: 20 rows provides clean 12 slices for 240p (320x240)
    static const uint16_t BUFFER_ROWS = 20;
    void* _draw_buf_mem1 = nullptr;
    void* _draw_buf_mem2 = nullptr;

#if LVGL_VERSION_MAJOR >= 9
    lv_display_t* _disp  = nullptr;
    lv_indev_t*   _indev = nullptr;

    static void _disp_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
        AMK_CYD_LVGL* instance = (AMK_CYD_LVGL*)lv_display_get_user_data(disp);
        if (instance && instance->_display) {
            instance->_display->flushRegion(area->x1, area->y1, area->x2, area->y2, (uint16_t*)px_map);
            // Wait for DMA hardware to complete before releasing buffer back to LVGL
            instance->_display->waitFlush();
        }
        lv_display_flush_ready(disp);
    }

    static void _touch_read_cb(lv_indev_t *indev, lv_indev_data_t *data) {
        AMK_CYD_LVGL* instance = (AMK_CYD_LVGL*)lv_indev_get_user_data(indev);
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
#else
    lv_disp_draw_buf_t _draw_buf;
    lv_disp_drv_t      _disp_drv;
    lv_indev_drv_t     _indev_drv;

    static void _disp_flush_cb(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
        AMK_CYD_LVGL* instance = (AMK_CYD_LVGL*)disp->user_data;
        if (instance && instance->_display) {
            instance->_display->flushRegion(area->x1, area->y1, area->x2, area->y2, (uint16_t*)color_p);
            // Wait for DMA hardware to complete before releasing buffer back to LVGL
            instance->_display->waitFlush();
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
#endif

public:
    AMK_CYD_LVGL() = default;
    ~AMK_CYD_LVGL() {
        if (_draw_buf_mem1) {
            free(_draw_buf_mem1);
            _draw_buf_mem1 = nullptr;
        }
        if (_draw_buf_mem2) {
            free(_draw_buf_mem2);
            _draw_buf_mem2 = nullptr;
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
        size_t buf_pixels = (size_t)w * BUFFER_ROWS;
        size_t buf_bytes  = buf_pixels * sizeof(uint16_t);

        // Allocate DMA-capable draw buffer 1 in internal SRAM
        if (!_draw_buf_mem1) {
            _draw_buf_mem1 = heap_caps_malloc(buf_bytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT | MALLOC_CAP_DMA);
            if (!_draw_buf_mem1) {
                _draw_buf_mem1 = malloc(buf_bytes);
            }
        }
        if (!_draw_buf_mem1) return false;

        // Allocate DMA-capable draw buffer 2 for double buffering (smooth 60fps)
        if (!_draw_buf_mem2) {
            _draw_buf_mem2 = heap_caps_malloc(buf_bytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT | MALLOC_CAP_DMA);
        }

#if LVGL_VERSION_MAJOR >= 9
        // Set tick callback to millis()
        lv_tick_set_cb([]() -> uint32_t { return (uint32_t)millis(); });

        // Create LVGL 9 display
        _disp = lv_display_create(w, h);
        if (!_disp) return false;
        lv_display_set_user_data(_disp, this);
        lv_display_set_buffers(_disp, _draw_buf_mem1, _draw_buf_mem2, buf_bytes, LV_DISPLAY_RENDER_MODE_PARTIAL);
        lv_display_set_flush_cb(_disp, _disp_flush_cb);

        // Create LVGL 9 touch input device
        _indev = lv_indev_create();
        if (!_indev) return false;
        lv_indev_set_type(_indev, LV_INDEV_TYPE_POINTER);
        lv_indev_set_read_cb(_indev, _touch_read_cb);
        lv_indev_set_user_data(_indev, this);
#else
        // Initialize LVGL 8 draw buffer
        lv_disp_draw_buf_init(&_draw_buf, _draw_buf_mem1, _draw_buf_mem2, buf_pixels);

        // Register LVGL 8 display driver
        lv_disp_drv_init(&_disp_drv);
        _disp_drv.hor_res   = w;
        _disp_drv.ver_res   = h;
        _disp_drv.flush_cb  = _disp_flush_cb;
        _disp_drv.draw_buf  = &_draw_buf;
        _disp_drv.user_data = this;
        lv_disp_drv_register(&_disp_drv);

        // Register LVGL 8 touch input driver
        lv_indev_drv_init(&_indev_drv);
        _indev_drv.type      = LV_INDEV_TYPE_POINTER;
        _indev_drv.read_cb   = _touch_read_cb;
        _indev_drv.user_data = this;
        lv_indev_drv_register(&_indev_drv);
#endif

        return true;
    }

    void tick() {
#if LVGL_VERSION_MAJOR < 9
        static uint32_t last_tick = 0;
        uint32_t now = millis();
        if (last_tick == 0) last_tick = now;
        uint32_t elapsed = now - last_tick;
        if (elapsed > 0) {
            lv_tick_inc(elapsed);
            last_tick = now;
        }
#endif
        lv_timer_handler();
    }
};
