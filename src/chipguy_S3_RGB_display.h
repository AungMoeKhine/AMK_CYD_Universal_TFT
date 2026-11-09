/*
 * chipguy_ESP32S3_Touch_LCD_7 - Display driver for the Waveshare
 * ESP32-S3-Touch-LCD-7 board.
 *
 * A driver for the 800x480 ST7262 RGB LCD on the Waveshare
 * ESP32-S3-Touch-LCD-7 development board.  This panel is a plain RGB-interface
 * TFT with no command controller, so there is no SPI init sequence: the ESP32-S3
 * RGB LCD peripheral drives it directly.  The panel's reset and the LCD
 * backlight enable are reached through the board's CH422G I2C expander rather
 * than ESP32 GPIOs (see ESP32S3_Touch_LCD_7_Board).
 *
 * Copyright (c) 2025 chipguyhere
 * MIT License
 */

#ifndef CHIPGUY_S3_RGB_DISPLAY_H
#define CHIPGUY_S3_RGB_DISPLAY_H

#include <Arduino.h>
#include "esp_lcd_panel_io.h"
#include "AMK_S3_RGB_Board.h"   // shared I2C bus + expander (RST/BL)
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

// This display requires PSRAM for its framebuffers (two 800x480 RGB565 buffers,
// ~768 KB each). The Arduino ESP32 core defines BOARD_HAS_PSRAM only when PSRAM
// is enabled in Tools > PSRAM. Fail the build early with a clear message rather
// than letting framebuffer allocation fail at runtime.
#if !defined(BOARD_HAS_PSRAM)
#error "chipguy_ESP32S3_Touch_LCD_7_display requires PSRAM. In the Arduino IDE, set Tools > PSRAM to \"OPI PSRAM\" (Board: ESP32S3 Dev Module)."
#endif


// Sprite system constants
#define SPRITE_MAX_COUNT 16          // Maximum number of sprites (compile-time adjustable)
#define SPRITE_DISABLED  0x80000000  // yx value indicating sprite slot is disabled

// Sprite flags
#define SPRITE_FLAG_COLLISION_DETECT 0x00000001  // Enable collision detection for this sprite

// Sprite structure - compact for ISR efficiency
// All sprites are rendered in display coordinates (unaffected by scrolling)
//
// PERFORMANCE NOTE: Sprite pixel data is read by the bounce buffer ISR which runs
// at high priority during display refresh. For best performance, sprite data should
// be stored in SRAM (internal RAM), not in flash or PSRAM. Reading from flash or
// PSRAM during the ISR adds significant latency due to cache misses and bus
// contention, which may cause display artifacts or timing issues with many sprites.
//
// Recommended: Declare sprite data as static arrays or use heap_caps_malloc()
// with MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT for SRAM allocation.
struct Sprite {
    const uint16_t* data;   // Pointer to RGB565 pixel data (SRAM recommended, see note above)
    uint32_t yx;            // Y in upper 16 bits, X in lower 16 bits (signed coordinates)
    uint32_t hw;            // Height in upper 16 bits, Width in lower 16 bits
    uint16_t transparent;   // Color value to skip (transparency key)
    uint16_t _reserved;     // Padding for 32-bit alignment
    uint32_t flags;         // Reserved for future use (e.g., flip, rotate, blend modes)
};



// Normalize a rotation argument to degrees.  Accepts degrees (0/90/180/270)
// directly, and the index aliases 1/2/3 as 90/180/270 respectively.  Anything
// else passes through unchanged (treated as no rotation downstream).
static inline uint16_t cglcd7_normalize_rotation(uint16_t rotation) {
    switch (rotation) {
        case 1: return 90;
        case 2: return 180;
        case 3: return 270;
        default: return rotation;
    }
}

static inline uint8_t cydr_normalize_rotation(uint16_t rotation) {
    switch (rotation) {
        case 1:
        case 90:  return 1;
        case 2:
        case 180: return 2;
        case 3:
        case 270: return 3;
        default:  return (uint8_t)(rotation % 4);
    }
}

class chipguy_S3_RGB_display {
public:
    // Constructor.  Takes the board that owns the shared I2C bus and the
    // CH422G expander; the display's reset and backlight are reached through
    // it.  fb_pad_x/fb_pad_y are optional pre-draw scroll padding.
    chipguy_S3_RGB_display(AMK_S3_Board& board = amk_get_default_board(),
                        uint16_t fb_pad_x = 20, uint16_t fb_pad_y = 60);

    // Initialize the display.
    // rotation: 0 = normal, 180 = flipped (for a panel mounted upside-down).
    //           The index aliases 1/2/3 are accepted for 90/180/270.  Only 0 and
    //           180 are handled here; 90/270 are intended to be done at the
    //           LVGL->framebuffer level (so the driver keeps reading the
    //           framebuffer forward/sequentially), not by this driver.
    bool begin(uint32_t pclk_hz = 16000000, uint16_t rotation = 0);
    // 21000000 is near the practical upper limit for this panel/timing
    // 16000000 is Waveshare's default for the ST7262 800x480 panel
    //  Lower pclk reduces ISR/bus load at the cost of refresh rate
    // More FPS will load up the application core with rendering burden

    // Backlight control.  On this board the LCD backlight is a CH422G expander
    // pin with no PWM, so it is on/off only: any percentage > 0 turns it on.
    void setBacklight(int percentage);
    void setBacklight(bool on);

    // Get framebuffer pointer for direct access
    uint16_t* getFramebuffer() { return _framebuffer; }

    // Double buffering support (optional)
    // Allocates a second framebuffer for tear-free rendering
    // Returns true on success, false if allocation fails or already allocated
    bool allocateSecondFramebuffer();

    // Get second framebuffer pointer (nullptr if not allocated)
    uint16_t* getFramebuffer2() { return _framebuffer2; }

    // Set which framebuffer to draw to (1 = primary, 2 = secondary)
    // Drawing functions (fillRect, pushImage, drawPixel) will use this buffer
    // Default is primary (1). Call after allocateSecondFramebuffer().
    void setDrawBuffer(uint8_t index);

    // Get current draw buffer index (1 or 2)
    uint8_t getDrawBuffer() const { return _draw_fb_index; }

    // Get pointer to current draw target buffer
    uint16_t* getDrawTarget() { return _draw_target; }

    // Set which framebuffer becomes active at next vsync
    // index: 0 = swap to the other buffer, 1 = primary, 2 = secondary
    // Takes effect when ISR requests row 0 of the next frame
    // If wait=true, blocks until the swap has occurred (safe to draw to other buffer)
    void setActiveFramebuffer(uint8_t index, bool wait = false);

    // Get display dimensions
    uint16_t width() const { return (_rotation == 1 || _rotation == 3) ? _height : _width; }
    uint16_t height() const { return (_rotation == 1 || _rotation == 3) ? _width : _height; }
    int16_t nativeWidth() const { return _width; }
    int16_t nativeHeight() const { return _height; }

    // Universal compatibility wrappers for TFT_eSPI & AMK_CYD_Universal_TFT
    void flushRegion(int16_t x0, int16_t y0, int16_t x1, int16_t y1, const uint16_t* pixels, bool swapBytes = false) {
        (void)swapBytes;
        pushImage(x0, y0, x1 - x0 + 1, y1 - y0 + 1, pixels);
    }
    void waitFlush() { /* Continuous RGB DMA refresh */ }
    void setRotation(uint8_t r) {
        _rotation = r % 4;
        _rotate180 = (_rotation == 2 || _rotation == 180);
    }
    uint8_t rotation() const { return _rotation; }
    void setBGR(bool bgr) { (void)bgr; }
    bool isBGR() const { return false; }
    void setSwapBytes(bool swap) { (void)swap; }
    bool getSwapBytes() const { return false; }
    void invertDisplay(bool invert) { (void)invert; }
    void setBacklightPin(uint8_t pin) { (void)pin; }
    uint8_t backlightPin() const { return 0; }
    void setMirrorX(bool mirror) { (void)mirror; }
    bool isMirrorX() const { return false; }
    void setPanel(cydr_panel_t p) { (void)p; }
    cydr_panel_t panel() const { return CYDR_PANEL_ST7262; }
    cydr_board_t board() const { return CYDR_BOARD_S3_RGB; }
    const char* panelName() const { return "ST7262"; }
    const char* modelName() const { return "ESP32-S3-RGB-800x480"; }
    const char* boardName() const { return "Waveshare/Sunton S3 RGB"; }
    uint8_t diagonalTenths() const { return 70; }
    const char* readbackName() const { return "Direct Framebuffer"; }
    uint8_t redLedPin() const { return 255; }
    uint8_t greenLedPin() const { return 255; }
    uint8_t blueLedPin() const { return 255; }

    // Copy RGB565 pixels to framebuffer region
    void copyPixels(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t* pixels) {
        pushImage(x,y,w,h,pixels);
    }

    void pushImage(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t* pixels);

    // Fill rectangle with solid color
    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);

    // Fill entire screen with color
    void fillScreen(uint16_t color);

    // Draw a single pixel
    void drawPixel(int16_t x, int16_t y, uint16_t color);

    // Flush cache for entire framebuffer
    void flush();

    // Scroll framebuffer by dx,dy pixels, filling exposed area with fill_color
    // Returns true if fill was performed, false if caller should handle exposed area
    bool scrollFramebuffer(int16_t dx, int16_t dy, uint16_t fill_color);

    // Hardware scroll - moves the virtual origin without copying memory
    // After scrolling, you should fill the newly exposed area
    void setScrollOffset(int16_t x, int16_t y);
    uint32_t getScrollOffset() { return _pending_scroll_offset; }
    int16_t getScrollX() { return _pending_scroll_offset % _fb_stride; }
    int16_t getScrollY() { return _pending_scroll_offset / _fb_stride; }

    // Scroll by delta, optionally fill exposed area
    // If fill=true and scroll fits in padding, pre-draws fill before scroll (no tearing)
    // Any fill that doesn't fit in padding is post-drawn after scroll
    void scroll(int16_t dx, int16_t dy, uint16_t fill_color = 0, bool fill = true);

    // Sprite system - hardware-accelerated overlays rendered by ISR
    // Sprites are drawn in display coordinates (unaffected by scrolling)
    // Changes take effect at the next frame boundary (vsync)

    // Set a sprite's properties. The sprite will appear at the next frame.
    // slot: 0 to SPRITE_MAX_COUNT-1
    // data: pointer to RGB565 pixel data (must remain valid; ideally in SRAM)
    // x, y: top-left position in display coordinates (can be negative for partial offscreen)
    // w, h: dimensions in pixels
    // transparent: color value to treat as transparent (pixels with this color are skipped)
    // flags: sprite flags (e.g., SPRITE_FLAG_COLLISION_DETECT)
    void setSprite(uint8_t slot, const uint16_t* data, int16_t x, int16_t y,
                   uint16_t w, uint16_t h, uint16_t transparent = 0xF81F,
                   uint32_t flags = 0);

    // Disable a sprite slot (sprite will disappear at next frame)
    void clearSprite(uint8_t slot);

    // Disable all sprites
    void clearAllSprites();

    // Get pointer to pending sprite array for advanced manipulation
    // Note: modify with care; changes take effect at next frame
    Sprite* getPendingSprites() { return _pending_sprites; }

    // Check if a sprite collision was detected and clear the flag
    // Returns true if a collision occurred since the last check
    // Collision detection must be enabled via SPRITE_FLAG_COLLISION_DETECT flag
    // A collision is detected when a sprite overwrites more than one unique color
    bool checkSpriteCollision(uint8_t slot);

    // Flash write overlay: display a "FLASH MEMORY WRITE" message during flash
    // operations that stall PSRAM access. Call beginFlashMessage() before the
    // flash write and endFlashMessage() after. The message is pre-rendered into
    // an SRAM buffer so the ISR can display it without touching PSRAM or flash.
    // The ISR renders a black screen with the message positioned vertically.
    //
    // y: Approximate vertical pixel position for the top of the message.
    //    Rounded down to the nearest bounce buffer boundary (multiple of 20 lines).
    //    Default (-2) centers the message vertically on screen.
    //    -1 blanks the screen without displaying any message text.
    //    -3 tiles the message on every bounce buffer band, so it remains visible
    //       even if the DMA replays a single chunk during a flash stall.
    void beginFlashMessage(int16_t y = -3);
    void endFlashMessage();

private:
    // The board that owns the shared I2C bus and CH422G expander.  The ST7262's
    // reset and the LCD backlight are driven through it (see begin/setBacklight).
    AMK_S3_Board& _board;

    // Friend function for bounce buffer callback (needs access to scroll offsets)
    friend bool bounceBufferFillCallback(esp_lcd_panel_handle_t, void*, int, int, void*);

    // The heavy PSRAM->bounce-buffer copy (and sprite overlay) runs in a
    // dedicated high-priority task instead of the GDMA ISR, so the ISR stays
    // short and does not block other interrupts on its core.  The ISR
    // (bounceBufferFillCallback) still does the cheap, time-critical work --
    // frame-start latching, the desync detection/redirect, and the flash-overlay
    // fill -- then hands the chosen buffer to this task via _bounceQueue.
    // _bounceFillTask drains the queue and performs the copy.
    static void _bounceFillTask(void* arg);
    void _fillBounceBuffer(void* bounce_buf, int pos_px, int len_bytes,
                           uint16_t* framebuffer, uint32_t scroll_offset);
    // Sprite overlay, factored out of _fillBounceBuffer.
    void _overlaySprites(void* bounce_buf, int pos_px, int len_bytes);
    QueueHandle_t _bounceQueue = nullptr;   // ISR -> fill-task hand-off
    TaskHandle_t  _bounceTask  = nullptr;   // the high-priority fill task

    // Cooperative dual-core fill (LCD7_BOUNCE_FILL_COOP, opt-in).  A low-priority
    // worker on the WiFi/PRO core does the copy opportunistically; a checkpoint
    // timer lets a high-priority backstop on the app core finish the remainder if
    // the worker stalled.  Both advance one shared atomic cursor over the chunk,
    // so overlapping copies are the harmless idempotent same-src->same-dst writes.
    // (Definitions are compiled only when the flag is on; declared here always so
    // the header needs no knowledge of the flag.)
    void _coopDrive(uint32_t myGen);          // claim+copy blocks until chunk done
    static void _coopWorkerTask(void* arg);   // core 0, low priority
    static void _coopBackstopTask(void* arg); // core 1, high priority (timer-woken)

    // Display properties (visible screen size)
    static const uint16_t _width = 800;
    static const uint16_t _height = 480;

    // 180-degree rotation, fixed at begin().  When true, the bounce-buffer fill
    // presents physical pixel p as logical pixel (_width*_height - 1 - p): the
    // PSRAM framebuffer is still read forward/sequentially, but written into the
    // SRAM bounce buffer backward.  Sprites and the flash-write overlay are
    // flipped to match.  Immutable after begin(), so it needs no latching.
    bool _rotate180 = false;
    uint8_t _rotation = 0;

    // Bounce buffer size in pixels.  The ESP32-S3 RGB driver streams the
    // framebuffer through two SRAM bounce buffers; this is the size of each.
    // It must divide LCD_width*LCD_height an even number of times.  We use
    // width*20 = 16000 px (20 scanlines): 800*480 / 16000 = 24 (even).  Two
    // buffers then span 40 lines, which keeps the no-PSRAM / flash-overlay
    // banding math the same as the 2.8C driver this is derived from.
    static const int _bounce_buffer_size_px = _width * 20;

    // Framebuffer layout - linear ring buffer with row stride
    // Each screen row is _width (800) contiguous pixels
    // Rows are spaced _fb_stride apart, leaving _fb_pad_x pixel gaps for pre-draw
    // Extra rows at the end provide vertical pre-draw space
    uint16_t _fb_pad_x;    // Horizontal padding per row
    uint16_t _fb_pad_y;    // Extra rows for vertical padding
    uint16_t _fb_stride;   // Row stride in pixels (_width + _fb_pad_x)
    uint16_t _fb_rows;     // Total rows (_height + _fb_pad_y)
    uint32_t _fb_size;     // Total buffer size (_fb_stride * _fb_rows)

    // Framebuffers
    uint16_t* _framebuffer = nullptr;   // Primary framebuffer (always allocated)
    uint16_t* _framebuffer2 = nullptr;  // Secondary framebuffer (optional, for double buffering)
    size_t _framebuffer_size;

    // Draw target - which buffer drawing operations use
    uint16_t* _draw_target = nullptr;   // Points to either _framebuffer or _framebuffer2
    uint8_t _draw_fb_index = 1;         // Which buffer is the draw target (1 or 2)

    // Double buffering state
    // Active is read by ISR, pending is written by main code
    // Values: 1 = primary (_framebuffer), 2 = secondary (_framebuffer2)
    volatile uint8_t _active_fb = 1;    // Currently displayed framebuffer
    volatile uint8_t _pending_fb = 1;   // Framebuffer to display at next vsync

    // Scroll offset - linear position in the ring buffer
    // Double buffered: pending is set by user, copied to active at frame start
    // Screen pixel (x,y) maps to fb[(scroll_offset + y*fb_stride + x) % fb_size]
    volatile uint32_t _scroll_offset = 0;         // Active offset (ISR reads this)
    volatile uint32_t _pending_scroll_offset = 0; // Pending offset (main code writes this)

    // LCD panel handle
    esp_lcd_panel_handle_t _panel_handle = nullptr;

    // Sprite arrays - double buffered for tear-free updates
    // Application writes to _pending_sprites, ISR reads from _active_sprites
    // Copy occurs at frame start (pos_px == 0)
    Sprite _pending_sprites[SPRITE_MAX_COUNT];
    Sprite _active_sprites[SPRITE_MAX_COUNT];

    // Collision detection array - one 32-bit word per sprite slot
    // ISR sets (never clears) when sprite with SPRITE_FLAG_COLLISION_DETECT
    // overwrites more than one unique color during rendering
    volatile uint32_t _sprite_collisions[SPRITE_MAX_COUNT];
};

// Hardware and backward-compatibility aliases
typedef chipguy_S3_RGB_display ESP32S3_Touch_LCD_7;
typedef chipguy_S3_RGB_display chipguy_ESP32S3_Touch_LCD_7_display;
typedef chipguy_S3_RGB_display ESP32S3_Touch_LCD_7_display;
typedef chipguy_S3_RGB_display AMK_S3_RGB_display;
typedef chipguy_S3_RGB_display ESP32S3_RGB_display;

// ISR timing instrumentation (cycle counts at 240MHz CPU)
// Use these to measure bounce buffer callback overhead
extern volatile uint64_t g_isrCyclesIn;   // Total cycles spent inside callback
extern volatile uint64_t g_isrCyclesOut;  // Total cycles spent outside callback
extern volatile uint32_t g_isrCallCount;  // Number of callback invocations
extern volatile uint32_t g_bbDesyncCount; // Number of bounce buffer desync events detected
extern volatile uint32_t g_bbDroppedCount;// Fill requests dropped because the fill task could not keep up (queue full)

// Diagnostic: last GDMA state seen by ISR (for debugging desync detection)
extern volatile uint32_t g_bbDiag_dscr;      // Last descriptor address from GDMA out.dscr
extern volatile uint32_t g_bbDiag_bufAddr;   // Buffer address extracted from that descriptor
extern volatile uint32_t g_bbDiag_bounceBuf; // bounce_buf pointer callback was asked to fill



#endif // CHIPGUY_S3_RGB_DISPLAY_H
