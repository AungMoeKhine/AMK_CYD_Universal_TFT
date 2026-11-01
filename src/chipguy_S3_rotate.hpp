/*
 * chipguy_ESP32S3_Touch_LCD_7_rotate - 90/270-degree LVGL rendering for the
 * ESP32-S3-Touch-LCD-7, done at the LVGL->framebuffer level (the driver itself
 * keeps reading PSRAM forward at 0 degrees; see the README).
 *
 * LVGL runs in PARTIAL render mode with rotation set, so it hands us partial
 * areas + pixels in LOGICAL (portrait) coordinates; LvglRotatedRenderer rotates
 * each into one of the driver's two PHYSICAL (landscape) framebuffers and keeps
 * the two buffers consistent with a deferred dirty-rectangle scheme:
 *
 *   - flushRegion(): rotate-blit the area into the buffer we're building
 *     (FB[draw]); record it in `thisFrame`; and SUBTRACT it from `pending`
 *     (regions FB[draw] still lacks vs the scanned buffer) -- so pixels about to
 *     be overwritten next frame are never copied (e.g. while scrolling).
 *   - frameDone() (on LVGL's last flush of the frame): copy whatever remains in
 *     `pending` from the scanned buffer into FB[draw], swap buffers at vsync,
 *     then carry `thisFrame` forward as the new `pending`.
 *
 * Everything favors forward, long, sequential PSRAM access: the rotate-blit
 * writes each physical row as one contiguous run (reading the SRAM partial
 * buffer strided), and the reconciliation copy is row-major full-width memcpys.
 *
 * Copyright (c) 2025 chipguyhere
 * MIT License
 */

#pragma once

#include <string.h>
#include <lvgl.h>
#include "chipguy_S3_RGB_display.h"
#include "chipguy_S3_dirtyrects.h"

class LvglRotatedRenderer {
public:
    // rotation must be 90 or 270 (index aliases 1/3 are accepted).  The display
    // must already be begun at 0 degrees and have its second framebuffer
    // allocated.
    LvglRotatedRenderer(ESP32S3_Touch_LCD_7& d, int rotation)
        : display(d), _rotation(cglcd7_normalize_rotation((uint16_t)rotation)),
          _activeIndex(1), _drawIndex(2) {
        // When the pending list runs out of slots, flush the smallest pending
        // copy immediately (scanned buffer -> buffer being built) rather than
        // coalescing -- so subtracted holes are never lost.  _thisFrame keeps no
        // sink: it is add-only, so its bbox-coalesce fallback is harmless.
        _pending.setOverflowSink(&LvglRotatedRenderer::pendingSink, this);
    }

    // Rotate one LVGL partial area (logical/portrait) into FB[draw], record it,
    // and subtract it from the pending carry-over list.
    void flushRegion(const lv_area_t* area, uint8_t* px_map) {
        const uint16_t* src = (const uint16_t*)px_map;
        const int lx1 = area->x1, ly1 = area->y1, lx2 = area->x2, ly2 = area->y2;
        const int areaW = lx2 - lx1 + 1;
        const int Wl = display.height();   // logical width  (480)
        const int Hl = display.width();    // logical height (800)
        const int Wp = display.width();    // physical stride (800)
        uint16_t* fbDraw = fbPtr(_drawIndex);

        CGRect phys;
        if (_rotation == 90) {
            // physical (px,py) = (ly, Wl-1-lx).  Each logical column -> one
            // physical row, written forward over px (=ly); src read strided.
            for (int lx = lx1; lx <= lx2; lx++) {
                int py = Wl - 1 - lx;
                uint16_t* d = fbDraw + (size_t)py * Wp + ly1;     // px starts at ly1
                const uint16_t* s = src + (lx - lx1);             // ly==ly1 row
                for (int ly = ly1; ly <= ly2; ly++) {
                    *d++ = *s;
                    s += areaW;
                }
            }
            phys = { (int16_t)ly1, (int16_t)(Wl - 1 - lx2),
                     (int16_t)ly2, (int16_t)(Wl - 1 - lx1) };
        } else {  // 270
            // physical (px,py) = (Hl-1-ly, lx).  Written forward over px, so ly
            // descends (from ly2) and src is read strided backward.
            const int pxStart = Hl - 1 - ly2;
            const int pxEnd   = Hl - 1 - ly1;
            for (int lx = lx1; lx <= lx2; lx++) {
                int py = lx;
                uint16_t* d = fbDraw + (size_t)py * Wp + pxStart;
                const uint16_t* s = src + (size_t)(ly2 - ly1) * areaW + (lx - lx1); // ly==ly2 row
                for (int px = pxStart; px <= pxEnd; px++) {
                    *d++ = *s;
                    s -= areaW;
                }
            }
            phys = { (int16_t)(Hl - 1 - ly2), (int16_t)lx1,
                     (int16_t)(Hl - 1 - ly1), (int16_t)lx2 };
        }

        _thisFrame.add(phys);     // FB[draw] now has this region (freshly drawn)
        _pending.subtract(phys);  // ...so don't copy the old content there
    }

    // Called on LVGL's last flush of the frame: bring FB[draw] fully up to date,
    // reveal it at vsync, and roll the lists for the next frame.
    void frameDone() {
        // Bring FB[draw] up to date with everything still pending (whatever the
        // overflow sink didn't already flush eagerly during the frame).
        for (int i = 0; i < _pending.count(); i++) _copyRect(_pending[i]);

        display.setActiveFramebuffer(_drawIndex, /*wait*/true);   // tear-free swap

        int t = _activeIndex; _activeIndex = _drawIndex; _drawIndex = t;
        _pending.copyFrom(_thisFrame);   // next frame must reconcile these into the new draw buffer
        _thisFrame.clear();
    }

    // Diagnostic: times the pending list was coalesced to a bbox.  With the
    // overflow sink installed this must stay 0 (coalescing a hole-carrying list
    // would lose holes); it is the canary for the old trail bug.
    uint32_t overflowCount() const { return _pending.overflowCount; }

    // Diagnostic: times the list ran out of slots and eager-flushed the smallest
    // pending copy instead of deferring it.  Nonzero just means heavy churn; it
    // is correct (the work is done now rather than at frameDone), not an error.
    uint32_t evictionCount() const { return _pending.evictionCount; }

    // Diagnostics (bring-up): which framebuffer the panel scans vs we build.
    int debugActiveIndex() const { return _activeIndex; }
    int debugDrawIndex() const { return _drawIndex; }

private:
    uint16_t* fbPtr(int idx) {
        return idx == 1 ? display.getFramebuffer() : display.getFramebuffer2();
    }

    // Copy one physical rectangle from the scanned buffer (complete) into the
    // buffer being built (not yet displayed) as forward, full-width row-major
    // runs.  Used both by frameDone()'s deferred reconcile and by the overflow
    // sink's eager flush -- both copy active -> draw, which is always safe.
    void _copyRect(const CGRect& r) {
        uint16_t* srcFb = fbPtr(_activeIndex);
        uint16_t* dstFb = fbPtr(_drawIndex);
        const int Wp = display.width();
        const size_t bytes = (size_t)(r.x2 - r.x1 + 1) * sizeof(uint16_t);
        for (int y = r.y1; y <= r.y2; y++)
            memcpy(dstFb + (size_t)y * Wp + r.x1,
                   srcFb + (size_t)y * Wp + r.x1, bytes);
    }

    // Overflow sink trampoline: the pending list calls this with the smallest
    // rectangle when it needs a free slot; we execute that copy immediately so
    // the obligation is met and the rect can be dropped without losing a hole.
    static void pendingSink(void* ctx, const CGRect& r) {
        ((LvglRotatedRenderer*)ctx)->_copyRect(r);
    }

    ESP32S3_Touch_LCD_7& display;
    int _rotation;          // 90 or 270
    int _activeIndex;       // framebuffer the panel is scanning (1 or 2)
    int _drawIndex;         // framebuffer LVGL is building this frame
    DirtyRectList _pending; // regions FB[draw] lacks vs FB[active]
    DirtyRectList _thisFrame;// regions drawn this frame (becomes next pending)
};
