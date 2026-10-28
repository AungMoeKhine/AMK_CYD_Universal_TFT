#pragma once
// ═══════════════════════════════════════════════════════════════════════════════
// AMK_CYD_Universal_TFT.h - Universal standalone TFT, Touch & Sprite driver
//                          for all ESP32 Cheap Yellow Display (CYD) resistive boards
//
// Usage in sketch:
//     #include <AMK_CYD_Universal_TFT.h>
//
// The sketch must declare these globals:
//     chipguy_CYD_resistive_display  display;
//     chipguy_CYD_resistive_touch    touch;
//     TFT_eSPI                       tft;
// ═══════════════════════════════════════════════════════════════════════════════

#include <Arduino.h>
#include <SPI.h>
#include <math.h>
#include <algorithm>
#include "chipguy_CYD_resistive_display.h"
#include "chipguy_CYD_resistive_touch.h"

#ifndef pgm_read_ptr
#define pgm_read_ptr(addr) (*(const void **)(addr))
#endif

#ifndef LOAD_GFXFF
#define LOAD_GFXFF
#endif

#include "Fonts/GFXFF/gfxfont.h"
#include "Free_Fonts.h"

// ─── Type Aliases for Backward & Universal Compatibility ──────────────────────
using AMK_CYD_display = chipguy_CYD_resistive_display;
using AMK_CYD_touch   = chipguy_CYD_resistive_touch;

// ─── Color constants (RGB565) ────────────────────────────────────────────────
#ifndef TFT_BLACK
#define TFT_BLACK        0x0000
#define TFT_NAVY         0x000F
#define TFT_DARKGREEN    0x03E0
#define TFT_DARKCYAN     0x03EF
#define TFT_MAROON       0x7800
#define TFT_PURPLE       0x780F
#define TFT_OLIVE        0x7BE0
#define TFT_LIGHTGREY    0xC618
#define TFT_DARKGREY     0x7BEF
#define TFT_BLUE         0x001F
#define TFT_GREEN        0x07E0
#define TFT_CYAN         0x07FF
#define TFT_RED          0xF800
#define TFT_MAGENTA      0xF81F
#define TFT_YELLOW       0xFFE0
#define TFT_WHITE        0xFFFF
#define TFT_ORANGE       0xFD20
#define TFT_GREENYELLOW  0xAFE5
#define TFT_PINK         0xFC18
#endif

#ifndef TFT_TRANSPARENT
#define TFT_TRANSPARENT  0x0120
#endif

// ─── Text datum constants ────────────────────────────────────────────────────
#ifndef TL_DATUM
#define TL_DATUM 0
#define TC_DATUM 1
#define TR_DATUM 2
#define ML_DATUM 3
#define CL_DATUM 3
#define MC_DATUM 4
#define CC_DATUM 4
#define MR_DATUM 5
#define CR_DATUM 5
#define BL_DATUM 6
#define BC_DATUM 7
#define BR_DATUM 8
#endif

// ═══════════════════════════════════════════════════════════════════════════════
//  Embedded 5x7 bitmap font  (ASCII 32-127, 5 bytes per glyph)
//  Each byte = one column; bit 0 = top row, bit 6 = bottom row.
// ═══════════════════════════════════════════════════════════════════════════════
static const uint8_t _amk_font5x7[] PROGMEM = {
    0x00,0x00,0x00,0x00,0x00, // 32  (space)
    0x00,0x00,0x5F,0x00,0x00, // 33  !
    0x00,0x07,0x00,0x07,0x00, // 34  "
    0x14,0x7F,0x14,0x7F,0x14, // 35  #
    0x24,0x2A,0x7F,0x2A,0x12, // 36  $
    0x23,0x13,0x08,0x64,0x62, // 37  %
    0x36,0x49,0x55,0x22,0x50, // 38  &
    0x00,0x05,0x03,0x00,0x00, // 39  '
    0x00,0x1C,0x22,0x41,0x00, // 40  (
    0x00,0x41,0x22,0x1C,0x00, // 41  )
    0x14,0x08,0x3E,0x08,0x14, // 42  *
    0x08,0x08,0x3E,0x08,0x08, // 43  +
    0x00,0x50,0x30,0x00,0x00, // 44  ,
    0x08,0x08,0x08,0x08,0x08, // 45  -
    0x00,0x60,0x60,0x00,0x00, // 46  .
    0x20,0x10,0x08,0x04,0x02, // 47  /
    0x3E,0x51,0x49,0x45,0x3E, // 48  0
    0x00,0x42,0x7F,0x40,0x00, // 49  1
    0x42,0x61,0x51,0x49,0x46, // 50  2
    0x21,0x41,0x45,0x4B,0x31, // 51  3
    0x18,0x14,0x12,0x7F,0x10, // 52  4
    0x27,0x45,0x45,0x45,0x39, // 53  5
    0x3C,0x4A,0x49,0x49,0x30, // 54  6
    0x01,0x71,0x09,0x05,0x03, // 55  7
    0x36,0x49,0x49,0x49,0x36, // 56  8
    0x06,0x49,0x49,0x29,0x1E, // 57  9
    0x00,0x36,0x36,0x00,0x00, // 58  :
    0x00,0x56,0x36,0x00,0x00, // 59  ;
    0x08,0x14,0x22,0x41,0x00, // 60  <
    0x14,0x14,0x14,0x14,0x14, // 61  =
    0x00,0x41,0x22,0x14,0x08, // 62  >
    0x02,0x01,0x51,0x09,0x06, // 63  ?
    0x32,0x49,0x79,0x41,0x3E, // 64  @
    0x7E,0x11,0x11,0x11,0x7E, // 65  A
    0x7F,0x49,0x49,0x49,0x36, // 66  B
    0x3E,0x41,0x41,0x41,0x22, // 67  C
    0x7F,0x41,0x41,0x22,0x1C, // 68  D
    0x7F,0x49,0x49,0x49,0x41, // 69  E
    0x7F,0x09,0x09,0x09,0x01, // 70  F
    0x3E,0x41,0x49,0x49,0x7A, // 71  G
    0x7F,0x08,0x08,0x08,0x7F, // 72  H
    0x00,0x41,0x7F,0x41,0x00, // 73  I
    0x20,0x40,0x41,0x3F,0x01, // 74  J
    0x7F,0x08,0x14,0x22,0x41, // 75  K
    0x7F,0x40,0x40,0x40,0x40, // 76  L
    0x7F,0x02,0x0C,0x02,0x7F, // 77  M
    0x7F,0x04,0x08,0x10,0x7F, // 78  N
    0x3E,0x41,0x41,0x41,0x3E, // 79  O
    0x7F,0x09,0x09,0x09,0x06, // 80  P
    0x3E,0x41,0x51,0x21,0x5E, // 81  Q
    0x7F,0x09,0x19,0x29,0x46, // 82  R
    0x46,0x49,0x49,0x49,0x31, // 83  S
    0x01,0x01,0x7F,0x01,0x01, // 84  T
    0x3F,0x40,0x40,0x40,0x3F, // 85  U
    0x1F,0x20,0x40,0x20,0x1F, // 86  V
    0x3F,0x40,0x38,0x40,0x3F, // 87  W
    0x63,0x14,0x08,0x14,0x63, // 88  X
    0x07,0x08,0x70,0x08,0x07, // 89  Y
    0x61,0x51,0x49,0x45,0x43, // 90  Z
    0x00,0x7F,0x41,0x41,0x00, // 91  [
    0x02,0x04,0x08,0x10,0x20, // 92  backslash
    0x00,0x41,0x41,0x7F,0x00, // 93  ]
    0x04,0x02,0x01,0x02,0x04, // 94  ^
    0x40,0x40,0x40,0x40,0x40, // 95  _
    0x00,0x01,0x02,0x04,0x00, // 96  `
    0x20,0x54,0x54,0x54,0x78, // 97  a
    0x7F,0x48,0x44,0x44,0x38, // 98  b
    0x38,0x44,0x44,0x44,0x20, // 99  c
    0x38,0x44,0x44,0x48,0x7F, // 100 d
    0x38,0x54,0x54,0x54,0x18, // 101 e
    0x08,0x7E,0x09,0x01,0x02, // 102 f
    0x0C,0x52,0x52,0x52,0x3E, // 103 g
    0x7F,0x08,0x04,0x04,0x78, // 104 h
    0x00,0x44,0x7D,0x40,0x00, // 105 i
    0x20,0x40,0x44,0x3D,0x00, // 106 j
    0x7F,0x10,0x28,0x44,0x00, // 107 k
    0x00,0x41,0x7F,0x40,0x00, // 108 l
    0x7C,0x04,0x18,0x04,0x78, // 109 m
    0x7C,0x08,0x04,0x04,0x78, // 110 n
    0x38,0x44,0x44,0x44,0x38, // 111 o
    0x7C,0x14,0x14,0x14,0x08, // 112 p
    0x08,0x14,0x14,0x18,0x7C, // 113 q
    0x7C,0x08,0x04,0x04,0x08, // 114 r
    0x48,0x54,0x54,0x54,0x20, // 115 s
    0x04,0x3F,0x44,0x40,0x20, // 116 t
    0x3C,0x40,0x40,0x20,0x7C, // 117 u
    0x1C,0x20,0x40,0x20,0x1C, // 118 v
    0x3C,0x40,0x30,0x40,0x3C, // 119 w
    0x44,0x28,0x10,0x28,0x44, // 120 x
    0x0C,0x50,0x50,0x50,0x3C, // 121 y
    0x44,0x64,0x54,0x4C,0x44, // 122 z
    0x00,0x08,0x36,0x41,0x00, // 123 {
    0x00,0x00,0x7F,0x00,0x00, // 124 |
    0x00,0x41,0x36,0x08,0x00, // 125 }
    0x10,0x08,0x08,0x10,0x08, // 126 ~
    0x78,0x46,0x41,0x46,0x78, // 127 DEL
};

// ═══════════════════════════════════════════════════════════════════════════════
//  The sketch provides the display and touch objects. We reference them via
//  extern so that tft.init() and touch.begin(display) use the SAME physical object.
// ═══════════════════════════════════════════════════════════════════════════════
extern chipguy_CYD_resistive_display display;
#ifndef AMK_CYD_NO_GLOBAL_TOUCH
extern chipguy_CYD_resistive_touch   touch;
#endif

class TFT_eSprite;

// ═══════════════════════════════════════════════════════════════════════════════
//  TFT_eSPI  -  Base display class
// ═══════════════════════════════════════════════════════════════════════════════
class TFT_eSPI : public Print {
protected:
    uint8_t  _textDatum   = TL_DATUM;
    uint8_t  _textSize    = 1;
    uint16_t _textFg      = TFT_WHITE;
    uint16_t _textBg      = TFT_BLACK;
    bool     _swapBytes   = false;
    int16_t  _cursorX     = 0;
    int16_t  _cursorY     = 0;
    bool     _textWrap    = true;
    uint16_t _textPadding = 0;
    int16_t  _pivotX      = 0;
    int16_t  _pivotY      = 0;
    const GFXfont* _gfxFont = nullptr;
    int8_t   _glyph_ab    = 0;
    int8_t   _glyph_bb    = 0;

    virtual void _pixel(int32_t x, int32_t y, uint16_t color) {
        if (x < 0 || y < 0 || x >= display.width() || y >= display.height()) return;
        alignas(4) static uint16_t _px;
        display.waitFlush();
        _px = __builtin_bswap16(color);
        display.flushRegion(x, y, x, y, &_px);
    }

    virtual void _fill(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color) {
        if (w <= 0 || h <= 0) return;
        int16_t sw = display.width(), sh = display.height();
        if (x < 0) { w += x; x = 0; }
        if (y < 0) { h += y; y = 0; }
        if (x + w > sw) w = sw - x;
        if (y + h > sh) h = sh - y;
        if (w <= 0 || h <= 0) return;

        alignas(4) static uint16_t _lineBuf[480];
        uint16_t be = __builtin_bswap16(color);
        for (int32_t row = 0; row < h; row++) {
            display.waitFlush();
            for (int32_t i = 0; i < w; i++) _lineBuf[i] = be;
            display.flushRegion(x, y + row, x + w - 1, y + row, _lineBuf);
        }
        display.waitFlush();
    }

    virtual void _drawChar(int32_t x, int32_t y, unsigned char c,
                           uint16_t fg, uint16_t bg, uint8_t scale) {
        if (c < 32 || c > 127) c = '?';
        const uint8_t* glyph = &_amk_font5x7[(c - 32) * 5];
        int16_t cw = 6 * scale;
        int16_t ch = 8 * scale;

        int16_t maxW = width();
        int16_t maxH = height();
        if (x + cw <= 0 || y + ch <= 0 || x >= maxW || y >= maxH) return;

        bool transparent = (fg == bg) || (bg == TFT_TRANSPARENT);

        if (!isSprite()) {
            if (transparent) {
                for (int col = 0; col < 5; col++) {
                    uint8_t line = pgm_read_byte(&glyph[col]);
                    for (int row = 0; row < 8; row++) {
                        if (line & (1 << row)) {
                            _fill(x + col * scale, y + row * scale, scale, scale, fg);
                        }
                    }
                }
                return;
            }

            if (scale <= 3) {
                alignas(4) static uint16_t _charBuf[18 * 24];
                uint16_t beFg = __builtin_bswap16(fg);
                uint16_t beBg = __builtin_bswap16(bg);
                for (int row = 0; row < 8; row++) {
                    for (int col = 0; col < 6; col++) {
                        uint16_t clr;
                        if (col < 5) {
                            uint8_t line = pgm_read_byte(&glyph[col]);
                            clr = (line & (1 << row)) ? beFg : beBg;
                        } else {
                            clr = beBg;
                        }
                        for (int sy = 0; sy < scale; sy++)
                            for (int sx = 0; sx < scale; sx++)
                                _charBuf[(row * scale + sy) * cw + col * scale + sx] = clr;
                    }
                }
                display.waitFlush();
                display.flushRegion(x, y, x + cw - 1, y + ch - 1, _charBuf);
                display.waitFlush();
            } else {
                for (int col = 0; col < 5; col++) {
                    uint8_t line = pgm_read_byte(&glyph[col]);
                    for (int row = 0; row < 8; row++) {
                        uint16_t clr = (line & (1 << row)) ? fg : bg;
                        _fill(x + col * scale, y + row * scale, scale, scale, clr);
                    }
                }
                _fill(x + 5 * scale, y, scale, ch, bg);
            }
        } else {
            if (transparent) {
                for (int col = 0; col < 5; col++) {
                    uint8_t line = pgm_read_byte(&glyph[col]);
                    for (int row = 0; row < 8; row++) {
                        if (line & (1 << row)) {
                            if (scale == 1) _pixel(x + col, y + row, fg);
                            else _fill(x + col * scale, y + row * scale, scale, scale, fg);
                        }
                    }
                }
                return;
            }

            for (int col = 0; col < 5; col++) {
                uint8_t line = pgm_read_byte(&glyph[col]);
                for (int row = 0; row < 8; row++) {
                    uint16_t clr = (line & (1 << row)) ? fg : bg;
                    if (scale == 1) _pixel(x + col, y + row, clr);
                    else _fill(x + col * scale, y + row * scale, scale, scale, clr);
                }
            }
            if (scale == 1) _fill(x + 5, y, 1, 8, bg);
            else _fill(x + 5 * scale, y, scale, ch, bg);
        }
    }

    virtual int16_t _drawCharGFX(int32_t x, int32_t y, unsigned char c,
                                 uint16_t fg, uint16_t bg, uint8_t scale) {
        (void)bg;
        if (!_gfxFont) return 0;
        uint16_t first = pgm_read_word(&_gfxFont->first);
        uint16_t last  = pgm_read_word(&_gfxFont->last);
        if (c < first || c > last) return 0;

        uint16_t idx = c - first;
        const GFXglyph* glyphs = (const GFXglyph*)pgm_read_ptr(&_gfxFont->glyph);
        const uint8_t*  bitmap = (const uint8_t*)pgm_read_ptr(&_gfxFont->bitmap);

        uint32_t bo = pgm_read_word(&glyphs[idx].bitmapOffset);
        uint8_t  w  = pgm_read_byte(&glyphs[idx].width);
        uint8_t  h  = pgm_read_byte(&glyphs[idx].height);
        uint8_t  xa = pgm_read_byte(&glyphs[idx].xAdvance);
        int8_t   xo = (int8_t)pgm_read_byte(&glyphs[idx].xOffset);
        int8_t   yo = (int8_t)pgm_read_byte(&glyphs[idx].yOffset);

        uint8_t bits = 0, bit = 0;
        uint16_t hpc = 0;

        for (uint8_t yy = 0; yy < h; yy++) {
            for (uint8_t xx = 0; xx < w; xx++) {
                if (bit == 0) {
                    bits = pgm_read_byte(&bitmap[bo++]);
                    bit = 0x80;
                }
                if (bits & bit) {
                    hpc++;
                } else {
                    if (hpc) {
                        if (scale == 1) {
                            drawFastHLine(x + xo + xx - hpc, y + yo + yy, hpc, fg);
                        } else {
                            fillRect(x + (xo + xx - hpc) * scale, y + (yo + yy) * scale, scale * hpc, scale, fg);
                        }
                        hpc = 0;
                    }
                }
                bit >>= 1;
            }
            if (hpc) {
                if (scale == 1) {
                    drawFastHLine(x + xo + w - hpc, y + yo + yy, hpc, fg);
                } else {
                    fillRect(x + (xo + w - hpc) * scale, y + (yo + yy) * scale, scale * hpc, scale, fg);
                }
                hpc = 0;
            }
        }
        return (int16_t)(xa * scale);
    }

public:
    TFT_eSPI() = default;
    virtual ~TFT_eSPI() = default;

    virtual bool isSprite() const { return false; }

    void init() {
        display.begin();
#ifndef AMK_CYD_NO_GLOBAL_TOUCH
        touch.begin(display);
#endif
    }
    void begin() { init(); }

    void setRotation(uint8_t r) {
        display.setRotation(r);
#ifndef AMK_CYD_NO_GLOBAL_TOUCH
        touch.setRotation(r);
#endif
    }
    uint8_t getRotation() const       { return display.rotation(); }
    void setSwapBytes(bool swap)      { _swapBytes = swap;         }
    bool getSwapBytes() const         { return _swapBytes;         }
    void setBGR(bool bgr)             { display.setBGR(bgr);       }
    bool isBGR() const                { return display.isBGR();    }
    void setPanel(cydr_panel_t panel) { display.setPanel(panel);   }
    cydr_panel_t getPanel() const     { return display.panel();    }
    void setMirrorX(bool mirror)      { display.setMirrorX(mirror); }
    bool isMirrorX() const            { return display.isMirrorX(); }
    void setBacklight(int pct)        { display.setBacklight(pct); }
    void setBacklightPin(uint8_t pin) { display.setBacklightPin(pin); }
    uint8_t backlightPin() const      { return display.backlightPin(); }
    virtual int16_t width()  const    { return display.width();    }
    virtual int16_t height() const    { return display.height();   }

    virtual void fillScreen(uint16_t color) { _fill(0, 0, width(), height(), color); }
    void invertDisplay(bool invert)   { display.invertDisplay(invert); }

    virtual uint16_t readPixel(int32_t x, int32_t y) const { (void)x; (void)y; return 0; }

    virtual void drawPixel(int32_t x, int32_t y, uint16_t color) {
        _pixel(x, y, color);
        if (!isSprite()) display.waitFlush();
    }

    virtual void drawFastHLine(int32_t x, int32_t y, int32_t w, uint16_t color) {
        _fill(x, y, w, 1, color);
    }

    virtual void drawFastVLine(int32_t x, int32_t y, int32_t h, uint16_t color) {
        _fill(x, y, 1, h, color);
    }

    virtual void fillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color) {
        _fill(x, y, w, h, color);
    }

    void drawRect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color) {
        drawFastHLine(x, y, w, color);
        drawFastHLine(x, y + h - 1, w, color);
        drawFastVLine(x, y, h, color);
        drawFastVLine(x + w - 1, y, h, color);
    }

    // Circles (Bresenham)
    void drawCircle(int32_t cx, int32_t cy, int32_t r, uint16_t color) {
        int32_t f = 1 - r, ddx = 1, ddy = -2 * r, x = 0, y = r;
        _pixel(cx, cy + r, color);  _pixel(cx, cy - r, color);
        _pixel(cx + r, cy, color);  _pixel(cx - r, cy, color);
        while (x < y) {
            if (f >= 0) { y--; ddy += 2; f += ddy; }
            x++; ddx += 2; f += ddx;
            _pixel(cx + x, cy + y, color); _pixel(cx - x, cy + y, color);
            _pixel(cx + x, cy - y, color); _pixel(cx - x, cy - y, color);
            _pixel(cx + y, cy + x, color); _pixel(cx - y, cy + x, color);
            _pixel(cx + y, cy - x, color); _pixel(cx - y, cy - x, color);
        }
        if (!isSprite()) display.waitFlush();
    }

    void fillCircle(int32_t cx, int32_t cy, int32_t r, uint16_t color) {
        drawFastHLine(cx - r, cy, 2 * r + 1, color);
        int32_t f = 1 - r, ddx = 1, ddy = -2 * r, x = 0, y = r;
        while (x < y) {
            if (f >= 0) { y--; ddy += 2; f += ddy; }
            x++; ddx += 2; f += ddx;
            drawFastHLine(cx - x, cy + y, 2 * x + 1, color);
            drawFastHLine(cx - x, cy - y, 2 * x + 1, color);
            drawFastHLine(cx - y, cy + x, 2 * y + 1, color);
            drawFastHLine(cx - y, cy - x, 2 * y + 1, color);
        }
    }

    // Ellipses (Midpoint)
    void drawEllipse(int32_t cx, int32_t cy, int32_t rx, int32_t ry, uint16_t color) {
        if (rx <= 0 || ry <= 0) return;
        int32_t x = 0, y = ry;
        int64_t rx2 = (int64_t)rx * rx;
        int64_t ry2 = (int64_t)ry * ry;
        int64_t fx2 = 4 * rx2;
        int64_t fy2 = 4 * ry2;
        int64_t s = 2 * ry2 + rx2 * (1 - 2 * ry);
        do {
            _pixel(cx + x, cy + y, color);
            _pixel(cx - x, cy + y, color);
            _pixel(cx - x, cy - y, color);
            _pixel(cx + x, cy - y, color);
            if (s >= 0) { s += fx2 * (1 - y); y--; }
            s += ry2 * ((4 * x) + 6);
            x++;
        } while (fx2 * y > fy2 * x);

        s = 2 * rx2 + ry2 * (1 - 2 * x);
        do {
            _pixel(cx + x, cy + y, color);
            _pixel(cx - x, cy + y, color);
            _pixel(cx - x, cy - y, color);
            _pixel(cx + x, cy - y, color);
            if (s >= 0) { s += fy2 * (1 - x); x--; }
            s += rx2 * ((4 * y) + 6);
            y--;
        } while (y >= 0);
        if (!isSprite()) display.waitFlush();
    }

    void fillEllipse(int32_t cx, int32_t cy, int32_t rx, int32_t ry, uint16_t color) {
        if (rx <= 0 || ry <= 0) return;
        int32_t x = 0, y = ry;
        int64_t rx2 = (int64_t)rx * rx;
        int64_t ry2 = (int64_t)ry * ry;
        int64_t fx2 = 4 * rx2;
        int64_t fy2 = 4 * ry2;
        int64_t s = 2 * ry2 + rx2 * (1 - 2 * ry);
        do {
            drawFastHLine(cx - x, cy + y, 2 * x + 1, color);
            drawFastHLine(cx - x, cy - y, 2 * x + 1, color);
            if (s >= 0) { s += fx2 * (1 - y); y--; }
            s += ry2 * ((4 * x) + 6);
            x++;
        } while (fx2 * y > fy2 * x);

        s = 2 * rx2 + ry2 * (1 - 2 * x);
        do {
            drawFastHLine(cx - x, cy + y, 2 * x + 1, color);
            drawFastHLine(cx - x, cy - y, 2 * x + 1, color);
            if (s >= 0) { s += fy2 * (1 - x); x--; }
            s += rx2 * ((4 * y) + 6);
            y--;
        } while (y >= 0);
    }

    // Rounded rects
    void fillRoundRect(int32_t x, int32_t y, int32_t w, int32_t h,
                       int32_t r, uint16_t color) {
        if (w <= 0 || h <= 0) return;
        if (r <= 0) { fillRect(x, y, w, h, color); return; }
        if (r > w / 2) r = w / 2;
        if (r > h / 2) r = h / 2;

        fillRect(x, y + r, w, h - 2 * r, color);

        for (int32_t dy = 0; dy < r; dy++) {
            int32_t dx = (int32_t)sqrtf((float)((int64_t)r*r -
                                                 (int64_t)(r - dy)*(r - dy)));
            int32_t fx = x + r - dx;
            int32_t fw = w - 2 * (r - dx);
            if (fw > 0) {
                drawFastHLine(fx, y + dy,         fw, color);
                drawFastHLine(fx, y + h - 1 - dy, fw, color);
            }
        }
    }

    void drawRoundRect(int32_t x, int32_t y, int32_t w, int32_t h,
                       int32_t r, uint16_t color) {
        if (w <= 0 || h <= 0) return;
        if (r <= 0) { drawRect(x, y, w, h, color); return; }
        if (r > w / 2) r = w / 2;
        if (r > h / 2) r = h / 2;

        drawFastHLine(x + r, y,         w - 2*r, color);
        drawFastHLine(x + r, y + h - 1, w - 2*r, color);
        drawFastVLine(x,         y + r, h - 2*r, color);
        drawFastVLine(x + w - 1, y + r, h - 2*r, color);

        int32_t f = 1 - r, ddx = 1, ddy = -2 * r, xi = 0, yi = r;
        while (xi < yi) {
            if (f >= 0) { yi--; ddy += 2; f += ddy; }
            xi++; ddx += 2; f += ddx;
            _pixel(x + r - xi,         y + r - yi,         color);
            _pixel(x + r - yi,         y + r - xi,         color);
            _pixel(x + w - 1 - r + xi, y + r - yi,         color);
            _pixel(x + w - 1 - r + yi, y + r - xi,         color);
            _pixel(x + r - xi,         y + h - 1 - r + yi, color);
            _pixel(x + r - yi,         y + h - 1 - r + xi, color);
            _pixel(x + w - 1 - r + xi, y + h - 1 - r + yi, color);
            _pixel(x + w - 1 - r + yi, y + h - 1 - r + xi, color);
        }
        if (!isSprite()) display.waitFlush();
    }

    // Line (Bresenham)
    void drawLine(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint16_t color) {
        int32_t dx = abs(x1 - x0), dy = abs(y1 - y0);
        int32_t sx = (x0 < x1) ? 1 : -1;
        int32_t sy = (y0 < y1) ? 1 : -1;
        int32_t err = dx - dy;
        for (;;) {
            _pixel(x0, y0, color);
            if (x0 == x1 && y0 == y1) break;
            int32_t e2 = 2 * err;
            if (e2 > -dy) { err -= dy; x0 += sx; }
            if (e2 <  dx) { err += dx; y0 += sy; }
        }
        if (!isSprite()) display.waitFlush();
    }

    // Triangle primitives
    void drawTriangle(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint16_t color) {
        drawLine(x0, y0, x1, y1, color);
        drawLine(x1, y1, x2, y2, color);
        drawLine(x2, y2, x0, y0, color);
    }

    void fillTriangle(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint16_t color) {
        int32_t a, b, y, last;
        if (y0 > y1) { std::swap(y0, y1); std::swap(x0, x1); }
        if (y1 > y2) { std::swap(y2, y1); std::swap(x2, x1); }
        if (y0 > y1) { std::swap(y0, y1); std::swap(x0, x1); }

        if (y0 == y2) {
            a = b = x0;
            if (x1 < a)      a = x1;
            else if (x1 > b) b = x1;
            if (x2 < a)      a = x2;
            else if (x2 > b) b = x2;
            drawFastHLine(a, y0, b - a + 1, color);
            return;
        }

        int32_t dx01 = x1 - x0, dy01 = y1 - y0;
        int32_t dx02 = x2 - x0, dy02 = y2 - y0;
        int32_t dx12 = x2 - x1, dy12 = y2 - y1;
        int32_t sa = 0, sb = 0;

        if (y1 == y2) last = y1;
        else         last = y1 - 1;

        for (y = y0; y <= last; y++) {
            a = x0 + sa / dy01;
            b = x0 + sb / dy02;
            sa += dx01;
            sb += dx02;
            if (a > b) std::swap(a, b);
            drawFastHLine(a, y, b - a + 1, color);
        }

        sa = dx12 * (y - y1);
        sb = dx02 * (y - y0);
        for (; y <= y2; y++) {
            a = x1 + sa / dy12;
            b = x0 + sb / dy02;
            sa += dx12;
            sb += dx02;
            if (a > b) std::swap(a, b);
            drawFastHLine(a, y, b - a + 1, color);
        }
    }

    // Wide and wedge lines
    void drawWideLine(float ax, float ay, float bx, float by, float wd, uint16_t fgcolor, uint16_t bgcolor = 0xFFFF) {
        (void)bgcolor;
        if (wd <= 1.0f) {
            drawLine((int32_t)roundf(ax), (int32_t)roundf(ay), (int32_t)roundf(bx), (int32_t)roundf(by), fgcolor);
            return;
        }
        float dx = bx - ax;
        float dy = by - ay;
        float len = sqrtf(dx * dx + dy * dy);
        if (len == 0.0f) {
            fillCircle((int32_t)roundf(ax), (int32_t)roundf(ay), (int32_t)(wd / 2.0f), fgcolor);
            return;
        }
        float halfW = wd / (2.0f * len);
        float px = -dy * halfW;
        float py =  dx * halfW;

        int32_t x0 = (int32_t)roundf(ax + px), y0 = (int32_t)roundf(ay + py);
        int32_t x1 = (int32_t)roundf(ax - px), y1 = (int32_t)roundf(ay - py);
        int32_t x2 = (int32_t)roundf(bx - px), y2 = (int32_t)roundf(by - py);
        int32_t x3 = (int32_t)roundf(bx + px), y3 = (int32_t)roundf(by + py);

        fillTriangle(x0, y0, x1, y1, x2, y2, fgcolor);
        fillTriangle(x0, y0, x2, y2, x3, y3, fgcolor);
    }

    void drawWedgeLine(float ax, float ay, float bx, float by, float aw, float bw, uint16_t fgcolor, uint16_t bgcolor = 0xFFFF) {
        (void)bgcolor;
        float dx = bx - ax;
        float dy = by - ay;
        float len = sqrtf(dx * dx + dy * dy);
        if (len == 0.0f) {
            fillCircle((int32_t)roundf(ax), (int32_t)roundf(ay), (int32_t)(aw / 2.0f), fgcolor);
            return;
        }
        float invLen = 1.0f / len;
        float pxA = -dy * (aw * 0.5f * invLen);
        float pyA =  dx * (aw * 0.5f * invLen);
        float pxB = -dy * (bw * 0.5f * invLen);
        float pyB =  dx * (bw * 0.5f * invLen);

        int32_t x0 = (int32_t)roundf(ax + pxA), y0 = (int32_t)roundf(ay + pyA);
        int32_t x1 = (int32_t)roundf(ax - pxA), y1 = (int32_t)roundf(ay - pyA);
        int32_t x2 = (int32_t)roundf(bx - pxB), y2 = (int32_t)roundf(by - pyB);
        int32_t x3 = (int32_t)roundf(bx + pxB), y3 = (int32_t)roundf(by + pyB);

        fillTriangle(x0, y0, x1, y1, x2, y2, fgcolor);
        fillTriangle(x0, y0, x2, y2, x3, y3, fgcolor);
    }

    // Image rendering
    virtual void pushImage(int32_t x, int32_t y, int32_t w, int32_t h, const uint16_t *data) {
        if (!data || w <= 0 || h <= 0) return;
        int16_t sw = width(), sh = height();
        if (x >= sw || y >= sh || x + w <= 0 || y + h <= 0) return;

        alignas(4) static uint16_t _imgLineBuf[480];
        for (int32_t row = 0; row < h; row++) {
            int32_t curY = y + row;
            if (curY < 0 || curY >= sh) continue;

            int32_t srcStart = 0;
            int32_t dstX = x;
            int32_t curW = w;
            if (dstX < 0) {
                srcStart = -dstX;
                curW += dstX;
                dstX = 0;
            }
            if (dstX + curW > sw) curW = sw - dstX;
            if (curW <= 0) continue;

            display.waitFlush();
            const uint16_t *srcLine = data + (row * w) + srcStart;
            for (int32_t i = 0; i < curW; i++) {
                _imgLineBuf[i] = _swapBytes ? __builtin_bswap16(srcLine[i]) : srcLine[i];
            }
            display.flushRegion(dstX, curY, dstX + curW - 1, curY, _imgLineBuf);
        }
        display.waitFlush();
    }

    virtual void pushImage(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t *data) {
        pushImage(x, y, w, h, (const uint16_t*)data);
    }

    virtual void pushImage(int32_t x, int32_t y, int32_t w, int32_t h, const uint16_t *data, uint16_t transparent) {
        if (!data || w <= 0 || h <= 0) return;
        int16_t sw = width(), sh = height();
        for (int32_t row = 0; row < h; row++) {
            int32_t curY = y + row;
            if (curY < 0 || curY >= sh) continue;
            for (int32_t col = 0; col < w; col++) {
                int32_t curX = x + col;
                if (curX < 0 || curX >= sw) continue;
                uint16_t colr = data[row * w + col];
                if (_swapBytes) colr = __builtin_bswap16(colr);
                if (colr != transparent) {
                    drawPixel(curX, curY, colr);
                }
            }
        }
        if (!isSprite()) display.waitFlush();
    }

    virtual void pushImage(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t *data, uint16_t transparent) {
        pushImage(x, y, w, h, (const uint16_t*)data, transparent);
    }

    // Text & cursor
    void setCursor(int16_t x, int16_t y) {
        _cursorX = x;
        _cursorY = y;
    }
    void setCursor(int16_t x, int16_t y, uint8_t font) {
        _cursorX = x;
        _cursorY = y;
        if (font > 0) _textSize = (font > 8) ? 8 : font;
    }
    int16_t getCursorX() const { return _cursorX; }
    int16_t getCursorY() const { return _cursorY; }

    void setTextColor(uint16_t fg)               { _textFg = fg; _textBg = fg; }
    void setTextColor(uint16_t fg, uint16_t bg)  { _textFg = fg; _textBg = bg; }
    void setTextSize(uint8_t s)                  { _textSize = (s > 0) ? s : 1; }
    void setTextFont(uint8_t f)                  { _gfxFont = nullptr; if (f > 0) _textSize = (f > 8) ? 8 : f; }
    void setTextDatum(uint8_t d)                 { _textDatum = d; }
    void setTextWrap(bool wrap)                  { _textWrap = wrap; }
    void setTextWrap(bool wrapX, bool wrapY)     { _textWrap = wrapX; (void)wrapY; }
    void setTextPadding(uint16_t padding)        { _textPadding = padding; }
    uint16_t getTextPadding() const              { return _textPadding; }

    void setFreeFont(const GFXfont *f = nullptr) {
        _gfxFont = f;
        _glyph_ab = 0;
        _glyph_bb = 0;
        if (!_gfxFont) return;
        uint16_t first = pgm_read_word(&_gfxFont->first);
        uint16_t last  = pgm_read_word(&_gfxFont->last);
        if (last < first) return;
        uint16_t numChars = last - first + 1;
        const GFXglyph* glyphs = (const GFXglyph*)pgm_read_ptr(&_gfxFont->glyph);
        for (uint16_t i = 0; i < numChars; i++) {
            int8_t ab = -(int8_t)pgm_read_byte(&glyphs[i].yOffset);
            if (ab > _glyph_ab) _glyph_ab = ab;
            int8_t bb = (int8_t)pgm_read_byte(&glyphs[i].height) - ab;
            if (bb > _glyph_bb) _glyph_bb = bb;
        }
    }
    const GFXfont* getFreeFont() const { return _gfxFont; }

    int16_t drawString(const char* str, int32_t x, int32_t y, uint8_t font = 0) {
        if (!str) return 0;
        uint8_t scale = (font > 0) ? font : _textSize;
        if (scale == 0) scale = 1;
        if (scale > 8) scale = 8;

        if (_gfxFont && font == 0) {
            int16_t tw = textWidth(str, 0);
            int16_t cheight = _glyph_ab * scale;
            int16_t th = (_glyph_ab + _glyph_bb) * scale;
            int32_t baselineY = y + cheight;

            switch (_textDatum) {
                case TC_DATUM: x -= tw / 2; break;
                case TR_DATUM: x -= tw;     break;
                case ML_DATUM: baselineY = y + (_glyph_ab - _glyph_bb) * scale / 2; break;
                case MC_DATUM: x -= tw / 2; baselineY = y + (_glyph_ab - _glyph_bb) * scale / 2; break;
                case MR_DATUM: x -= tw;     baselineY = y + (_glyph_ab - _glyph_bb) * scale / 2; break;
                case BL_DATUM: baselineY = y - _glyph_bb * scale; break;
                case BC_DATUM: x -= tw / 2; baselineY = y - _glyph_bb * scale; break;
                case BR_DATUM: x -= tw;     baselineY = y - _glyph_bb * scale; break;
                default: break; // TL_DATUM
            }

            int32_t padX = x;
            int16_t padW = tw;
            if (_textPadding > tw) {
                padW = _textPadding;
                if (_textDatum == TC_DATUM || _textDatum == MC_DATUM || _textDatum == BC_DATUM) {
                    padX = x - (padW - tw) / 2;
                } else if (_textDatum == TR_DATUM || _textDatum == MR_DATUM || _textDatum == BR_DATUM) {
                    padX = x - (padW - tw);
                }
            }

            if (_textBg != TFT_TRANSPARENT && _textFg != _textBg) {
                fillRect(padX, baselineY - cheight, padW, th, _textBg);
            }

            int32_t curX = x;
            size_t len = strlen(str);
            for (size_t i = 0; i < len; i++) {
                curX += _drawCharGFX(curX, baselineY, (unsigned char)str[i], _textFg, _textBg, scale);
            }
            if (!isSprite()) display.waitFlush();
            return tw;
        }

        int len = (int)strlen(str);
        int16_t tw = len * 6 * scale;
        int16_t th = 8 * scale;

        switch (_textDatum) {
            case TC_DATUM: x -= tw / 2; break;
            case TR_DATUM: x -= tw;     break;
            case ML_DATUM:               y -= th / 2; break;
            case MC_DATUM: x -= tw / 2; y -= th / 2; break;
            case MR_DATUM: x -= tw;     y -= th / 2; break;
            case BL_DATUM:               y -= th;     break;
            case BC_DATUM: x -= tw / 2; y -= th;     break;
            case BR_DATUM: x -= tw;     y -= th;     break;
            default: break;  // TL_DATUM
        }

        if (_textPadding > tw && _textBg != TFT_TRANSPARENT && _textFg != _textBg) {
            int16_t padW = _textPadding - tw;
            int32_t padX = x + tw;
            if (_textDatum == TC_DATUM || _textDatum == MC_DATUM || _textDatum == BC_DATUM) {
                int16_t half = padW / 2;
                fillRect(x - half, y, half, th, _textBg);
                fillRect(padX, y, padW - half, th, _textBg);
            } else if (_textDatum == TR_DATUM || _textDatum == MR_DATUM || _textDatum == BR_DATUM) {
                fillRect(x - padW, y, padW, th, _textBg);
            } else {
                fillRect(padX, y, padW, th, _textBg);
            }
        }

        for (int i = 0; i < len; i++)
            _drawChar(x + i * 6 * scale, y, str[i], _textFg, _textBg, scale);
        if (!isSprite()) display.waitFlush();
        return tw;
    }

    int16_t drawString(const String &s, int32_t x, int32_t y, uint8_t font = 0) {
        return drawString(s.c_str(), x, y, font);
    }

    int16_t drawCentreString(const char* str, int32_t x, int32_t y, uint8_t font = 0) {
        uint8_t prev = _textDatum;
        _textDatum = TC_DATUM;
        int16_t w = drawString(str, x, y, font);
        _textDatum = prev;
        return w;
    }
    int16_t drawCentreString(const String &s, int32_t x, int32_t y, uint8_t font = 0) {
        return drawCentreString(s.c_str(), x, y, font);
    }

    int16_t drawRightString(const char* str, int32_t x, int32_t y, uint8_t font = 0) {
        uint8_t prev = _textDatum;
        _textDatum = TR_DATUM;
        int16_t w = drawString(str, x, y, font);
        _textDatum = prev;
        return w;
    }
    int16_t drawRightString(const String &s, int32_t x, int32_t y, uint8_t font = 0) {
        return drawRightString(s.c_str(), x, y, font);
    }

    int16_t drawNumber(long int_num, int32_t poX, int32_t poY, uint8_t font = 0) {
        char buf[32];
        ltoa(int_num, buf, 10);
        return drawString(buf, poX, poY, font);
    }

    int16_t drawFloat(float floatNumber, uint8_t decimal, int32_t poX, int32_t poY, uint8_t font = 0) {
        char buf[32];
        dtostrf(floatNumber, 0, decimal, buf);
        return drawString(buf, poX, poY, font);
    }

    int16_t textWidth(const char* str, uint8_t font = 0) const {
        if (!str) return 0;
        uint8_t scale = (font > 0) ? font : _textSize;
        if (scale == 0) scale = 1;
        if (scale > 8) scale = 8;

        if (_gfxFont && font == 0) {
            int16_t w = 0;
            uint16_t first = pgm_read_word(&_gfxFont->first);
            uint16_t last  = pgm_read_word(&_gfxFont->last);
            const GFXglyph* glyphs = (const GFXglyph*)pgm_read_ptr(&_gfxFont->glyph);
            const char* p = str;
            while (*p) {
                uint8_t c = (uint8_t)*p++;
                if (c >= first && c <= last) {
                    uint16_t idx = c - first;
                    uint8_t xa = pgm_read_byte(&glyphs[idx].xAdvance);
                    if (*p) {
                        w += xa;
                    } else {
                        int8_t xo = (int8_t)pgm_read_byte(&glyphs[idx].xOffset);
                        uint8_t gw = pgm_read_byte(&glyphs[idx].width);
                        w += (xa > (xo + gw)) ? xa : (xo + gw);
                    }
                }
            }
            return w * scale;
        }

        return (int16_t)(strlen(str) * 6 * scale);
    }
    int16_t textWidth(const String &s, uint8_t font = 0) const {
        return textWidth(s.c_str(), font);
    }

    int16_t fontHeight(uint8_t font = 0) const {
        uint8_t scale = (font > 0) ? font : _textSize;
        if (scale == 0) scale = 1;
        if (scale > 8) scale = 8;
        if (_gfxFont && font == 0) {
            return (int16_t)(pgm_read_byte(&_gfxFont->yAdvance) * scale);
        }
        return (int16_t)(8 * scale);
    }

    // Pivot support
    void setPivot(int16_t x, int16_t y) { _pivotX = x; _pivotY = y; }
    int16_t getPivotX() const           { return _pivotX; }
    int16_t getPivotY() const           { return _pivotY; }

    // Color conversion
    static inline uint16_t color565(uint8_t r, uint8_t g, uint8_t b) {
        return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
    }
    static inline uint16_t color24to16(uint32_t c) {
        return color565((c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF);
    }
    static inline uint8_t color16to8(uint16_t c) {
        return (uint8_t)(((c & 0xE000) >> 8) | ((c & 0x0700) >> 6) | ((c & 0x0018) >> 3));
    }
    static inline uint16_t color8to16(uint8_t c) {
        uint16_t r = (c >> 5) & 0x07, g = (c >> 2) & 0x07, b = c & 0x03;
        uint16_t r5 = (r << 2) | (r >> 1);
        uint16_t g6 = (g << 3) | g;
        uint16_t b5 = (b << 3) | (b << 1) | (b >> 1);
        return (r5 << 11) | (g6 << 5) | b5;
    }

#ifndef AMK_CYD_NO_GLOBAL_TOUCH
    bool getTouch(uint16_t *x, uint16_t *y, uint16_t threshold = 600) {
        (void)threshold;
        if (!x || !y) return false;
        return touch.read(*x, *y);
    }
    bool getTouchRaw(uint16_t *x, uint16_t *y) {
        if (!x || !y) return false;
        return touch.readRaw(*x, *y);
    }
    uint16_t getTouchRawZ() {
        return touch.pressure();
    }
    void setTouch(uint16_t *parameters) {
        if (!parameters) return;
        touch.setCalibration(parameters[0], parameters[1], parameters[2], parameters[3]);
    }
    void calibrateTouch(uint16_t *parameters, uint16_t color_fg, uint16_t color_bg, uint8_t size) {
        if (!parameters) return;
        int16_t w = width(), h = height();
        const int16_t cx[4] = { (int16_t)size, (int16_t)(w - size), (int16_t)(w - size), (int16_t)size };
        const int16_t cy[4] = { (int16_t)size, (int16_t)size, (int16_t)(h - size), (int16_t)(h - size) };
        uint16_t rawX[4] = {0}, rawY[4] = {0};

        for (int i = 0; i < 4; i++) {
            fillCircle(cx[i], cy[i], size, color_fg);
            fillCircle(cx[i], cy[i], size > 2 ? size - 2 : 1, color_bg);
            fillCircle(cx[i], cy[i], 2, color_fg);

            uint16_t rx, ry;
            while (touch.readRaw(rx, ry)) delay(10);
            while (!touch.readRaw(rx, ry)) delay(10);

            uint32_t sumX = 0, sumY = 0;
            int count = 0;
            uint32_t start = millis();
            while (millis() - start < 400) {
                if (touch.readRaw(rx, ry)) {
                    sumX += rx; sumY += ry;
                    count++;
                }
                delay(10);
            }
            if (count > 0) {
                rawX[i] = sumX / count;
                rawY[i] = sumY / count;
            }
            fillCircle(cx[i], cy[i], size, color_bg);
            delay(100);
        }

        uint16_t rx, ry;
        while (touch.readRaw(rx, ry)) delay(10);

        int16_t natW = touch.nativeWidth();
        int16_t natH = touch.nativeHeight();
        if (natW <= 0) natW = 240;
        if (natH <= 0) natH = 320;
        int16_t W = natW - 1;
        int16_t H = natH - 1;

        float natX[4] = {0}, natY[4] = {0};
        for (int i = 0; i < 4; i++) {
            float px, py;
            switch (getRotation()) {
                case 0: px = cx[i];     py = cy[i];     break;
                case 1: px = W - cy[i]; py = cx[i];     break;
                case 2: px = W - cx[i]; py = H - cy[i]; break;
                case 3: px = cy[i];     py = H - cx[i]; break;
                default: px = cx[i];    py = cy[i];     break;
            }
            natX[i] = px;
            natY[i] = py;
        }

        auto linfit = [](const float *I, const uint16_t *D, int n, float &a, float &b) {
            float sI = 0, sD = 0, sII = 0, sID = 0;
            for (int i = 0; i < n; i++) {
                sI += I[i];
                sD += (float)D[i];
                sII += I[i] * I[i];
                sID += I[i] * (float)D[i];
            }
            float den = n * sII - sI * sI;
            b = (den != 0.0f) ? (n * sID - sI * sD) / den : 0.0f;
            a = (sD - b * sI) / n;
        };

        float ax, bx, ay, by;
        linfit(natX, rawX, 4, ax, bx);
        linfit(natY, rawY, 4, ay, by);

        auto clampRaw = [](float v) -> uint16_t {
            if (v < 0.0f) return 0;
            if (v > 4095.0f) return 4095;
            return (uint16_t)(v + 0.5f);
        };

        const bool mx = touch.mirrorsX(), my = touch.mirrorsY();
        uint16_t rx0 = clampRaw(ax), rx1 = clampRaw(ax + bx * (natW - 1));
        uint16_t ry0 = clampRaw(ay), ry1 = clampRaw(ay + by * (natH - 1));
        uint16_t minX = mx ? rx1 : rx0, maxX = mx ? rx0 : rx1;
        uint16_t minY = my ? ry1 : ry0, maxY = my ? ry0 : ry1;
        if (minX > maxX) std::swap(minX, maxX);
        if (minY > maxY) std::swap(minY, maxY);

        parameters[0] = minX;
        parameters[1] = maxX;
        parameters[2] = minY;
        parameters[3] = maxY;
        parameters[4] = 0;

        touch.setCalibration(minX, maxX, minY, maxY);
    }
#endif

    // Print interface
    virtual size_t write(uint8_t c) override {
        if (c == '\r') {
            _cursorX = 0;
            return 1;
        }
        if (c == '\n') {
            _cursorX = 0;
            _cursorY += fontHeight();
            return 1;
        }
        uint8_t scale = _textSize;
        if (scale == 0) scale = 1;
        if (scale > 8) scale = 8;

        if (_gfxFont) {
            uint16_t first = pgm_read_word(&_gfxFont->first);
            uint16_t last  = pgm_read_word(&_gfxFont->last);
            if (c >= first && c <= last) {
                uint16_t idx = c - first;
                const GFXglyph* glyphs = (const GFXglyph*)pgm_read_ptr(&_gfxFont->glyph);
                uint8_t xa = pgm_read_byte(&glyphs[idx].xAdvance);
                int16_t cw = xa * scale;
                if (_textWrap && (_cursorX + cw > width())) {
                    _cursorX = 0;
                    _cursorY += fontHeight();
                }
                _drawCharGFX(_cursorX, _cursorY, c, _textFg, _textBg, scale);
                _cursorX += cw;
                if (!isSprite()) display.waitFlush();
            }
            return 1;
        }

        int16_t cw = 6 * scale;
        int16_t ch = 8 * scale;
        if (_textWrap && (_cursorX + cw > width())) {
            _cursorX = 0;
            _cursorY += ch;
        }
        _drawChar(_cursorX, _cursorY, c, _textFg, _textBg, scale);
        _cursorX += cw;
        if (!isSprite()) display.waitFlush();
        return 1;
    }

    virtual size_t write(const uint8_t *buffer, size_t size) override {
        if (!buffer || size == 0) return 0;
        size_t n = 0;
        while (size--) {
            if (write(*buffer++)) n++;
            else break;
        }
        return n;
    }
};

// ═══════════════════════════════════════════════════════════════════════════════
//  TFT_eSprite  -  Full off-screen canvas / frame buffer (16-bit, 8-bit, 4-bit)
// ═══════════════════════════════════════════════════════════════════════════════
class TFT_eSprite : public TFT_eSPI {
private:
    TFT_eSPI* _tft = nullptr;
    int16_t   _w = 0;
    int16_t   _h = 0;
    uint8_t   _bpp = 16;
    uint8_t*  _buffer = nullptr;
    bool      _isCreated = false;
    uint16_t  _palette[16];
    uint8_t   _rotation = 0;

    static inline uint8_t color565to332(uint16_t c) {
        return (uint8_t)(((c & 0xE000) >> 8) | ((c & 0x0700) >> 6) | ((c & 0x0018) >> 3));
    }

    static inline uint16_t color332to565(uint8_t c) {
        uint16_t r = (c >> 5) & 0x07, g = (c >> 2) & 0x07, b = c & 0x03;
        uint16_t r5 = (r << 2) | (r >> 1);
        uint16_t g6 = (g << 3) | g;
        uint16_t b5 = (b << 3) | (b << 1) | (b >> 1);
        return (r5 << 11) | (g6 << 5) | b5;
    }

    uint8_t _mapColorTo4Bit(uint16_t color) const {
        if (color < 16) return (uint8_t)color;
        for (uint8_t i = 0; i < 16; i++) {
            if (_palette[i] == color) return i;
        }
        int r1 = (color >> 11) & 0x1F, g1 = (color >> 5) & 0x3F, b1 = color & 0x1F;
        uint8_t bestIdx = 0;
        int bestDist = 999999;
        for (uint8_t i = 0; i < 16; i++) {
            int r2 = (_palette[i] >> 11) & 0x1F;
            int g2 = (_palette[i] >> 5) & 0x3F;
            int b2 = _palette[i] & 0x1F;
            int dr = r1 - r2, dg = (g1 - g2) >> 1, db = b1 - b2;
            int dist = dr * dr + dg * dg + db * db;
            if (dist < bestDist) {
                bestDist = dist;
                bestIdx = i;
                if (dist == 0) break;
            }
        }
        return bestIdx;
    }

protected:
    void _pixel(int32_t x, int32_t y, uint16_t color) override {
        if (!_isCreated || !_buffer) return;
        if (x < 0 || y < 0 || x >= _w || y >= _h) return;
        if (_bpp == 16) {
            ((uint16_t*)_buffer)[y * _w + x] = color;
        } else if (_bpp == 8) {
            _buffer[y * _w + x] = color565to332(color);
        } else {
            uint8_t idx = _mapColorTo4Bit(color);
            size_t byteIdx = ((size_t)y * _w + x) >> 1;
            if ((x & 1) == 0) {
                _buffer[byteIdx] = (_buffer[byteIdx] & 0x0F) | (idx << 4);
            } else {
                _buffer[byteIdx] = (_buffer[byteIdx] & 0xF0) | (idx & 0x0F);
            }
        }
    }

    void _fill(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color) override {
        if (!_isCreated || !_buffer || w <= 0 || h <= 0) return;
        if (x < 0) { w += x; x = 0; }
        if (y < 0) { h += y; y = 0; }
        if (x + w > _w) w = _w - x;
        if (y + h > _h) h = _h - y;
        if (w <= 0 || h <= 0) return;

        if (_bpp == 16) {
            for (int32_t row = 0; row < h; row++) {
                uint16_t* line = ((uint16_t*)_buffer) + (y + row) * _w + x;
                for (int32_t col = 0; col < w; col++) line[col] = color;
            }
        } else if (_bpp == 8) {
            uint8_t c8 = color565to332(color);
            for (int32_t row = 0; row < h; row++) {
                uint8_t* line = _buffer + (y + row) * _w + x;
                memset(line, c8, w);
            }
        } else {
            uint8_t idx = _mapColorTo4Bit(color);
            for (int32_t row = 0; row < h; row++) {
                for (int32_t col = 0; col < w; col++) {
                    int32_t curX = x + col;
                    size_t byteIdx = ((size_t)(y + row) * _w + curX) >> 1;
                    if ((curX & 1) == 0) {
                        _buffer[byteIdx] = (_buffer[byteIdx] & 0x0F) | (idx << 4);
                    } else {
                        _buffer[byteIdx] = (_buffer[byteIdx] & 0xF0) | (idx & 0x0F);
                    }
                }
            }
        }
    }

public:
    explicit TFT_eSprite(TFT_eSPI* tft = nullptr) : _tft(tft) {
        for (int i = 0; i < 16; i++) _palette[i] = 0;
    }
    virtual ~TFT_eSprite() override { deleteSprite(); }

    bool isSprite() const override { return true; }
    int16_t width()  const override { return _w; }
    int16_t height() const override { return _h; }

    void* setColorDepth(int8_t b) {
        _bpp = (b == 4 || b == 8) ? b : 16;
        if (_isCreated && _w > 0 && _h > 0) {
            int16_t w = _w, h = _h;
            return createSprite(w, h);
        }
        return (void*)_buffer;
    }

    uint8_t getColorDepth() const { return _bpp; }

    void createPalette(const uint16_t* cmap = nullptr, uint8_t colors = 16) {
        if (cmap) {
            uint8_t count = (colors > 16) ? 16 : colors;
            for (uint8_t i = 0; i < count; i++) _palette[i] = cmap[i];
        }
    }

    void setPaletteColor(uint8_t index, uint16_t color) {
        if (index < 16) _palette[index] = color;
    }

    uint16_t getPaletteColor(uint8_t index) const {
        return (index < 16) ? _palette[index] : 0;
    }

    void* createSprite(int16_t w, int16_t h, uint8_t frames = 1) {
        (void)frames;
        deleteSprite();
        if (w <= 0 || h <= 0) return nullptr;
        _w = w;
        _h = h;
        _pivotX = w / 2;
        _pivotY = h / 2;

        size_t bytes = 0;
        if (_bpp == 16) bytes = (size_t)_w * _h * sizeof(uint16_t);
        else if (_bpp == 8) bytes = (size_t)_w * _h;
        else bytes = ((size_t)_w * _h + 1) / 2;

        _buffer = (uint8_t*)heap_caps_malloc(bytes, MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);
        if (!_buffer) _buffer = (uint8_t*)malloc(bytes);

        if (_buffer) {
            memset(_buffer, 0, bytes);
            _isCreated = true;
            return (void*)_buffer;
        }
        _isCreated = false;
        _w = 0; _h = 0;
        return nullptr;
    }

    bool created() const { return _isCreated && (_buffer != nullptr); }

    void deleteSprite() {
        if (_buffer) {
            free(_buffer);
            _buffer = nullptr;
        }
        _isCreated = false;
        _w = 0;
        _h = 0;
    }

    void* getPointer() { return (void*)_buffer; }
    void* frameBuffer(int8_t f = 1) { (void)f; return (void*)_buffer; }

    void fillSprite(uint16_t color) {
        if (!_isCreated || !_buffer) return;
        if (_bpp == 16) {
            size_t count = (size_t)_w * _h;
            uint16_t* b16 = (uint16_t*)_buffer;
            for (size_t i = 0; i < count; i++) b16[i] = color;
        } else if (_bpp == 8) {
            uint8_t c8 = color565to332(color);
            memset(_buffer, c8, (size_t)_w * _h);
        } else {
            uint8_t idx = _mapColorTo4Bit(color);
            uint8_t doubleIdx = (idx << 4) | (idx & 0x0F);
            memset(_buffer, doubleIdx, ((size_t)_w * _h + 1) / 2);
        }
    }

    void fillScreen(uint16_t color) override {
        fillSprite(color);
    }

    uint16_t readPixel(int32_t x, int32_t y) const override {
        if (x < 0 || y < 0 || x >= _w || y >= _h || !_buffer) return 0;
        if (_bpp == 16) {
            return ((const uint16_t*)_buffer)[y * _w + x];
        } else if (_bpp == 8) {
            return color332to565(_buffer[y * _w + x]);
        } else {
            size_t byteIdx = ((size_t)y * _w + x) >> 1;
            uint8_t idx = ((x & 1) == 0) ? (_buffer[byteIdx] >> 4) : (_buffer[byteIdx] & 0x0F);
            return _palette[idx & 0x0F];
        }
    }

    uint16_t readPixelValue(int32_t x, int32_t y) const {
        if (x < 0 || y < 0 || x >= _w || y >= _h || !_buffer) return 0;
        if (_bpp == 16) return ((const uint16_t*)_buffer)[y * _w + x];
        if (_bpp == 8) return _buffer[y * _w + x];
        size_t byteIdx = ((size_t)y * _w + x) >> 1;
        return ((x & 1) == 0) ? (_buffer[byteIdx] >> 4) : (_buffer[byteIdx] & 0x0F);
    }

    void pushImage(int32_t x, int32_t y, int32_t w, int32_t h, const uint16_t* data) override {
        if (!_isCreated || !_buffer || !data || w <= 0 || h <= 0) return;
        for (int32_t row = 0; row < h; row++) {
            int32_t destY = y + row;
            if (destY < 0 || destY >= _h) continue;
            for (int32_t col = 0; col < w; col++) {
                int32_t destX = x + col;
                if (destX < 0 || destX >= _w) continue;
                _pixel(destX, destY, data[row * w + col]);
            }
        }
    }

    void pushImage(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t* data) override {
        pushImage(x, y, w, h, (const uint16_t*)data);
    }

    void pushImage(int32_t x, int32_t y, int32_t w, int32_t h, const uint16_t* data, uint16_t transparent) override {
        if (!_isCreated || !_buffer || !data || w <= 0 || h <= 0) return;
        for (int32_t row = 0; row < h; row++) {
            int32_t destY = y + row;
            if (destY < 0 || destY >= _h) continue;
            for (int32_t col = 0; col < w; col++) {
                int32_t destX = x + col;
                if (destX < 0 || destX >= _w) continue;
                uint16_t c = data[row * w + col];
                if (c != transparent) _pixel(destX, destY, c);
            }
        }
    }

    void pushImage(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t* data, uint16_t transparent) override {
        pushImage(x, y, w, h, (const uint16_t*)data, transparent);
    }

    void pushSprite(int32_t x, int32_t y) {
        if (!_isCreated || !_buffer) return;
        if (_tft && _tft->isSprite()) {
            pushToSprite((TFT_eSprite*)_tft, x, y);
            return;
        }
        int16_t sw = display.width(), sh = display.height();
        if (x >= sw || y >= sh || x + _w <= 0 || y + _h <= 0) return;

        alignas(4) static uint16_t _sprLineBuf[480];
        for (int32_t row = 0; row < _h; row++) {
            int32_t curY = y + row;
            if (curY < 0 || curY >= sh) continue;

            int32_t srcStartX = 0;
            int32_t dstX = x;
            int32_t curW = _w;
            if (dstX < 0) {
                srcStartX = -dstX;
                curW += dstX;
                dstX = 0;
            }
            if (dstX + curW > sw) curW = sw - dstX;
            if (curW <= 0) continue;

            display.waitFlush();
            if (_bpp == 16) {
                const uint16_t* src = ((const uint16_t*)_buffer) + (row * _w) + srcStartX;
                for (int32_t i = 0; i < curW; i++) _sprLineBuf[i] = __builtin_bswap16(src[i]);
            } else if (_bpp == 8) {
                const uint8_t* src = _buffer + (row * _w) + srcStartX;
                for (int32_t i = 0; i < curW; i++) {
                    uint16_t c = color332to565(src[i]);
                    _sprLineBuf[i] = __builtin_bswap16(c);
                }
            } else {
                for (int32_t i = 0; i < curW; i++) {
                    int32_t sprX = srcStartX + i;
                    size_t byteIdx = ((size_t)row * _w + sprX) >> 1;
                    uint8_t idx = ((sprX & 1) == 0) ? (_buffer[byteIdx] >> 4) : (_buffer[byteIdx] & 0x0F);
                    uint16_t c = _palette[idx & 0x0F];
                    _sprLineBuf[i] = __builtin_bswap16(c);
                }
            }
            display.flushRegion(dstX, curY, dstX + curW - 1, curY, _sprLineBuf);
        }
        display.waitFlush();
    }

    void pushSprite(int32_t x, int32_t y, uint16_t transparent) {
        if (!_isCreated || !_buffer) return;
        if (_tft && _tft->isSprite()) {
            pushToSprite((TFT_eSprite*)_tft, x, y, transparent);
            return;
        }
        int16_t sw = display.width(), sh = display.height();
        if (x >= sw || y >= sh || x + _w <= 0 || y + _h <= 0) return;

        alignas(4) static uint16_t _transpLineBuf[480];
        for (int32_t row = 0; row < _h; row++) {
            int32_t curY = y + row;
            if (curY < 0 || curY >= sh) continue;

            int32_t runStart = -1;
            int32_t runLen = 0;

            for (int32_t col = 0; col < _w; col++) {
                int32_t curX = x + col;
                if (curX < 0 || curX >= sw) continue;

                uint16_t c = readPixel(col, row);
                if (c != transparent) {
                    if (runStart < 0) {
                        runStart = curX;
                        runLen = 0;
                    }
                    _transpLineBuf[runLen++] = __builtin_bswap16(c);
                } else {
                    if (runStart >= 0) {
                        display.waitFlush();
                        display.flushRegion(runStart, curY, runStart + runLen - 1, curY, _transpLineBuf);
                        runStart = -1;
                        runLen = 0;
                    }
                }
            }
            if (runStart >= 0) {
                display.waitFlush();
                display.flushRegion(runStart, curY, runStart + runLen - 1, curY, _transpLineBuf);
            }
        }
        display.waitFlush();
    }

    bool pushSprite(int32_t tx, int32_t ty, int32_t sx, int32_t sy, int32_t sw, int32_t sh) {
        if (!_isCreated || !_buffer) return false;
        if (sx < 0) { sw += sx; tx -= sx; sx = 0; }
        if (sy < 0) { sh += sy; ty -= sy; sy = 0; }
        if (sx + sw > _w)  sw = _w - sx;
        if (sy + sh > _h)  sh = _h - sy;
        if (sw <= 0 || sh <= 0) return false;

        alignas(4) static uint16_t _subLineBuf[480];
        int16_t dispW = display.width(), dispH = display.height();

        for (int32_t row = 0; row < sh; row++) {
            int32_t curDstY = ty + row;
            int32_t curSrcY = sy + row;
            if (curDstY < 0 || curDstY >= dispH) continue;

            int32_t lineStartX = 0;
            int32_t dstX = tx;
            int32_t curW = sw;
            if (dstX < 0) {
                lineStartX = -dstX;
                curW += dstX;
                dstX = 0;
            }
            if (dstX + curW > dispW) curW = dispW - dstX;
            if (curW <= 0) continue;

            display.waitFlush();
            for (int32_t i = 0; i < curW; i++) {
                uint16_t c = readPixel(sx + lineStartX + i, curSrcY);
                _subLineBuf[i] = __builtin_bswap16(c);
            }
            display.flushRegion(dstX, curDstY, dstX + curW - 1, curDstY, _subLineBuf);
        }
        display.waitFlush();
        return true;
    }

    bool pushToSprite(TFT_eSprite* dspr, int32_t x, int32_t y) {
        if (!dspr || !_isCreated || !_buffer || !dspr->created()) return false;
        for (int32_t row = 0; row < _h; row++) {
            int32_t dstY = y + row;
            if (dstY < 0 || dstY >= dspr->height()) continue;
            for (int32_t col = 0; col < _w; col++) {
                int32_t dstX = x + col;
                if (dstX < 0 || dstX >= dspr->width()) continue;
                dspr->drawPixel(dstX, dstY, readPixel(col, row));
            }
        }
        return true;
    }

    bool pushToSprite(TFT_eSprite* dspr, int32_t x, int32_t y, uint16_t transparent) {
        if (!dspr || !_isCreated || !_buffer || !dspr->created()) return false;
        for (int32_t row = 0; row < _h; row++) {
            int32_t dstY = y + row;
            if (dstY < 0 || dstY >= dspr->height()) continue;
            for (int32_t col = 0; col < _w; col++) {
                int32_t dstX = x + col;
                if (dstX < 0 || dstX >= dspr->width()) continue;
                uint16_t c = readPixel(col, row);
                if (c != transparent) dspr->drawPixel(dstX, dstY, c);
            }
        }
        return true;
    }

    bool pushRotated(int16_t angle, uint32_t transp = 0x00FFFFFF) {
        if (!_isCreated || !_buffer) return false;
        if (!_tft) return false;
        return pushRotated(_tft, angle, transp);
    }

    bool pushRotated(TFT_eSPI* dst, int16_t angle, uint32_t transp = 0x00FFFFFF) {
        if (!dst || !_isCreated || !_buffer) return false;
        while (angle < 0) angle += 360;
        angle = angle % 360;

        float rad = angle * 0.0174532925f; // DEG_TO_RAD
        float cosA = cosf(rad);
        float sinA = sinf(rad);

        int16_t srcPivotX = _pivotX;
        int16_t srcPivotY = _pivotY;
        int16_t dstPivotX = dst->getPivotX();
        int16_t dstPivotY = dst->getPivotY();

        float x0 = -srcPivotX,         y0 = -srcPivotY;
        float x1 = _w - srcPivotX - 1, y1 = -srcPivotY;
        float x2 = _w - srcPivotX - 1, y2 = _h - srcPivotY - 1;
        float x3 = -srcPivotX,         y3 = _h - srcPivotY - 1;

        float rx0 = x0 * cosA - y0 * sinA;
        float ry0 = x0 * sinA + y0 * cosA;
        float rx1 = x1 * cosA - y1 * sinA;
        float ry1 = x1 * sinA + y1 * cosA;
        float rx2 = x2 * cosA - y2 * sinA;
        float ry2 = x2 * sinA + y2 * cosA;
        float rx3 = x3 * cosA - y3 * sinA;
        float ry3 = x3 * sinA + y3 * cosA;

        float minX = std::min(std::min(rx0, rx1), std::min(rx2, rx3));
        float maxX = std::max(std::max(rx0, rx1), std::max(rx2, rx3));
        float minY = std::min(std::min(ry0, ry1), std::min(ry2, ry3));
        float maxY = std::max(std::max(ry0, ry1), std::max(ry2, ry3));

        int32_t startX = (int32_t)floorf(minX + dstPivotX);
        int32_t endX   = (int32_t)ceilf(maxX + dstPivotX);
        int32_t startY = (int32_t)floorf(minY + dstPivotY);
        int32_t endY   = (int32_t)ceilf(maxY + dstPivotY);

        int32_t dstW = dst->width();
        int32_t dstH = dst->height();
        if (startX < 0) startX = 0;
        if (startY < 0) startY = 0;
        if (endX >= dstW) endX = dstW - 1;
        if (endY >= dstH) endY = dstH - 1;

        for (int32_t dy = startY; dy <= endY; dy++) {
            float dyDiff = dy - dstPivotY;
            float baseSX = srcPivotX + dyDiff * sinA - dstPivotX * cosA;
            float baseSY = srcPivotY + dyDiff * cosA + dstPivotX * sinA;

            for (int32_t dx = startX; dx <= endX; dx++) {
                int32_t sx = (int32_t)(baseSX + dx * cosA + 0.5f);
                int32_t sy = (int32_t)(baseSY - dx * sinA + 0.5f);

                if (sx >= 0 && sx < _w && sy >= 0 && sy < _h) {
                    uint16_t c = readPixel(sx, sy);
                    if (transp == 0x00FFFFFF || c != (uint16_t)transp) {
                        dst->drawPixel(dx, dy, c);
                    }
                }
            }
        }
        return true;
    }

    void setRotation(uint8_t r) {
        r = r & 3;
        if (((_rotation & 1) != (r & 1)) && _isCreated) {
            std::swap(_w, _h);
            std::swap(_pivotX, _pivotY);
        }
        _rotation = r;
    }
    uint8_t getRotation() const { return _rotation; }

    void scroll(int16_t dx, int16_t dy = 0) {
        if (!_isCreated || !_buffer || (dx == 0 && dy == 0)) return;
        if (_bpp == 16) {
            uint16_t *buf = (uint16_t*)_buffer;
            if (dy != 0) {
                if (dy > 0) {
                    for (int32_t y = _h - 1; y >= dy; y--)
                        memcpy(&buf[y * _w], &buf[(y - dy) * _w], _w * 2);
                    for (int32_t y = 0; y < dy; y++)
                        memset(&buf[y * _w], 0, _w * 2);
                } else {
                    for (int32_t y = 0; y < _h + dy; y++)
                        memcpy(&buf[y * _w], &buf[(y - dy) * _w], _w * 2);
                    for (int32_t y = _h + dy; y < _h; y++)
                        memset(&buf[y * _w], 0, _w * 2);
                }
            }
            if (dx != 0) {
                for (int32_t y = 0; y < _h; y++) {
                    uint16_t *row = &buf[y * _w];
                    if (dx > 0) {
                        memmove(&row[dx], &row[0], (_w - dx) * 2);
                        memset(&row[0], 0, dx * 2);
                    } else {
                        memmove(&row[0], &row[-dx], (_w + dx) * 2);
                        memset(&row[_w + dx], 0, (-dx) * 2);
                    }
                }
            }
        } else if (_bpp == 8) {
            if (dy != 0) {
                if (dy > 0) {
                    for (int32_t y = _h - 1; y >= dy; y--)
                        memcpy(&_buffer[y * _w], &_buffer[(y - dy) * _w], _w);
                    for (int32_t y = 0; y < dy; y++)
                        memset(&_buffer[y * _w], 0, _w);
                } else {
                    for (int32_t y = 0; y < _h + dy; y++)
                        memcpy(&_buffer[y * _w], &_buffer[(y - dy) * _w], _w);
                    for (int32_t y = _h + dy; y < _h; y++)
                        memset(&_buffer[y * _w], 0, _w);
                }
            }
            if (dx != 0) {
                for (int32_t y = 0; y < _h; y++) {
                    uint8_t *row = &_buffer[y * _w];
                    if (dx > 0) {
                        memmove(&row[dx], &row[0], _w - dx);
                        memset(&row[0], 0, dx);
                    } else {
                        memmove(&row[0], &row[-dx], _w + dx);
                        memset(&row[_w + dx], 0, -dx);
                    }
                }
            }
        }
    }
};
