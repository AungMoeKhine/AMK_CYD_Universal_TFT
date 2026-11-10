#pragma once
#ifndef AMK_IMAGE_DECODER_H
#define AMK_IMAGE_DECODER_H
// ═══════════════════════════════════════════════════════════════════════════════
// AMK_Image_Decoder.h - Universal 1-Line Image Rendering Companion
//                      for AMK_CYD_Universal_TFT (v1.6.4+)
//
// Features:
//   - Zero-boilerplate 1-line JPEG decoding via JPEGDecoder / picojpeg
//   - Zero-boilerplate 1-line PNG decoding via PNGdec
//   - MCU-block and scanline streaming directly into pushImage()
//   - Zero RAM overhead: no full-image frame buffering required for decoding
//   - Universal support across Classic ESP32 CYD (2.4"-4.0") & ESP32-S3 RGB (4.3"-7.0")
//   - Seamless filesystem integration: LittleFS, SPIFFS, SD, SD_MMC
// ═══════════════════════════════════════════════════════════════════════════════

#include <Arduino.h>
#include <FS.h>

#if __has_include(<JPEGDecoder.h>)
#include <JPEGDecoder.h>
#define AMK_HAS_JPEG_DECODER 1
#endif

#if __has_include(<PNGdec.h>)
#include <PNGdec.h>
#define AMK_HAS_PNG_DECODER 1
#endif

// ── In-Memory & Filesystem JPEG Decoding Implementation ──
inline bool TFT_eSPI::drawJpg(const uint8_t *data, uint32_t len, int16_t x, int16_t y, uint16_t maxWidth, uint16_t maxHeight) {
#if defined(AMK_HAS_JPEG_DECODER)
    if (!data || len == 0) return false;
    if (JpegDec.decodeArray(data, len) == 0) return false;

    uint16_t *pImg;
    uint16_t mcu_w = JpegDec.MCUWidth;
    uint16_t mcu_h = JpegDec.MCUHeight;
    uint32_t max_x = maxWidth > 0 ? (x + maxWidth) : (x + JpegDec.width);
    uint32_t max_y = maxHeight > 0 ? (y + maxHeight) : (y + JpegDec.height);

    while (JpegDec.read()) {
        pImg = JpegDec.pImage;
        int mcu_x = JpegDec.MCUx * mcu_w + x;
        int mcu_y = JpegDec.MCUy * mcu_h + y;

        if (mcu_x < max_x && mcu_y < max_y) {
            uint32_t win_w = (mcu_x + mcu_w <= max_x) ? mcu_w : (max_x - mcu_x);
            uint32_t win_h = (mcu_y + mcu_h <= max_y) ? mcu_h : (max_y - mcu_y);
            this->pushImage(mcu_x, mcu_y, win_w, win_h, pImg);
        }
    }
    return true;
#else
    (void)data; (void)len; (void)x; (void)y; (void)maxWidth; (void)maxHeight;
    Serial.println(F("[AMK Image] JPEGDecoder not installed. Add 'JPEGDecoder' via Library Manager."));
    return false;
#endif
}

inline bool TFT_eSPI::drawJpgFile(fs::FS &fs, const char *path, int16_t x, int16_t y, uint16_t maxWidth, uint16_t maxHeight) {
#if defined(AMK_HAS_JPEG_DECODER)
    if (!path) return false;
    File file = fs.open(path, "r");
    if (!file) {
        Serial.printf("[AMK Image] Failed to open JPEG file: %s\n", path);
        return false;
    }
    int res = JpegDec.decodeFsFile(file);
    if (res == 0) {
        file.close();
        return false;
    }

    uint16_t *pImg;
    uint16_t mcu_w = JpegDec.MCUWidth;
    uint16_t mcu_h = JpegDec.MCUHeight;
    uint32_t max_x = maxWidth > 0 ? (x + maxWidth) : (x + JpegDec.width);
    uint32_t max_y = maxHeight > 0 ? (y + maxHeight) : (y + JpegDec.height);

    while (JpegDec.read()) {
        pImg = JpegDec.pImage;
        int mcu_x = JpegDec.MCUx * mcu_w + x;
        int mcu_y = JpegDec.MCUy * mcu_h + y;

        if (mcu_x < max_x && mcu_y < max_y) {
            uint32_t win_w = (mcu_x + mcu_w <= max_x) ? mcu_w : (max_x - mcu_x);
            uint32_t win_h = (mcu_y + mcu_h <= max_y) ? mcu_h : (max_y - mcu_y);
            this->pushImage(mcu_x, mcu_y, win_w, win_h, pImg);
        }
    }
    file.close();
    return true;
#else
    (void)fs; (void)path; (void)x; (void)y; (void)maxWidth; (void)maxHeight;
    Serial.println(F("[AMK Image] JPEGDecoder not installed. Add 'JPEGDecoder' via Library Manager."));
    return false;
#endif
}

// ── In-Memory & Filesystem PNG Decoding Implementation ──
#if defined(AMK_HAS_PNG_DECODER)
struct AMK_PNG_DrawContext {
    TFT_eSPI *tft;
    int16_t x;
    int16_t y;
    uint16_t maxWidth;
    uint16_t maxHeight;
};

inline void amk_png_draw_scanline(PNGDRAW *pDraw) {
    AMK_PNG_DrawContext *ctx = (AMK_PNG_DrawContext*)pDraw->pUser;
    if (!ctx || !ctx->tft) return;

    int16_t draw_y = ctx->y + pDraw->y;
    if (ctx->maxHeight > 0 && (draw_y >= ctx->y + ctx->maxHeight)) return;

    uint16_t line_buffer[pDraw->iWidth];
    pDraw->pfnGetLine(pDraw, line_buffer, PNG_RGB565_LITTLE_ENDIAN, 0xffffffff);

    uint16_t draw_w = (ctx->maxWidth > 0 && pDraw->iWidth > ctx->maxWidth) ? ctx->maxWidth : pDraw->iWidth;
    ctx->tft->pushImage(ctx->x, draw_y, draw_w, 1, line_buffer);
}
#endif

inline bool TFT_eSPI::drawPng(const uint8_t *data, uint32_t len, int16_t x, int16_t y, uint16_t maxWidth, uint16_t maxHeight) {
#if defined(AMK_HAS_PNG_DECODER)
    if (!data || len == 0) return false;
    PNG png;
    int rc = png.openRAM((uint8_t*)data, len, amk_png_draw_scanline);
    if (rc != PNG_SUCCESS) return false;

    AMK_PNG_DrawContext ctx = { this, x, y, maxWidth, maxHeight };
    rc = png.decode(&ctx, 0);
    png.close();
    return (rc == PNG_SUCCESS);
#else
    (void)data; (void)len; (void)x; (void)y; (void)maxWidth; (void)maxHeight;
    Serial.println(F("[AMK Image] PNGdec not installed. Add 'PNGdec' via Library Manager."));
    return false;
#endif
}

inline bool TFT_eSPI::drawPngFile(fs::FS &fs, const char *path, int16_t x, int16_t y, uint16_t maxWidth, uint16_t maxHeight) {
#if defined(AMK_HAS_PNG_DECODER)
    if (!path) return false;
    File file = fs.open(path, "r");
    if (!file) return false;

    size_t size = file.size();
    if (size == 0) { file.close(); return false; }

    uint8_t *buf = (uint8_t*)malloc(size);
    if (!buf) { file.close(); return false; }

    file.read(buf, size);
    file.close();

    bool ok = drawPng(buf, size, x, y, maxWidth, maxHeight);
    free(buf);
    return ok;
#else
    (void)fs; (void)path; (void)x; (void)y; (void)maxWidth; (void)maxHeight;
    Serial.println(F("[AMK Image] PNGdec not installed. Add 'PNGdec' via Library Manager."));
    return false;
#endif
}

#endif // AMK_IMAGE_DECODER_H
