/*
 * chipguy_ESP32S3_Touch_LCD_7_dirtyrects - dirty-rectangle bookkeeping for the
 * 90/270-degree LVGL reconciliation path (see chipguy_ESP32S3_Touch_LCD_7_rotate.hpp).
 *
 * This header is intentionally PURE: it depends only on <stdint.h> and has no
 * Arduino/LVGL/hardware dependencies, so the rectangle math can be unit-tested
 * on a host with g++ (see extras/rect_test.cpp).
 *
 * Coordinates are inclusive [x1..x2] x [y1..y2], in the PHYSICAL (landscape,
 * row-major) framebuffer space.  A rectangle with x1>x2 or y1>y2 is empty.
 *
 * BOUNDED STORAGE, NO PRECISION LOSS:
 *   The list holds a fixed CG_LCD7_MAX_DIRTY_RECTS slots (no heap).  Subtraction
 *   fragments rectangles and can need more slots than it has; the danger is that
 *   collapsing the list to a bounding box on overflow would fill in the HOLES
 *   carved by subtract(), and the deferred copy would then repaint stale pixels
 *   over freshly drawn ones (the "trail" artifact).
 *
 *   To stay bounded WITHOUT losing holes, the list supports an "overflow sink":
 *   when it needs a slot and none is free, it hands the smallest rectangle to the
 *   sink, which executes that copy immediately, and drops it from the list.  A
 *   pending-copy that has actually been performed no longer needs tracking, so no
 *   information is lost -- the work is simply done eagerly instead of deferred.
 *   The renderer wires the sink to "copy this region from the scanned buffer into
 *   the buffer being built" (safe: source complete, dest not yet displayed).
 *
 *   If no sink is set (e.g. an add-only list with no holes, like the
 *   draw-this-frame record), overflow falls back to bounding-box coalescing,
 *   which for an add-only list is safe: it only ever yields a superset.
 *
 * Copyright (c) 2025 chipguyhere
 * MIT License
 */

#ifndef CHIPGUY_ESP32S3_TOUCH_LCD_7_DIRTYRECTS_H
#define CHIPGUY_ESP32S3_TOUCH_LCD_7_DIRTYRECTS_H

#include <stdint.h>

// Number of rectangle slots.  Fixed (no heap); the overflow sink keeps a
// hole-carrying list within this bound by executing the smallest pending copy
// immediately instead of growing.  Bump it to defer more copies before any
// eager flush kicks in.
#ifndef CG_LCD7_MAX_DIRTY_RECTS
#define CG_LCD7_MAX_DIRTY_RECTS 64
#endif

struct CGRect {
    int16_t x1, y1, x2, y2;   // inclusive
    bool empty() const { return x1 > x2 || y1 > y2; }
    // Area in pixels (0 if empty).  Used to pick the cheapest rect to flush.
    int32_t area() const {
        if (empty()) return 0;
        return (int32_t)(x2 - x1 + 1) * (int32_t)(y2 - y1 + 1);
    }
};

static inline int16_t cg_min16(int16_t a, int16_t b) { return a < b ? a : b; }
static inline int16_t cg_max16(int16_t a, int16_t b) { return a > b ? a : b; }

// Overflow sink: invoked with the rectangle the list is evicting so the caller
// can execute that copy immediately.  ctx is the caller's opaque pointer.
typedef void (*CGRectSink)(void* ctx, const CGRect& r);

class DirtyRectList {
public:
    DirtyRectList() : overflowCount(0), evictionCount(0),
                      _sink(nullptr), _sinkCtx(nullptr), _count(0) {}

    // Install the overflow sink.  With a sink set, overflow evicts (and executes)
    // the smallest rectangle instead of coalescing -- so holes are never lost.
    void setOverflowSink(CGRectSink fn, void* ctx) { _sink = fn; _sinkCtx = ctx; }

    void clear() { _count = 0; }
    int  count() const { return _count; }
    const CGRect& operator[](int i) const { return _rects[i]; }

    void copyFrom(const DirtyRectList& o) {
        _count = o._count;
        for (int i = 0; i < _count; i++) _rects[i] = o._rects[i];
        // Diagnostics and the sink belong to this list; don't copy them.
    }

    // Append a rectangle (ignored if empty).  On overflow, make room first --
    // by eager-flushing the smallest rect through the sink, or (no sink)
    // coalescing to a bounding box.
    void add(const CGRect& r) {
        if (r.empty()) return;
        if (_count >= CG_LCD7_MAX_DIRTY_RECTS) makeRoom();
        _rects[_count++] = r;
    }

    // Remove the area of `s` from every rectangle in the list, fragmenting as
    // needed.  Fragments favor full-width horizontal bands (long, contiguous,
    // forward row-major runs) over tall slivers -- the cheap shape to copy.
    void subtract(const CGRect& s) {
        if (s.empty() || _count == 0) return;
        DirtyRectList out;
        out._sink = _sink; out._sinkCtx = _sinkCtx;   // out stays bounded too
        for (int i = 0; i < _count; i++) subtractOne(_rects[i], s, out);
        // Carry the diagnostics forward, then take the fragmented result.
        out.overflowCount = overflowCount + out.overflowCount;
        out.evictionCount = evictionCount + out.evictionCount;
        uint32_t ov = out.overflowCount, ev = out.evictionCount;
        copyFrom(out);
        overflowCount = ov;
        evictionCount = ev;
    }

    uint32_t overflowCount;   // times the list was coalesced to a bbox (no sink)
    uint32_t evictionCount;   // times the sink eager-flushed the smallest rect

private:
    // Free up one slot.  Preferred: hand the smallest rect to the sink (it
    // executes that copy now) and drop it.  Fallback (no sink): coalesce to bbox.
    void makeRoom() {
        if (_sink) {
            int idx = smallestIndex();
            _sink(_sinkCtx, _rects[idx]);
            eraseAt(idx);
            evictionCount++;
        } else {
            coalesceToBBox();
        }
    }

    int smallestIndex() const {
        int best = 0;
        int32_t bestArea = _rects[0].area();
        for (int i = 1; i < _count; i++) {
            int32_t a = _rects[i].area();
            if (a < bestArea) { bestArea = a; best = i; }
        }
        return best;
    }

    void eraseAt(int i) {
        _rects[i] = _rects[_count - 1];   // order is irrelevant; swap with last
        _count--;
    }

    // P minus S -> up to 4 fragments, appended to `out`.  Horizontal-band
    // priority: top and bottom bands span P's full width; left/right are short.
    static void subtractOne(const CGRect& P, const CGRect& S, DirtyRectList& out) {
        int16_t ox1 = cg_max16(P.x1, S.x1);
        int16_t oy1 = cg_max16(P.y1, S.y1);
        int16_t ox2 = cg_min16(P.x2, S.x2);
        int16_t oy2 = cg_min16(P.y2, S.y2);
        if (ox1 > ox2 || oy1 > oy2) {           // no overlap: P survives whole
            out.add(P);
            return;
        }
        if (oy1 > P.y1) out.add({P.x1, P.y1, P.x2, (int16_t)(oy1 - 1)});  // top band
        if (oy2 < P.y2) out.add({P.x1, (int16_t)(oy2 + 1), P.x2, P.y2});  // bottom band
        if (ox1 > P.x1) out.add({P.x1, oy1, (int16_t)(ox1 - 1), oy2});    // left sliver
        if (ox2 < P.x2) out.add({(int16_t)(ox2 + 1), oy1, P.x2, oy2});    // right sliver
        // If S fully covers P, nothing is added (P removed).
    }

    // Coalesce the list to its bounding box.  Used only for sink-less (add-only)
    // lists, where a superset is harmless; never for a hole-carrying list.
    void coalesceToBBox() {
        if (_count == 0) return;
        CGRect b = _rects[0];
        for (int i = 1; i < _count; i++) {
            b.x1 = cg_min16(b.x1, _rects[i].x1);
            b.y1 = cg_min16(b.y1, _rects[i].y1);
            b.x2 = cg_max16(b.x2, _rects[i].x2);
            b.y2 = cg_max16(b.y2, _rects[i].y2);
        }
        _rects[0] = b;
        _count = 1;
        overflowCount++;
    }

    CGRect     _rects[CG_LCD7_MAX_DIRTY_RECTS];
    CGRectSink _sink;
    void*      _sinkCtx;
    int        _count;
};

#endif // CHIPGUY_ESP32S3_TOUCH_LCD_7_DIRTYRECTS_H
