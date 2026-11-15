# Upstream Maintenance & Chipguy Update Guide

This document records all modifications, extensions, and compatibility wrappers added to the upstream `chipguy` drivers within **AMK_CYD_Universal_TFT**.

If you pull a new release or update files from upstream (`chipguyhere`), **refer to this checklist** to re-apply these additions and prevent compilation errors or color corruption.

---

## 📋 Quick Reference: Common Upstream Update Errors & Fixes

When upgrading upstream `chipguy` driver files, the build will halt if the following methods are missing:

| Compilation Error | Missing Method / Variable | Required Location |
| :--- | :--- | :--- |
| `'class chipguy_CYD_resistive_display' has no member named 'setSwapBytes'` | `void setSwapBytes(bool swap)`<br>`bool getSwapBytes() const` | `src/chipguy_CYD_resistive_display.h` |
| `'class chipguy_CYD_resistive_display' has no member named 'setBGR'` | `void setBGR(bool bgr)`<br>`bool isBGR() const` | `src/chipguy_CYD_resistive_display.h` |
| `'class chipguy_CYD_resistive_display' has no member named 'setMirrorX'` | `void setMirrorX(bool mirror)`<br>`bool isMirrorX() const` | `src/chipguy_CYD_resistive_display.h` |
| `'class chipguy_CYD_resistive_display' has no member named 'invertDisplay'` | `void invertDisplay(bool invert)` | `src/chipguy_CYD_resistive_display.h` |
| `'class chipguy_CYD_resistive_display' has no member named 'setBacklightPin'` | `void setBacklightPin(uint8_t pin)`<br>`int8_t backlightPin() const` | `src/chipguy_CYD_resistive_display.h` |

---

## 🛠️ Detailed Code Patches to Preserve

### 1. `src/chipguy_CYD_resistive_display.h`

In `class chipguy_CYD_resistive_display`, ensure the following block is present inside `public:`:

```cpp
    // ═════════════════════════════════════════════════════════════════════════
    // AMK_CYD_Universal_TFT Compatibility Wrappers
    // (Preserve when updating from upstream chipguy)
    // ═════════════════════════════════════════════════════════════════════════
    void setBGR(bool bgr)             { setColorOrder(bgr); }
    bool isBGR() const                { return colorBGR(); }
    void setMirrorX(bool mirror)      { setMirror(mirror, _my0); }
    bool isMirrorX() const            { return mirrorX(); }
    void invertDisplay(bool invert)   { setInversion(invert); }
    void setBacklightPin(uint8_t pin) { _blPin = pin; pinMode(_blPin, OUTPUT); }
    int8_t backlightPin() const       { return _blPin; }
    void setSwapBytes(bool swap)      { (void)swap; }
    bool getSwapBytes() const         { return false; }
```

> [!IMPORTANT]
> **Byte Swapping Isolation (CRITICAL FOR COLOR FIDELITY):**
> * In `chipguy_CYD_resistive_display.h`, `setSwapBytes(bool)` is deliberately a no-op `(void)swap;`.
> * In `chipguy_CYD_resistive_display::flushRegion()`, byte swapping is ONLY performed when the caller explicitly passes `swapBytes = true` (which `AMK_CYD_LVGL` does for LVGL frames).
> * In `TFT_eSPI`, `setSwapBytes(bool)` only swaps image bytes for `pushImage()`.
> * **DO NOT** make `setSwapBytes()` globally swap all pixels in `flushRegion()`! Doing so would invert colors for all standard drawing primitives (`fillRect`, `drawPixel`, `pushSprite`), turning dark charcoal cards purple and red into blue.

---

### 2. `src/chipguy_S3_RGB_display.h`

For ESP32-S3 parallel RGB displays, ensure the following API parity stubs exist so user code runs seamlessly across both classic CYD and S3 RGB boards:

```cpp
    void setBGR(bool bgr) { (void)bgr; }
    bool isBGR() const { return false; }
    void setSwapBytes(bool swap) { (void)swap; }
    bool getSwapBytes() const { return false; }
    void invertDisplay(bool invert) { (void)invert; }
    void setBacklightPin(uint8_t pin) { (void)pin; }
    uint8_t backlightPin() const { return 0; }
    void setMirrorX(bool mirror) { (void)mirror; }
    bool isMirrorX() const { return false; }
```

---

## 📌 Architecture Rationale: Why These Wrappers Exist

1. **`AMK_CYD_LVGL` Integration:**
   LVGL generates RGB565 pixels in little-endian format. The display controller expects big-endian format over the SPI bus. `_display->flushRegion(..., true)` explicitly requests byte-swapping during DMA flushes.
2. **`TFT_eSPI` Drop-in Parity:**
   Existing Arduino sketches frequently call `tft.setBGR(true)`, `tft.invertDisplay(true)`, or `tft.setSwapBytes(true)`. By exposing these methods directly, sketches transition between `TFT_eSPI` and `AMK_display` with zero friction.
3. **PSRAM Isolation:**
   - **ESP32-S3 RGB Displays:** Require 8MB/16MB PSRAM for two 800×480 framebuffers (~1.5 MB). The compile-time `#error` check in `chipguy_S3_RGB_display.h` is intentional to prevent runtime memory panics.
   - **Classic ESP32 CYD Boards:** Must **never** be gated by global PSRAM checks, as standard 2.8" and 2.4" CYD boards do not have external PSRAM chips.

---

### 3. ESP32-S3 RGB Display Parity & Direct GPIO Fallback (v1.6.3)

In src/chipguy_S3_RGB_display.h, ensure the following methods are maintained for unified dimension querying and 6-parameter lushRegion:
`cpp
    int16_t nativeWidth() const { return _width; }
    int16_t nativeHeight() const { return _height; }
    void flushRegion(int16_t x0, int16_t y0, int16_t x1, int16_t y1, const uint16_t* pixels, bool swapBytes = false) {
        (void)swapBytes;
        pushImage(x0, y0, x1 - x0 + 1, y1 - y0 + 1, pixels);
    }
`

In src/AMK_S3_RGB_Board.h, preserve the I2C probing of CH422G (CH422G_I2C_ADDR_CFG = 0x24) with direct GPIO fallback for Sunton ESP32-8048S050 and Guition JC8048W550C boards:
`cpp
    Wire.beginTransmission(CH422G_I2C_ADDR_CFG);
    hasExpander = (Wire.endTransmission() == 0);
    if (!hasExpander) {
        pinMode(2, OUTPUT);
        digitalWrite(2, HIGH); // Backlight direct GPIO 2
    }
`

---

### 4. Synchronized Scanline Rendering Pipeline in src/AMK_CYD_Universal_TFT.h (v1.6.3)

Always preserve strict DMA wait synchronization (display.waitFlush()) prior to writing to _lineBuf, _imgLineBuf, and _sprLineBuf scanline buffers across _fill, pushImage, and pushSprite. Asynchronous multi-row chunk buffer mutation without hardware ping-pong buffers causes mid-transfer memory corruption, vertical shift, and visual card duplication on CYD SPI panels.