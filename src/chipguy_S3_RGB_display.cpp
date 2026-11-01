/*
 * chipguy_ESP32S3_Touch_LCD_7 - Display driver for the Waveshare
 * ESP32-S3-Touch-LCD-7 board (ST7262 800x480 RGB LCD).
 *
 * The panel is a plain RGB-interface TFT with no command controller, so unlike
 * the ST7701-based 2.8C board there is no SPI bring-up sequence: the ESP32-S3
 * RGB LCD peripheral drives the panel directly.  The panel reset and the LCD
 * backlight enable are reached through the board's CH422G I2C expander.
 *
 * Copyright (c) 2025 chipguyhere
 * MIT License
 */

#include <Arduino.h>

#if defined(CONFIG_IDF_TARGET_ESP32S3) || defined(BOARD_ESP32_S3_RGB) || defined(BOARD_ESP32_S3_TOUCH_7) || defined(ARDUINO_WAVESHARE_ESP32_S3_TOUCH_LCD_7)

#include "chipguy_S3_RGB_display.h"
#include "esp_heap_caps.h"
#include "esp_cpu.h"
#include "freertos/portmacro.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "esp_lcd_panel_rgb.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_private/gdma.h"
#include "hal/lcd_hal.h"
#include "hal/lcd_ll.h"
#include "soc/gdma_struct.h"
#include "hal/gdma_ll.h"

// Cache writeback function from ROM
#include "esp32s3/rom/cache.h"
extern "C" int Cache_WriteBack_Addr(uint32_t addr, uint32_t size);


// Helper macros for sprite coordinate/dimension packing
#define SPRITE_YX(y, x)     (((uint32_t)(uint16_t)(y) << 16) | (uint16_t)(x))
#define SPRITE_HW(h, w)     (((uint32_t)(uint16_t)(h) << 16) | (uint16_t)(w))
#define SPRITE_GET_Y(yx)    ((int16_t)((yx) >> 16))
#define SPRITE_GET_X(yx)    ((int16_t)((yx) & 0xFFFF))
#define SPRITE_GET_H(hw)    ((uint16_t)((hw) >> 16))
#define SPRITE_GET_W(hw)    ((uint16_t)((hw) & 0xFFFF))

// Pin definitions for the Waveshare ESP32-S3-Touch-LCD-7 board.
//
// This is a 16-bit parallel RGB panel (ST7262).  The panel reset and backlight
// are NOT on ESP32 GPIOs -- they hang off the CH422G I2C expander (LCD reset =
// IO3, backlight = IO2), driven via ESP32S3_Touch_LCD_7_Board.  There is no
// chip-select or SPI: the RGB sync/data lines below are the entire interface.
#define PIN_LCD_DE       5
#define PIN_LCD_VSYNC    3
#define PIN_LCD_HSYNC    46
#define PIN_LCD_PCLK     7
// RGB565 data lines.  data_gpio_nums[0..15] = B0..B4, G0..G5, R0..R4.
#define PIN_LCD_B0       14
#define PIN_LCD_B1       38
#define PIN_LCD_B2       18
#define PIN_LCD_B3       17
#define PIN_LCD_B4       10
#define PIN_LCD_G0       39
#define PIN_LCD_G1       0
#define PIN_LCD_G2       45
#define PIN_LCD_G3       48
#define PIN_LCD_G4       47
#define PIN_LCD_G5       21
#define PIN_LCD_R0       1
#define PIN_LCD_R1       2
#define PIN_LCD_R2       42
#define PIN_LCD_R3       41
#define PIN_LCD_R4       40


// Cache writeback helper - writes back a memory region with interrupt protection
// The ISR reads through the same CPU cache, so it sees dirty cache lines automatically.
// This writeback may be unnecessary - disable by uncommenting the return statement.
static portMUX_TYPE s_cache_mux = portMUX_INITIALIZER_UNLOCKED;
static inline void cacheWriteBack(const void* addr, size_t size) {
    return;  // DISABLED FOR TESTING - ISR should see dirty cache lines directly
    portENTER_CRITICAL(&s_cache_mux);
    Cache_WriteBack_Addr((uint32_t)addr, size);
    portEXIT_CRITICAL(&s_cache_mux);
}

// ISR timing instrumentation (cycle counts at 240MHz)
volatile uint64_t g_isrCyclesIn = 0;      // Total cycles spent inside ISR
volatile uint64_t g_isrCyclesOut = 0;     // Total cycles spent outside ISR
volatile uint32_t g_isrLastExit = 0;      // Cycle count when ISR last exited (32-bit is fine for deltas)
volatile uint32_t g_isrCallCount = 0;     // Number of ISR calls

// ---------------------------------------------------------------------------
// Bounce buffer desync detection and workaround
// ---------------------------------------------------------------------------
// PROBLEM: The ESP-IDF RGB LCD driver uses two bounce buffers (A and B) in a
// ping-pong arrangement. Normally the callback fills A while DMA sends B, then
// fills B while DMA sends A. Under certain conditions (e.g. flash/LittleFS
// writes introducing delays), the driver's phase can shift so the callback is
// asked to fill the SAME buffer the DMA is currently sending. This manifests
// as the display being vertically offset by the bounce buffer height (20 lines)
// with streaking artifacts where the CPU write races the DMA read.
//
// DETECTION: During begin(), we extract the GDMA channel handle from the
// panel's internal struct and cache a pointer to the GDMA TX out.dscr hardware
// register. In the ISR callback, we read this register to get the address of
// the DMA descriptor currently being processed, then dereference it to get the
// buffer address the DMA is actively reading from. If that address falls within
// the bounce buffer we've been asked to fill, we have a confirmed phase desync.
//
// WORKAROUND: When desync is detected, we redirect all writes (framebuffer
// copy, sprite overlay) to the OTHER bounce buffer, which is idle. This avoids
// corrupting the in-flight DMA transfer and places the data where the DMA will
// read it on its next pass.
//
// ESP-IDF DEPENDENCY: This workaround depends on:
//   1. The layout of esp_rgb_panel_t (esp_lcd_panel_rgb.c) to extract dma_chan.
//      See esp_rgb_panel_partial_t below -- must match ESP-IDF's struct layout.
//   2. The GDMA out.dscr register containing the current descriptor address.
//   3. The DMA descriptor layout: word0=flags, word1=buffer ptr, word2=next.
// If a future ESP-IDF version fixes the desync bug or changes any of these
// internals, this workaround can be removed by:
//   - Deleting this block of globals and the esp_rgb_panel_partial_t struct
//   - Deleting the desync detection + redirect block in bounceBufferFillCallback
//   - Deleting the GDMA setup block in begin()
//   - Removing the #include "soc/gdma_struct.h" and #include "hal/gdma_ll.h"
//   - Removing the g_bbDesyncCount / g_bbDiag_* externs from the header
// ---------------------------------------------------------------------------
volatile uint32_t g_bbDesyncCount = 0;    // Number of desync events detected
volatile uint32_t g_bbDroppedCount = 0;   // Fill requests dropped (queue full -> task fell behind)

// These three are used only by the desync workaround (LCD7_BOUNCE_DESYNC_WORKAROUND);
// marked unused so they don't warn when it is compiled out (the default).
static volatile uint32_t* s_gdma_out_dscr_reg __attribute__((unused)) = nullptr;  // GDMA TX out.dscr register
static void* s_bb_addrs[2] __attribute__((unused)) = {nullptr, nullptr};           // Learned bounce buffer addresses
static size_t s_bb_size __attribute__((unused)) = 0;                               // Bounce buffer size in bytes

// Diagnostic: last values seen by ISR (readable from main loop for debugging)
volatile uint32_t g_bbDiag_dscr = 0;        // Last descriptor address read from GDMA
volatile uint32_t g_bbDiag_bufAddr = 0;     // Buffer address extracted from that descriptor
volatile uint32_t g_bbDiag_bounceBuf = 0;   // bounce_buf pointer we were asked to fill

// ---------------------------------------------------------------------------
// Flash write overlay message
// ---------------------------------------------------------------------------
// When s_flashMsgBitmap is non-null, the ISR displays a black screen with a
// centered message, reading only from SRAM (no PSRAM or flash access).
// The bitmap is 1bpp (1 bit per pixel, MSB first, rows padded to byte boundary),
// pre-rendered by beginFlashMessage() and freed by endFlashMessage().
// The ISR expands 1bpp to RGB565 (0=black, 1=white) row by row.
// Double-buffered: main code writes to pending, ISR copies to active at frame start.
// This ensures the transition happens at a frame boundary (no tearing).
static volatile uint8_t* s_flashMsgActive = nullptr;   // active bitmap (ISR reads)
static volatile uint8_t* s_flashMsgPending = nullptr;  // pending bitmap (main code writes)
static int s_flashMsgW = 0;          // width of message in pixels
static int s_flashMsgH = 0;          // height of message in pixels
static int s_flashMsgStride = 0;     // bytes per row (ceil(width/8))
static int s_flashMsgX = 0;          // x offset to center on screen
static int s_flashMsgY = 0;          // y offset to center on screen
static bool s_flashMsgTile = false;  // if true, tile message every bounce buffer band
// Sentinel value used when beginFlashMessage(-1) blanks screen without text.
// Must be non-null so the ISR enters the flash overlay path, but not heap-allocated.
static uint8_t s_blankSentinel = 0;

// Partial mirror of esp_rgb_panel_t from esp_lcd_panel_rgb.c (ESP-IDF v5.5)
// Only fields up through dma_chan are needed so we can extract the GDMA channel
// handle at init time. We use byte arrays and void* for types whose definitions
// are not available in public headers (esp_lcd_panel_t, intr_handle_t, etc.),
// keeping the sizes identical to the real types (all pointer-sized on ESP32-S3).
// WARNING: This struct MUST be kept in sync with the real esp_rgb_panel_t.
// If the ESP-IDF struct layout changes, the dma_chan offset will be wrong and
// the gdma_get_group_channel_id() call in begin() will fail (logged to Serial).
struct esp_rgb_panel_partial_t {
    uint8_t base[10 * sizeof(void*)]; // esp_lcd_panel_t: 9 fn ptrs + user_data
    int panel_id;
    void* hal_dev;                  // lcd_hal_context_t contains one pointer
    size_t data_width;
    size_t fb_bits_per_pixel;
    size_t num_fbs;
    size_t output_bits_per_pixel;
    size_t dma_burst_size;
    int disp_gpio_num;
    void* intr;                     // intr_handle_t (opaque pointer)
    void* pm_lock;                  // esp_pm_lock_handle_t (opaque pointer)
    size_t num_dma_nodes;
    gdma_channel_handle_t dma_chan;  // <-- needed for desync detection
    void* dma_fb_links[3];          // gdma_link_list_handle_t dma_fb_links[RGB_LCD_PANEL_MAX_FB_NUM]
    void* dma_bb_link;              // gdma_link_list_handle_t dma_bb_link
    void* dma_restart_link;         // gdma_link_list_handle_t dma_restart_link (ESP32-S3 only)
    uint8_t* fbs[3];               // uint8_t *fbs[RGB_LCD_PANEL_MAX_FB_NUM]
    uint8_t* bounce_buffer[2];     // <-- needed to prefill "MUST ENABLE PSRAM" message
    size_t fb_size;
    size_t bb_size;
};

// ---------------------------------------------------------------------------
// "MUST ENABLE PSRAM" error message for when PSRAM is unavailable
// ---------------------------------------------------------------------------
// Minimal 5x7 font covering only the characters needed for the message.
// Each character is 7 bytes (one per row), with pixel data in bits 7..3 (MSB=left).
// Rendered at 2x scale (10x14 per char) centered on the 800px-wide screen.
static const uint8_t s_charA[] = {0x70,0x88,0x88,0xF8,0x88,0x88,0x88};
static const uint8_t s_charB[] = {0xF0,0x88,0x88,0xF0,0x88,0x88,0xF0};
static const uint8_t s_charE[] = {0xF8,0x80,0x80,0xF0,0x80,0x80,0xF8};
static const uint8_t s_charF[] = {0xF8,0x80,0x80,0xF0,0x80,0x80,0x80};
static const uint8_t s_charH[] = {0x88,0x88,0x88,0xF8,0x88,0x88,0x88};
static const uint8_t s_charI[] = {0x70,0x20,0x20,0x20,0x20,0x20,0x70};
static const uint8_t s_charL[] = {0x80,0x80,0x80,0x80,0x80,0x80,0xF8};
static const uint8_t s_charM[] = {0x88,0xD8,0xA8,0xA8,0x88,0x88,0x88};
static const uint8_t s_charN[] = {0x88,0xC8,0xA8,0x98,0x88,0x88,0x88};
static const uint8_t s_charO[] = {0x70,0x88,0x88,0x88,0x88,0x88,0x70};
static const uint8_t s_charP[] = {0xF0,0x88,0x88,0xF0,0x80,0x80,0x80};
static const uint8_t s_charR[] = {0xF0,0x88,0x88,0xF0,0xA0,0x90,0x88};
static const uint8_t s_charS[] = {0x70,0x88,0x80,0x70,0x08,0x88,0x70};
static const uint8_t s_charT[] = {0xF8,0x20,0x20,0x20,0x20,0x20,0x20};
static const uint8_t s_charU[] = {0x88,0x88,0x88,0x88,0x88,0x88,0x70};
static const uint8_t s_charW[] = {0x88,0x88,0x88,0xA8,0xA8,0xD8,0x88};
static const uint8_t s_charY[] = {0x88,0x88,0x50,0x20,0x20,0x20,0x20};
static const uint8_t s_charSP[]= {0x00,0x00,0x00,0x00,0x00,0x00,0x00};

// "MUST ENABLE PSRAM" = 17 characters (including spaces)
static const uint8_t* const s_noPsramMsg[] = {
    s_charM,s_charU,s_charS,s_charT,s_charSP,
    s_charE,s_charN,s_charA,s_charB,s_charL,s_charE,s_charSP,
    s_charP,s_charS,s_charR,s_charA,s_charM
};
static const int s_noPsramMsgLen = sizeof(s_noPsramMsg) / sizeof(s_noPsramMsg[0]);
// At 2x scale: each char is 10px wide + 2px gap = 12px per char.
// 17 chars * 12 - 2 (no trailing gap) = 202px wide, 14px tall.
// Centered on 800px screen: x_offset = (800 - 202) / 2 = 299
// The two bounce buffers span 40 lines, so the message tiles every 40 lines.
static const int s_noPsramScale = 2;
static const int s_noPsramCharW = 5 * s_noPsramScale;       // 10px
static const int s_noPsramCharH = 7 * s_noPsramScale;       // 14px
static const int s_noPsramGap = s_noPsramScale;             // 2px gap between chars
static const int s_noPsramTotalW = s_noPsramMsgLen * (s_noPsramCharW + s_noPsramGap) - s_noPsramGap;
static const int s_noPsramXOffset = (800 - s_noPsramTotalW) / 2;
// Vertical: center the 14px-tall text in a 40px band, repeating every 40 lines
static const int s_noPsramBandH = 40;
static const int s_noPsramYOffset = (s_noPsramBandH - s_noPsramCharH) / 2;

// "FLASH MEMORY WRITE" = 18 characters (including spaces)
// Used by beginFlashMessage() / endFlashMessage() to show an overlay during flash ops.
static const uint8_t* const s_flashWriteMsg[] = {
    s_charF,s_charL,s_charA,s_charS,s_charH,s_charSP,
    s_charM,s_charE,s_charM,s_charO,s_charR,s_charY,s_charSP,
    s_charW,s_charR,s_charI,s_charT,s_charE
};
static const int s_flashWriteMsgLen = sizeof(s_flashWriteMsg) / sizeof(s_flashWriteMsg[0]);

// Pre-render a font message into a 1bpp bitmap allocated in SRAM.
// Returns the buffer (caller must free), and fills in width, height, stride.
// 1bpp format: MSB first, rows padded to byte boundary.
// scale: pixel scale factor (2 = 10x14 per character).
// NOT IRAM_ATTR -- runs from flash at call time, not from ISR.
static uint8_t* renderMessageTo1bpp(const uint8_t* const* msg, int msgLen,
                                     int scale, int* outW, int* outH, int* outStride) {
    int charW = 5 * scale;
    int charH = 7 * scale;
    int gap = scale;
    int totalW = msgLen * (charW + gap) - gap;
    int totalH = charH;
    int stride = (totalW + 7) / 8;  // bytes per row, rounded up
    *outW = totalW;
    *outH = totalH;
    *outStride = stride;
    uint8_t* buf = (uint8_t*)heap_caps_malloc(stride * totalH,
                                               MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (!buf) return nullptr;
    memset(buf, 0, stride * totalH);
    for (int y = 0; y < totalH; y++) {
        int src_row = y / scale;
        for (int x = 0; x < totalW; x++) {
            int char_cell = x / (charW + gap);
            int in_cell_x = x % (charW + gap);
            if (char_cell < msgLen && in_cell_x < charW) {
                int src_col = in_cell_x / scale;
                uint8_t row_bits = msg[char_cell][src_row];
                if (row_bits & (0x80 >> src_col)) {
                    buf[y * stride + x / 8] |= (0x80 >> (x % 8));
                }
            }
        }
    }
    return buf;
}

// Render "MUST ENABLE PSRAM" message into a bounce buffer region.
// Called once per bounce buffer during init (not from ISR) to prefill the
// buffers with the error message. The DMA then continuously displays them.
// NOT IRAM_ATTR -- this runs from flash at init time only, saving IRAM.
static void renderNoPsramMessage(uint16_t* dst, int pos_px, int pixel_count, int screen_width) {
    for (int i = 0; i < pixel_count; i++) {
        int scr_x = (pos_px + i) % screen_width;
        int scr_y = (pos_px + i) / screen_width;
        int band_y = scr_y % s_noPsramBandH;
        int font_y = band_y - s_noPsramYOffset;
        uint16_t color = 0x0000; // black background
        if (font_y >= 0 && font_y < s_noPsramCharH) {
            int src_row = font_y / s_noPsramScale;
            int msg_x = scr_x - s_noPsramXOffset;
            if (msg_x >= 0 && msg_x < s_noPsramTotalW) {
                int char_cell = msg_x / (s_noPsramCharW + s_noPsramGap);
                int in_cell_x = msg_x % (s_noPsramCharW + s_noPsramGap);
                if (char_cell < s_noPsramMsgLen && in_cell_x < s_noPsramCharW) {
                    int src_col = in_cell_x / s_noPsramScale;
                    uint8_t row_bits = s_noPsramMsg[char_cell][src_row];
                    if (row_bits & (0x80 >> src_col)) {
                        color = 0xFFFF; // white text
                    }
                }
            }
        }
        dst[i] = color;
    }
}

// ---------------------------------------------------------------------------
// ISR -> fill-task hand-off
// ---------------------------------------------------------------------------
// The GDMA "bounce buffer empty" callback runs in ISR context.  To keep that
// ISR short (so it does not block other interrupts on its core), it does only
// the cheap, time-critical work -- frame-start latching, desync detection and
// the flash-overlay fill -- and hands the heavy PSRAM->bounce-buffer copy to a
// dedicated high-priority task via this request queue.  Each request snapshots
// the per-frame state (which framebuffer, which scroll offset) so the copy is
// consistent even if the task runs a little behind.
struct BounceFillRequest {
    void*     bounce_buf;     // target bounce buffer (already desync-redirected)
    int       pos_px;         // linear screen pixel position this chunk starts at
    int       len_bytes;      // bytes to fill
    uint16_t* framebuffer;    // active framebuffer to copy from (non-null)
    uint32_t  scroll_offset;  // scroll offset latched for this frame
    uint32_t  gen;            // chunk generation (cooperative mode; else unused)
};

// Bounce-buffer pointer-validation / desync redirect workaround.
//   0 (default) = trust the bounce_buf pointer the ESP-IDF RGB driver hands us.
//   1           = detect the driver's ping-pong phase inversion and redirect the
//                 fill to the idle buffer (see extras/DESYNC_WORKAROUND.md for the
//                 full technique and history -- it took real effort to get right).
// Disabled by default because the GDMA-channel extraction it needs reads a
// private esp_rgb_panel_t struct mirror whose layout is ESP-IDF-version-specific;
// on the core this was last built against (IDF 5.5.4) that read returned a null
// dma_chan.  The artifact it guards against is obvious on-screen (streaking that
// races the scan, typically after flash writes), so flip this back on -- and fix
// the struct offset per extras/DESYNC_WORKAROUND.md -- if it ever returns.
#ifndef LCD7_BOUNCE_DESYNC_WORKAROUND
#define LCD7_BOUNCE_DESYNC_WORKAROUND 0
#endif

// Where the PSRAM->bounce-buffer copy runs.
//   1 (default) = deferred to a dedicated high-priority task, so the GDMA ISR
//                 stays short (it only hands off the request) and doesn't block
//                 other interrupts on its core.  This is the normal mode.
//   0           = inline in the GDMA ISR, like the 2.8C driver -- runs the copy
//                 synchronously at the bounce-empty moment.  Also used as the
//                 automatic fallback if the task/queue can't be created.
// (Watch g_bbDroppedCount in task mode: a non-zero, climbing value means the
// fill task isn't keeping up with the refresh.)
#ifndef LCD7_BOUNCE_FILL_IN_TASK
#define LCD7_BOUNCE_FILL_IN_TASK 1
#endif

// NOTE: a GDMA (esp_async_memcpy) offload of this copy was tried and removed.  It
// could not meet the per-bounce-buffer deadline: the bounce mechanism is strictly
// serial (two buffers, one always being scanned out, so only the just-freed one
// can be filled -- no room to pipeline), and GDMA reaches PSRAM through the same
// cache as the CPU, so it is no faster than the CPU's prefetched/burst cached read
// while adding per-chunk write-back (cache coherency), submit, and completion-wake
// overhead.  Measured ~9% of chunks dropped vs 0% for the CPU copy.  The CPU fill
// below is the right tool on the ESP32-S3.  (A separate ESP32-P4 driver, which has
// an independent AXI DMA + 2D copy engine, does this differently and performs well.)

// The bounce-fill task runs at a very high priority, pinned to the core that
// owns the RGB panel, so it can meet the per-bounce-buffer deadline (it must
// finish before the DMA wraps back to the buffer it is filling).  Lower it if it
// starves other work on that core.
#define LCD7_BOUNCE_TASK_PRIORITY (configMAX_PRIORITIES - 1)
// Depth of the ISR->task hand-off queue.  There are only two bounce buffers, so
// a few slots absorb scheduling jitter; an overflow (counted in g_bbDroppedCount)
// means the task could not keep up with the refresh.
#define LCD7_BOUNCE_QUEUE_LEN 8

// Which core runs the bounce-fill task.
//   0 (default) = the same core as begin() (and the GDMA ISR), so the ISR->task
//                 hand-off is a same-core wakeup with the least latency.  That
//                 core (typically the Arduino APP CPU, core 1) is also running
//                 LVGL / loop(), so the copy shares it with the UI work -- which
//                 is fine: the CPU copy meets the ~1 ms deadline with margin.
//   1           = the OTHER core, moving the per-frame copy off the UI core.
//                 Costs a cross-core wakeup (a few microseconds, well within the
//                 deadline) but lets LVGL keep the begin() core to itself.
//
//                 *** WiFi / Bluetooth WARNING ***  The "other" core is normally
//                 the PRO CPU (core 0), where the WiFi driver and the Bluetooth
//                 controller live.  This fill task is configMAX_PRIORITIES-1
//                 (above the WiFi task) and uses ~45% of a core (~936 wakeups/s),
//                 so pinning it to core 0 will degrade WiFi throughput/latency and
//                 can break Bluetooth's hard real-time radio timing (glitches /
//                 drops).  Only enable this if the radio is unused -- or lower
//                 LCD7_BOUNCE_TASK_PRIORITY below the radio tasks (accepting some
//                 drop risk) if you truly need both.
#ifndef LCD7_BOUNCE_FILL_OTHER_CORE
#define LCD7_BOUNCE_FILL_OTHER_CORE 0
#endif

// ---------------------------------------------------------------------------
// Cooperative dual-core bounce fill (EXPERIMENTAL, opt-in; default OFF).
//
// Goal: use the PRO/WiFi core's spare time for the per-frame copy WITHOUT ever
// compromising WiFi/BT or missing the bounce deadline.
//   - A LOW-priority worker on the other (PRO) core does the copy opportunistically.
//     Low priority => WiFi/BT always preempt it => the radio is protected.
//   - A free-running checkpoint timer fires partway through each chunk's window.
//     If the worker hasn't finished (it got preempted by the radio), the timer
//     wakes a HIGH-priority backstop task on the app core to finish the remainder.
//   - Both advance ONE shared atomic cursor over the chunk.  Copying the same
//     stable source to the same destination is idempotent, so overlapping work
//     during a hand-over is harmless (no torn pixels), and a compare-and-swap on
//     the cursor is exactly "did the other core move it?" -- the loser just bows out.
//
// Covers the common case (no scroll, stride == width -- what the examples use);
// other configs fall back to a whole-chunk fill on the worker.  Sprites are drawn
// once by whichever core finishes the base copy.
//
// *** UNTESTED in this environment (no hardware/toolchain here) -- bring up on
//     device and watch g_bbDroppedCount under a WiFi load. ***
//
// Flash-write note: the checkpoint timer's ISR touches only internal SRAM (it
// reads the cursor and notifies a task -- no PSRAM), so it is safe provided its
// ISR runs from IRAM (enable CONFIG_GPTIMER_ISR_IRAM_SAFE if the app writes flash
// at runtime; the bundled demos do not).  The worker/backstop are tasks, so they
// never run during a flash write, like the default fill task.
#ifndef LCD7_BOUNCE_FILL_COOP
#define LCD7_BOUNCE_FILL_COOP 0
#endif

#ifndef LCD7_COOP_BLOCK_PX
#define LCD7_COOP_BLOCK_PX 512      // pixels copied per claimed block (CAS granularity)
#endif
#ifndef LCD7_COOP_CHECKPOINT_US
#define LCD7_COOP_CHECKPOINT_US 500 // how long to let the worker try before backstop helps
#endif
#ifndef LCD7_COOP_TICK_US
#define LCD7_COOP_TICK_US 150       // checkpoint-timer period
#endif
#ifndef LCD7_COOP_WORKER_PRIORITY
#define LCD7_COOP_WORKER_PRIORITY 2 // low (above idle, below the WiFi/BT tasks)
#endif

#if LCD7_BOUNCE_FILL_COOP
#include <atomic>
#include "driver/gptimer.h"

// Parameters of the chunk currently being filled (set by the bounce ISR).  Plain
// struct in internal RAM; the generation counter publishes it (see _coopDrive).
struct CoopChunk {
    void*     bounce_buf;
    uint16_t* framebuffer;
    int       pos_px;
    int       pixel_count;
    int       rev_base;      // 180: output px j <- framebuffer[rev_base - j]
    bool      rotate180;
    bool      coop_ok;       // scroll==0 && stride==width (else worker does whole fill)
};
static CoopChunk g_coop;
static std::atomic<uint32_t> g_coopCursor{0};   // next output pixel to copy
static volatile uint32_t g_coopGen = 0;         // bumped per chunk; old workers bail
static volatile uint32_t g_coopStartCcount = 0; // CPU cycle count at chunk start
static volatile uint32_t g_coopThreshCycles = 0;// LCD7_COOP_CHECKPOINT_US in cycles
static volatile bool      g_coopActive = false; // a coop chunk is in flight
static gptimer_handle_t   g_coopTimer = nullptr;
static TaskHandle_t       g_coopBackstop = nullptr;

// Copy output pixels [j0, j0+n) of chunk `c`.  Runs in task context only (worker
// or backstop), so PSRAM access and memcpy are fine.  Reads PSRAM forward in both
// orientations (the 180 case writes the SRAM side in reverse).
static void coopCopyBlock(const CoopChunk& c, int j0, int n) {
    uint16_t* dst = (uint16_t*)c.bounce_buf;
    if (!c.rotate180) {
        memcpy(dst + j0, c.framebuffer + c.pos_px + j0, (size_t)n * sizeof(uint16_t));
    } else {
        // output px (j0 .. j0+n-1) <- framebuffer[rev_base - j]; read the source
        // run forward and write the destination reversed.
        const uint16_t* sf = c.framebuffer + (c.rev_base - (j0 + n - 1));
        for (int k = 0; k < n; k++) dst[j0 + (n - 1 - k)] = sf[k];
    }
}

// Checkpoint timer ISR: SRAM-only.  If the worker is past the grace period but
// the chunk isn't done, wake the app-core backstop.  Touches no PSRAM.
static bool IRAM_ATTR coopTimerCb(gptimer_handle_t, const gptimer_alarm_event_data_t*, void*) {
    if (!g_coopActive) return false;
    if (g_coopCursor.load(std::memory_order_relaxed) >= (uint32_t)g_coop.pixel_count) return false;
    uint32_t now = esp_cpu_get_cycle_count();
    if ((now - g_coopStartCcount) < g_coopThreshCycles) return false;  // give the worker more time
    BaseType_t hpw = pdFALSE;
    if (g_coopBackstop) vTaskNotifyGiveFromISR(g_coopBackstop, &hpw);
    return hpw == pdTRUE;
}
#endif // LCD7_BOUNCE_FILL_COOP

// Bounce buffer callback - runs in ISR context (GDMA "bounce empty" interrupt).
// Does the cheap, time-critical work inline and defers the PSRAM copy to the
// high-priority fill task.  See the block comment above.
bool IRAM_ATTR bounceBufferFillCallback(esp_lcd_panel_handle_t panel, void *bounce_buf,
                                                int pos_px, int len_bytes, void *user_ctx) {
    uint32_t entry_time = esp_cpu_get_cycle_count();

    // Accumulate time spent outside ISR (since last exit)
    if (g_isrCallCount > 0) {
        g_isrCyclesOut += (entry_time - g_isrLastExit);
    }
    g_isrCallCount=g_isrCallCount+1;

    ESP32S3_Touch_LCD_7* display = (ESP32S3_Touch_LCD_7*)user_ctx;

#if LCD7_BOUNCE_DESYNC_WORKAROUND
    // --- Begin bounce buffer desync detection and workaround ---
    // (See extras/DESYNC_WORKAROUND.md for the full explanation and history.)
    //
    // This MUST stay in the ISR, not the fill task: it reads the GDMA descriptor
    // register at notification time, which is exactly when the driver's
    // ping-pong phase inversion is detectable.  Deferring it to the task would
    // also mistake mere task lateness (DMA reading our buffer because we are slow)
    // for a phase inversion.  It is cheap -- one MMIO read and a compare -- and
    // touches no PSRAM/flash, so it is safe in the ISR.

    // Learn both bounce buffer addresses from the first two distinct callback invocations
    if (s_bb_addrs[0] == nullptr) {
        s_bb_addrs[0] = bounce_buf;
    } else if (s_bb_addrs[1] == nullptr && bounce_buf != s_bb_addrs[0]) {
        s_bb_addrs[1] = bounce_buf;
    }

    // Read the GDMA out.dscr register to find the descriptor currently being DMA'd,
    // then extract the buffer address from that descriptor (word 1 of the DMA descriptor).
    // If the DMA's active buffer is the same one we've been asked to fill, the driver's
    // ping-pong phase is inverted -- redirect our writes to the other (idle) buffer.
    if (s_gdma_out_dscr_reg != nullptr && s_bb_size > 0) {
        uint32_t dscr_addr = *s_gdma_out_dscr_reg;
        g_bbDiag_dscr = dscr_addr;
        g_bbDiag_bounceBuf = (uint32_t)bounce_buf;
        if (dscr_addr != 0) {
            uint32_t* desc = (uint32_t*)dscr_addr;
            uint32_t dma_buf_addr = desc[1];  // DMA descriptor word 1 = buffer pointer
            g_bbDiag_bufAddr = dma_buf_addr;
            uint32_t bb_start = (uint32_t)bounce_buf;
            uint32_t bb_end = bb_start + s_bb_size;
            if (dma_buf_addr >= bb_start && dma_buf_addr < bb_end) {
                g_bbDesyncCount = g_bbDesyncCount + 1;
                // Redirect writes to the other bounce buffer since this one is in-flight
                if (s_bb_addrs[0] != nullptr && s_bb_addrs[1] != nullptr) {
                    bounce_buf = (bounce_buf == s_bb_addrs[0]) ? s_bb_addrs[1] : s_bb_addrs[0];
                }
            }
        }
    }
    // --- End bounce buffer desync detection and workaround ---
#endif // LCD7_BOUNCE_DESYNC_WORKAROUND

    // At frame start (pos_px == 0), copy pending values to active
    // This provides tear-free scrolling/swapping by only changing at frame boundaries
    if (pos_px == 0) {
        display->_scroll_offset = display->_pending_scroll_offset;
        display->_active_fb = display->_pending_fb;
        s_flashMsgActive = s_flashMsgPending;

        // Copy pending sprites to active sprites for this frame
        for (int i = 0; i < SPRITE_MAX_COUNT; i++) {
            display->_active_sprites[i] = display->_pending_sprites[i];
        }
    }

    // Select the active framebuffer (1 = primary, 2 = secondary)
    uint16_t* framebuffer = (display->_active_fb == 2 && display->_framebuffer2 != nullptr)
                            ? display->_framebuffer2
                            : display->_framebuffer;

    const int screen_width = chipguy_S3_RGB_display::_width;

    // If no framebuffer (PSRAM not enabled), the bounce buffers were prefilled
    // with the "MUST ENABLE PSRAM" message during begin(). Nothing to do here --
    // the DMA just keeps cycling the prefilled buffers.
    if (framebuffer == nullptr) {
        return false;
    }

    // Flash write overlay: if active, fill bounce buffer with black background
    // and expand the 1bpp SRAM bitmap for message rows that overlap this chunk.
    // Row-by-row copy from SRAM only -- no PSRAM or flash access.
    //
    // This path MUST stay in the ISR (not the fill task): it runs during flash
    // writes, when the cache is disabled and no FreeRTOS task can be scheduled.
    // It only touches SRAM, so the IRAM-safe ISR can execute it throughout the
    // stall -- which is the whole reason the overlay exists.
    const volatile uint8_t* flashBmp = s_flashMsgActive;
    if (flashBmp != nullptr) {
        uint16_t* dst = (uint16_t*)bounce_buf;
        int pixel_count = len_bytes / sizeof(uint16_t);
        int start_y = pos_px / screen_width;
        int num_lines = pixel_count / screen_width;

        // Fill entire bounce buffer black (32-bit fill for speed)
        uint32_t* dst32 = (uint32_t*)dst;
        for (int i = 0; i < pixel_count / 2; i++) {
            dst32[i] = 0;
        }

        // Expand 1bpp message rows that fall within this chunk.
        // If tiling, the message repeats every num_lines lines (bounce buffer height),
        // so every chunk shows it at the same offset -- even if the DMA replays a
        // single chunk during a flash stall, the message is always visible.
        int msgX = s_flashMsgX;
        int msgY = s_flashMsgY;
        int msgW = s_flashMsgW;
        int msgH = s_flashMsgH;
        int stride = s_flashMsgStride;
        bool tile = s_flashMsgTile;
        // Under 180-degree rotation the overlay is flipped so the text reads
        // upright on the inverted panel: each physical line maps to logical line
        // (H-1-scr_y) and each message column to physical x (W-1-(msgX+x)).
        bool rot = display->_rotate180;
        for (int line = 0; line < num_lines; line++) {
            int scr_y = start_y + line;                                  // physical line
            int ly = rot ? (chipguy_S3_RGB_display::_height - 1 - scr_y)    // logical line
                         : scr_y;
            int rel_y;
            if (tile) {
                // Tile: use position within the bounce buffer band, offset by msgY
                // which is the offset within a band (set by beginFlashMessage)
                int band_y = ly % num_lines;
                rel_y = band_y - msgY;
            } else {
                rel_y = ly - msgY;
            }
            if (rel_y < 0 || rel_y >= msgH) continue;
            const volatile uint8_t* src_row = flashBmp + rel_y * stride;
            for (int x = 0; x < msgW; x++) {
                if (src_row[x / 8] & (0x80 >> (x % 8))) {
                    int phys_x = rot ? (screen_width - 1 - (msgX + x)) : (msgX + x);
                    dst[line * screen_width + phys_x] = 0xFFFF;
                }
            }
        }
        return false;
    }

    // Normal operation: hand the heavy PSRAM->bounce-buffer copy (and sprite
    // overlay) to the high-priority fill task.  The desync redirect above has
    // already chosen the target buffer; snapshot the per-frame state so the copy
    // is consistent even if the task runs slightly behind.
    BaseType_t hpw = pdFALSE;
    if (display->_bounceQueue != nullptr) {
        BounceFillRequest req;
        req.bounce_buf    = bounce_buf;
        req.pos_px        = pos_px;
        req.len_bytes     = len_bytes;
        req.framebuffer   = framebuffer;
        req.scroll_offset = display->_scroll_offset;
        req.gen           = 0;
#if LCD7_BOUNCE_FILL_COOP
        // Publish this chunk for the cooperative worker + backstop.  Bump the
        // generation FIRST so any worker still on the previous chunk bails before
        // we overwrite the params; then set params and reset the shared cursor.
        {
            int pixel_count = len_bytes / sizeof(uint16_t);
            bool coop_ok = (display->_scroll_offset == 0) &&
                           (display->_fb_stride == chipguy_S3_RGB_display::_width);
            g_coopActive = false;                       // pause the checkpoint timer
            uint32_t g = g_coopGen + 1;
            g_coopGen = g;
            g_coop.bounce_buf  = bounce_buf;
            g_coop.framebuffer = framebuffer;
            g_coop.pos_px      = pos_px;
            g_coop.pixel_count = pixel_count;
            g_coop.rotate180   = display->_rotate180;
            g_coop.coop_ok     = coop_ok;
            g_coop.rev_base    = chipguy_S3_RGB_display::_width *
                                 chipguy_S3_RGB_display::_height - pos_px - 1;
            g_coopCursor.store(0, std::memory_order_release);
            g_coopStartCcount  = entry_time;
            g_coopActive       = coop_ok;               // timer only acts on coop chunks
            req.gen            = g;
        }
#endif
        if (xQueueSendFromISR(display->_bounceQueue, &req, &hpw) != pdTRUE) {
            // Queue full: the fill task could not keep up.  The buffer keeps its
            // previous contents this pass (a transient glitch); count it.
            g_bbDroppedCount = g_bbDroppedCount + 1;
        }
    } else {
        // Fallback (no fill task -- creation failed): do the copy inline, as the
        // original driver did.  Safe during normal operation (cache enabled).
        display->_fillBounceBuffer(bounce_buf, pos_px, len_bytes, framebuffer,
                                   display->_scroll_offset);
    }

    // Accumulate time spent inside ISR and record exit time
    uint32_t exit_time = esp_cpu_get_cycle_count();
    g_isrCyclesIn += (exit_time - entry_time);
    g_isrLastExit = exit_time;

    return hpw == pdTRUE;
}

// The high-priority task that drains the request queue and performs the actual
// PSRAM->bounce-buffer copy + sprite overlay.  Runs in task context (cache
// enabled), so it never executes during a flash write -- those frames are
// handled entirely by the ISR's flash-overlay path above.
void chipguy_S3_RGB_display::_bounceFillTask(void* arg) {
    ESP32S3_Touch_LCD_7* self = (ESP32S3_Touch_LCD_7*)arg;
    BounceFillRequest req;
    for (;;) {
        if (xQueueReceive(self->_bounceQueue, &req, portMAX_DELAY) == pdTRUE) {
            self->_fillBounceBuffer(req.bounce_buf, req.pos_px, req.len_bytes,
                                    req.framebuffer, req.scroll_offset);
        }
    }
}

// Copy one bounce-buffer-worth of pixels from the PSRAM framebuffer, then draw
// any sprites that overlap this chunk.  This is the heavy work; by default it
// runs inline in the GDMA ISR (LCD7_BOUNCE_FILL_IN_TASK==0), or in _bounceFillTask
// when that is enabled.  The caller has already done frame-start latching and the
// no-PSRAM / flash-overlay checks.  Marked IRAM_ATTR so the ISR-inline path runs
// from IRAM (deterministic timing, no i-cache miss) -- harmless for the task path.
void IRAM_ATTR chipguy_S3_RGB_display::_fillBounceBuffer(void* bounce_buf, int pos_px, int len_bytes,
                                            uint16_t* framebuffer, uint32_t scroll_offset) {
    const int screen_width = chipguy_S3_RGB_display::_width;
    const uint32_t fb_size = _fb_size;
    const int fb_stride = _fb_stride;

    int pixel_count = len_bytes / sizeof(uint16_t);

    if (!_rotate180) {
        // ---- Normal orientation: read PSRAM forward, write bounce forward ----
        uint16_t* dst = (uint16_t*)bounce_buf;

        // Screen position from linear pos_px
        int screen_y = pos_px / screen_width;
        int screen_x = pos_px % screen_width;

        // Linear framebuffer position: scroll_offset + screen_y * fb_stride + screen_x
        // Each screen row is contiguous (800 pixels), with fb_stride spacing between rows
        while (pixel_count > 0) {
            // Compute linear position in framebuffer
            uint32_t fb_pos = (scroll_offset + screen_y * fb_stride + screen_x) % fb_size;
            uint16_t* src = framebuffer + fb_pos;

            // How many pixels until end of screen row or buffer wrap?
            int pixels_to_row_end = screen_width - screen_x;
            int pixels_to_wrap = fb_size - fb_pos;
            int segment = pixels_to_row_end;
            if (pixels_to_wrap < segment) segment = pixels_to_wrap;
            if (pixel_count < segment) segment = pixel_count;

            // Copy segment using 32-bit operations
            int remaining = segment;

            // Align dst if needed
            if (((uintptr_t)dst & 2) && remaining > 0) {
                *dst++ = *src++;
                remaining--;
            }

            // Align src if needed
            if (((uintptr_t)src & 2) && remaining > 0) {
                *dst++ = *src++;
                remaining--;
            }

            // Now both are 32-bit aligned - fast copy
            uint32_t* src32 = (uint32_t*)src;
            uint32_t* dst32 = (uint32_t*)dst;
            while (remaining >= 8) {
                dst32[0] = src32[0];
                dst32[1] = src32[1];
                dst32[2] = src32[2];
                dst32[3] = src32[3];
                src32 += 4;
                dst32 += 4;
                remaining -= 8;
            }
            while (remaining >= 2) {
                *dst32++ = *src32++;
                remaining -= 2;
            }
            dst = (uint16_t*)dst32;
            if (remaining) {
                *dst++ = *(uint16_t*)src32;
            }

            pixel_count -= segment;
            screen_x += segment;
            if (screen_x >= screen_width) {
                screen_x = 0;
                screen_y++;
            }
        }
    } else {
        // ---- 180-degree rotation ----
        // Physical pixel p must show logical pixel (total-1-p).  Read PSRAM
        // FORWARD from the opposite end of the screen (cache-friendly sequential
        // access -- the whole point), and write the SRAM bounce buffer BACKWARD.
        // Only the cheap SRAM side runs in reverse; PSRAM stays sequential.
        const int total_screen_pixels = screen_width * chipguy_S3_RGB_display::_height;
        int rotated_pos = total_screen_pixels - pos_px - pixel_count;
        int screen_y = rotated_pos / screen_width;
        int screen_x = rotated_pos % screen_width;

        // dst starts at the end of the bounce buffer and decrements
        uint16_t* dst = (uint16_t*)bounce_buf + pixel_count - 1;

        while (pixel_count > 0) {
            uint32_t fb_pos = (scroll_offset + screen_y * fb_stride + screen_x) % fb_size;
            uint16_t* src = framebuffer + fb_pos;

            // How many pixels until end of screen row or buffer wrap?
            int pixels_to_row_end = screen_width - screen_x;
            int pixels_to_wrap = fb_size - fb_pos;
            int segment = pixels_to_row_end;
            if (pixels_to_wrap < segment) segment = pixels_to_wrap;
            if (pixel_count < segment) segment = pixel_count;

            // Read forward from PSRAM, write backward to SRAM.
            //
            // Fast path: read a 32-bit word (two pixels) forward and store it
            // reversed at the mirrored position.  On little-endian, a word is
            // (src[i+1]<<16 | src[i]); the mirrored destination word must be
            // (src[i]<<16 | src[i+1]) -- i.e. a 16-bit halfword rotate of the
            // word -- so we still read PSRAM forward in 32-bit bursts and halve
            // the load/store count.  Requires src word-aligned AND the
            // destination word base (dst-1) word-aligned (the mirror makes this a
            // dual constraint); otherwise fall back to the scalar loop below.
            int i = 0;
            if ((((uintptr_t)src & 3) == 0) && (((uintptr_t)(dst - 1) & 3) == 0)) {
                const uint32_t* s32 = (const uint32_t*)src;
                uint32_t* d32 = (uint32_t*)(dst - 1);  // base of first mirrored word
                int pairs = segment >> 1;
                for (int p = 0; p < pairs; p++) {
                    uint32_t x = s32[p];
                    d32[-p] = (x >> 16) | (x << 16);   // swap the two pixels
                }
                i = pairs << 1;
                dst -= i;                              // advance past the batched pixels
            }
            // Scalar remainder (and the whole segment when unaligned)
            for (; i < segment; i++) {
                *dst-- = src[i];
            }

            pixel_count -= segment;
            screen_x += segment;
            if (screen_x >= screen_width) {
                screen_x = 0;
                screen_y++;
            }
        }
    }

    // Draw any sprites that overlap this chunk (shared with the DMA fill path).
    _overlaySprites(bounce_buf, pos_px, len_bytes);
}

// Overlay active sprites onto a freshly-filled bounce buffer.  Split out of
// _fillBounceBuffer so the GDMA fill path can reuse it after the DMA copy.
void IRAM_ATTR chipguy_S3_RGB_display::_overlaySprites(void* bounce_buf, int pos_px, int len_bytes) {
    const int screen_width = chipguy_S3_RGB_display::_width;

    // Sprite overlay - draw sprites on top of framebuffer content
    // Sprites are in display coordinates (not affected by scrolling)
    const int chunk_start_px = pos_px;
    const int chunk_end_px = pos_px + (len_bytes / sizeof(uint16_t)) - 1;
    const int chunk_start_y = chunk_start_px / screen_width;
    const int chunk_end_y = chunk_end_px / screen_width;

    uint16_t* bounce_buf16 = (uint16_t*)bounce_buf;

    for (int spr = 0; spr < SPRITE_MAX_COUNT; spr++) {
        const Sprite& sprite = _active_sprites[spr];

        // Skip disabled sprites
        if (sprite.yx == SPRITE_DISABLED || sprite.data == nullptr) continue;

        // Extract sprite properties
        const int16_t spr_y = SPRITE_GET_Y(sprite.yx);
        const int16_t spr_x = SPRITE_GET_X(sprite.yx);
        const uint16_t spr_h = SPRITE_GET_H(sprite.hw);
        const uint16_t spr_w = SPRITE_GET_W(sprite.hw);
        const uint16_t trans = sprite.transparent;
        const uint16_t* spr_data = sprite.data;
        const bool detect_collision = (sprite.flags & SPRITE_FLAG_COLLISION_DETECT) != 0;

        // Collision detection state for this sprite in this chunk
        bool first_color_seen = false;
        uint16_t first_color = 0;
        bool collision_detected = false;

        // Calculate sprite's logical screen Y range
        const int16_t spr_y_end = spr_y + spr_h - 1;

        // Physical Y span of this sprite (flipped under 180-degree rotation),
        // used to reject sprites that don't fall in this (physical) chunk.
        int16_t phys_y_lo, phys_y_hi;
        if (_rotate180) {
            phys_y_lo = chipguy_S3_RGB_display::_height - 1 - spr_y_end;
            phys_y_hi = chipguy_S3_RGB_display::_height - 1 - spr_y;
        } else {
            phys_y_lo = spr_y;
            phys_y_hi = spr_y_end;
        }

        // Quick rejection: sprite doesn't overlap this chunk's (physical) Y range
        if (phys_y_lo > chunk_end_y || phys_y_hi < chunk_start_y) continue;

        // Calculate sprite's screen X range (for horizontal bounds)
        const int16_t spr_x_end = spr_x + spr_w - 1;

        // Quick rejection: sprite is entirely off-screen horizontally
        if (spr_x >= screen_width || spr_x_end < 0) continue;

        // Process each row of the sprite that falls within this chunk
        for (int16_t row = 0; row < spr_h; row++) {
            int16_t scr_y = spr_y + row;          // logical screen row

            // Skip rows off-screen (logical bounds)
            if (scr_y < 0 || scr_y >= chipguy_S3_RGB_display::_height) continue;

            // Physical row this logical row maps to (flipped under 180)
            int16_t phys_y = _rotate180 ? (chipguy_S3_RGB_display::_height - 1 - scr_y) : scr_y;

            // Skip rows outside this chunk's (physical) Y range
            if (phys_y < chunk_start_y || phys_y > chunk_end_y) continue;

            // Calculate visible X range for this sprite row
            int16_t vis_x_start = (spr_x < 0) ? 0 : spr_x;
            int16_t vis_x_end = (spr_x_end >= screen_width) ? (screen_width - 1) : spr_x_end;

            // Skip if no visible pixels
            if (vis_x_start > vis_x_end) continue;

            // For each pixel in the visible sprite row
            for (int16_t scr_x = vis_x_start; scr_x <= vis_x_end; scr_x++) {
                // Physical position of this logical pixel (flipped under 180)
                int16_t phys_x = _rotate180 ? (screen_width - 1 - scr_x) : scr_x;
                int linear_pos = phys_y * screen_width + phys_x;

                // Check if this pixel is in the current chunk
                if (linear_pos < chunk_start_px || linear_pos > chunk_end_px) continue;

                // Get sprite pixel (sprite data is addressed in logical space)
                int spr_px_x = scr_x - spr_x;  // X offset into sprite
                uint16_t spr_pixel = spr_data[row * spr_w + spr_px_x];

                // Skip transparent pixels
                if (spr_pixel == trans) continue;

                int buf_offset = linear_pos - chunk_start_px;

                // Collision detection: check if we're overwriting multiple colors
                if (detect_collision && !collision_detected) {
                    uint16_t existing_color = bounce_buf16[buf_offset];
                    if (!first_color_seen) {
                        first_color = existing_color;
                        first_color_seen = true;
                    } else if (existing_color != first_color) {
                        collision_detected = true;
                    }
                }

                // Write to bounce buffer
                bounce_buf16[buf_offset] = spr_pixel;
            }
        }

        // Set collision flag if detected (only sets, never clears)
        if (collision_detected) {
            _sprite_collisions[spr] = 1;
        }
    }
}

#if LCD7_BOUNCE_FILL_COOP
// Cooperatively drain the current chunk: claim a block, copy it, advance the
// shared cursor with a CAS.  A failed CAS means the other core moved the cursor
// (it's helping / took over) -- we just re-read and keep helping; the loop ends
// when the cursor reaches the end.  Whoever lands the final advance draws the
// sprites and clears the active flag.  myGen guards against the chunk rolling
// over while a stalled worker is still here (it abandons the stale chunk).
void chipguy_S3_RGB_display::_coopDrive(uint32_t myGen) {
    if (g_coopGen != myGen) return;
    CoopChunk c = g_coop;                 // snapshot the chunk params...
    if (g_coopGen != myGen) return;       // ...and confirm they're still ours
    const uint32_t end = (uint32_t)c.pixel_count;
    for (;;) {
        if (g_coopGen != myGen) return;   // chunk rolled over -> abandon
        uint32_t p = g_coopCursor.load(std::memory_order_acquire);
        if (p >= end) break;              // finished by us or the other core
        uint32_t n = end - p;
        if (n > (uint32_t)LCD7_COOP_BLOCK_PX) n = (uint32_t)LCD7_COOP_BLOCK_PX;
        coopCopyBlock(c, (int)p, (int)n);                 // idempotent on overlap
        uint32_t want = p + n;
        if (!g_coopCursor.compare_exchange_strong(
                p, want, std::memory_order_release, std::memory_order_relaxed)) {
            continue;                     // the other core advanced it -> re-read
        }
        if (want >= end) {                // we landed the final block -> finish up
            _overlaySprites(c.bounce_buf, c.pos_px, c.pixel_count * (int)sizeof(uint16_t));
            g_coopActive = false;         // stop the checkpoint timer acting on this chunk
            break;
        }
    }
}

// Core-0 (PRO/WiFi core), LOW priority: opportunistic worker.  Runs the chunk
// cooperatively when it qualifies, else does the whole fill itself.
void chipguy_S3_RGB_display::_coopWorkerTask(void* arg) {
    ESP32S3_Touch_LCD_7* self = (ESP32S3_Touch_LCD_7*)arg;
    BounceFillRequest req;
    for (;;) {
        if (xQueueReceive(self->_bounceQueue, &req, portMAX_DELAY) == pdTRUE) {
            bool coop_ok = (req.scroll_offset == 0) &&
                           (self->_fb_stride == chipguy_S3_RGB_display::_width);
            if (coop_ok) {
                self->_coopDrive(req.gen);
            } else {
                self->_fillBounceBuffer(req.bounce_buf, req.pos_px, req.len_bytes,
                                        req.framebuffer, req.scroll_offset);
            }
        }
    }
}

// Core-1 (app core), HIGH priority: backstop.  Woken by the checkpoint timer only
// when the worker has fallen behind; finishes the remainder of the live chunk.
void chipguy_S3_RGB_display::_coopBackstopTask(void* arg) {
    ESP32S3_Touch_LCD_7* self = (ESP32S3_Touch_LCD_7*)arg;
    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        self->_coopDrive(g_coopGen);      // help whatever chunk is current
    }
}
#endif // LCD7_BOUNCE_FILL_COOP

chipguy_S3_RGB_display::chipguy_S3_RGB_display(AMK_S3_Board& board,
                                         uint16_t fb_pad_x, uint16_t fb_pad_y)
    : _board(board)
    , _fb_pad_x(fb_pad_x)
    , _fb_pad_y(fb_pad_y)
    , _fb_stride(_width + fb_pad_x)
    , _fb_rows(_height + fb_pad_y)
    , _fb_size((_width + fb_pad_x) * (_height + fb_pad_y))
{
    // Framebuffer includes padding for pre-drawing scroll content
    _framebuffer_size = _fb_size * sizeof(uint16_t);

    // Initialize all sprite slots as disabled
    for (int i = 0; i < SPRITE_MAX_COUNT; i++) {
        _pending_sprites[i].data = nullptr;
        _pending_sprites[i].yx = SPRITE_DISABLED;
        _pending_sprites[i].hw = 0;
        _pending_sprites[i].transparent = 0;
        _pending_sprites[i].flags = 0;
        _active_sprites[i].data = nullptr;
        _active_sprites[i].yx = SPRITE_DISABLED;
        _active_sprites[i].hw = 0;
        _active_sprites[i].transparent = 0;
        _active_sprites[i].flags = 0;
        _sprite_collisions[i] = 0;
    }
}

bool chipguy_S3_RGB_display::allocateSecondFramebuffer() {
    // Already allocated
    if (_framebuffer2 != nullptr) {
        return false;
    }

    // Allocate second framebuffer with same size and alignment as primary
    _framebuffer2 = (uint16_t*)heap_caps_aligned_alloc(64, _framebuffer_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!_framebuffer2) {
        Serial.println("Failed to allocate second PSRAM framebuffer");
        return false;
    }

    // Clear to black
    memset(_framebuffer2, 0, _framebuffer_size);

    return true;
}

void chipguy_S3_RGB_display::setDrawBuffer(uint8_t index) {
    if (index == 1) {
        _draw_target = _framebuffer;
        _draw_fb_index = 1;
    } else if (index == 2 && _framebuffer2 != nullptr) {
        _draw_target = _framebuffer2;
        _draw_fb_index = 2;
    }
}

void chipguy_S3_RGB_display::setActiveFramebuffer(uint8_t index, bool wait) {
    // index: 0 = swap to other, 1 = primary, 2 = secondary
    if (index == 0) {
        // Swap: toggle between 1 and 2 (only if framebuffer2 exists)
        if (_framebuffer2 != nullptr) {
            _pending_fb = (_pending_fb == 1) ? 2 : 1;
        } else {
            // No second framebuffer, nothing to swap
            return;
        }
    } else if (index == 1) {
        _pending_fb = 1;
    } else if (index == 2 && _framebuffer2 != nullptr) {
        _pending_fb = 2;
    } else {
        // Invalid request - nothing to wait for
        return;
    }

    // If requested, block until the ISR has applied the pending value
    if (wait) {
        while (_active_fb != _pending_fb) {
            // Yield to other tasks while waiting for vsync
            vTaskDelay(1);
        }
    }
}

bool chipguy_S3_RGB_display::begin(uint32_t pclk_hz, uint16_t rotation) {
    // Latch the rotation for the lifetime of the driver.  Normalize index
    // aliases (1/2/3 -> 90/180/270) first; only 180 flips here.  90/270 are not
    // handled by the driver -- they belong at the LVGL->framebuffer level.
    rotation = cglcd7_normalize_rotation(rotation);
    _rotate180 = (rotation == 180);

    // Bring up the board's shared I2C bus + CH422G expander first: on this board
    // the ST7262's reset and the LCD backlight live on the expander.
    // (Idempotent -- touch.begin() may also call it.)
    _board.begin();

    // Hardware-reset the panel via the expander (IO3).  There is no SPI command
    // controller on this RGB panel, so this is the entire panel bring-up.
    _board.lcdReset();

    // Allocate our own framebuffer in PSRAM (required for no_fb mode with callback)
    // Use 64-byte alignment for optimal cache line access
    // If PSRAM is not enabled or allocation fails, _framebuffer stays null and
    // the bounce buffer callback will display a "MUST ENABLE PSRAM" message instead.
    _framebuffer = (uint16_t*)heap_caps_aligned_alloc(64, _framebuffer_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!_framebuffer) {
        Serial.println("Failed to allocate PSRAM framebuffer - display will show error message");
    }

    // Set default draw target to primary framebuffer
    _draw_target = _framebuffer;
    _draw_fb_index = 1;

    // Configure RGB panel for the ST7262 800x480 panel (Waveshare's timing).
    esp_lcd_rgb_panel_config_t panel_config = {
        .clk_src = LCD_CLK_SRC_DEFAULT,
        .timings = {
            .pclk_hz = pclk_hz,
            .h_res = _width,
            .v_res = _height,
            .hsync_pulse_width = 4,
            .hsync_back_porch = 8,
            .hsync_front_porch = 8,
            .vsync_pulse_width = 4,
            .vsync_back_porch = 8,
            .vsync_front_porch = 8,
            .flags = {
                .hsync_idle_low = 0,
                .vsync_idle_low = 0,
                .de_idle_high = 0,
                .pclk_active_neg = 1,  // ST7262: data latched on falling edge
                .pclk_idle_high = 0,
            },
        },
        .data_width = 16,
        .bits_per_pixel = 16,
        .num_fbs = 0,
        .bounce_buffer_size_px = _bounce_buffer_size_px,  // 20 scanlines in SRAM
        .sram_trans_align = 8,
        .psram_trans_align = 64,
        .hsync_gpio_num = PIN_LCD_HSYNC,
        .vsync_gpio_num = PIN_LCD_VSYNC,
        .de_gpio_num = PIN_LCD_DE,
        .pclk_gpio_num = PIN_LCD_PCLK,
        .disp_gpio_num = -1,
        .data_gpio_nums = {0},
        .flags = {
            .disp_active_low = 0,
            .refresh_on_demand = false,
            .fb_in_psram = false,
            .double_fb = false,
            .no_fb = true,
            .bb_invalidate_cache = false,
        },
    };

    // Set data GPIO pins individually (RGB565: B=bits 0-4, G=bits 5-10, R=bits 11-15)
    panel_config.data_gpio_nums[0] = PIN_LCD_B0;
    panel_config.data_gpio_nums[1] = PIN_LCD_B1;
    panel_config.data_gpio_nums[2] = PIN_LCD_B2;
    panel_config.data_gpio_nums[3] = PIN_LCD_B3;
    panel_config.data_gpio_nums[4] = PIN_LCD_B4;
    panel_config.data_gpio_nums[5] = PIN_LCD_G0;
    panel_config.data_gpio_nums[6] = PIN_LCD_G1;
    panel_config.data_gpio_nums[7] = PIN_LCD_G2;
    panel_config.data_gpio_nums[8] = PIN_LCD_G3;
    panel_config.data_gpio_nums[9] = PIN_LCD_G4;
    panel_config.data_gpio_nums[10] = PIN_LCD_G5;
    panel_config.data_gpio_nums[11] = PIN_LCD_R0;
    panel_config.data_gpio_nums[12] = PIN_LCD_R1;
    panel_config.data_gpio_nums[13] = PIN_LCD_R2;
    panel_config.data_gpio_nums[14] = PIN_LCD_R3;
    panel_config.data_gpio_nums[15] = PIN_LCD_R4;

    esp_err_t ret = esp_lcd_new_rgb_panel(&panel_config, &_panel_handle);
    if (ret != ESP_OK) {
        Serial.printf("Failed to create RGB panel: %d\n", ret);
        return false;
    }

#if LCD7_BOUNCE_FILL_IN_TASK
    // Move the PSRAM->bounce-buffer copy into a high-priority task so the GDMA ISR
    // stays short (it only latches frame state and hands off the request).  This
    // is ON by default: the task's cached CPU copy meets the per-bounce-buffer
    // (~1 ms) deadline comfortably (0 drops in testing).  Set LCD7_BOUNCE_FILL_IN_TASK
    // to 0 to copy inline in the ISR instead (like the 2.8C driver), which is also
    // the automatic fallback if the task/queue can't be created.
    //
    // Created BEFORE refresh starts.  By default pinned to the core running
    // begin() -- the same core the GDMA ISR runs on -- so the hand-off is a
    // same-core wakeup; LCD7_BOUNCE_FILL_OTHER_CORE moves it to the other core.
    // If creation fails, _bounceQueue stays null and the ISR copies inline.
    BaseType_t fill_core = xPortGetCoreID();
#if LCD7_BOUNCE_FILL_OTHER_CORE
    fill_core ^= 1;   // pin to the opposite core (off the LVGL/UI core)
#endif
    _bounceQueue = xQueueCreate(LCD7_BOUNCE_QUEUE_LEN, sizeof(BounceFillRequest));
    if (_bounceQueue != nullptr) {
#if LCD7_BOUNCE_FILL_COOP
        // Cooperative dual-core fill: a low-priority worker on the OTHER (PRO/WiFi)
        // core, a high-priority backstop on the app core, and a checkpoint timer
        // that wakes the backstop if the worker stalls.  See the COOP block above.
        g_coopThreshCycles = (uint32_t)LCD7_COOP_CHECKPOINT_US * (uint32_t)getCpuFrequencyMhz();
        BaseType_t appCore   = xPortGetCoreID();
        BaseType_t otherCore = appCore ^ 1;
        BaseType_t okW = xTaskCreatePinnedToCore(
            _coopWorkerTask, "lcd7_bbcoop0", 4096, this,
            LCD7_COOP_WORKER_PRIORITY, &_bounceTask, otherCore);
        BaseType_t okB = xTaskCreatePinnedToCore(
            _coopBackstopTask, "lcd7_bbcoop1", 4096, this,
            LCD7_BOUNCE_TASK_PRIORITY, &g_coopBackstop, appCore);
        bool timerOk = false;
        gptimer_config_t tcfg = {};
        tcfg.clk_src = GPTIMER_CLK_SRC_DEFAULT;
        tcfg.direction = GPTIMER_COUNT_UP;
        tcfg.resolution_hz = 1000000;   // 1 tick = 1 us
        if (gptimer_new_timer(&tcfg, &g_coopTimer) == ESP_OK) {
            gptimer_event_callbacks_t tcbs = {};
            tcbs.on_alarm = coopTimerCb;
            gptimer_alarm_config_t acfg = {};
            acfg.reload_count = 0;
            acfg.alarm_count = LCD7_COOP_TICK_US;
            acfg.flags.auto_reload_on_alarm = true;
            timerOk = (gptimer_register_event_callbacks(g_coopTimer, &tcbs, nullptr) == ESP_OK) &&
                      (gptimer_set_alarm_action(g_coopTimer, &acfg) == ESP_OK) &&
                      (gptimer_enable(g_coopTimer) == ESP_OK) &&
                      (gptimer_start(g_coopTimer) == ESP_OK);
        }
        if (okW != pdPASS || okB != pdPASS || !timerOk) {
            Serial.println("Cooperative fill setup failed; falling back to single task");
            // Best-effort fallback to the normal single fill task on the app core.
            if (g_coopBackstop) { vTaskDelete(g_coopBackstop); g_coopBackstop = nullptr; }
            if (_bounceTask)    { vTaskDelete(_bounceTask);    _bounceTask = nullptr; }
            if (g_coopTimer)    { gptimer_del_timer(g_coopTimer); g_coopTimer = nullptr; }
            if (xTaskCreatePinnedToCore(_bounceFillTask, "lcd7_bbfill", 4096, this,
                    LCD7_BOUNCE_TASK_PRIORITY, &_bounceTask, fill_core) != pdPASS) {
                vQueueDelete(_bounceQueue);
                _bounceQueue = nullptr;   // ISR will copy inline
            }
        }
#else
        BaseType_t ok = xTaskCreatePinnedToCore(
            _bounceFillTask, "lcd7_bbfill", 4096, this,
            LCD7_BOUNCE_TASK_PRIORITY, &_bounceTask, fill_core);
        if (ok != pdPASS) {
            Serial.println("Bounce-fill task creation failed; copying inline in the ISR");
            vQueueDelete(_bounceQueue);
            _bounceQueue = nullptr;
        }
#endif
    } else {
        Serial.println("Bounce-fill queue creation failed; copying inline in the ISR");
    }
#endif

    // Register bounce buffer callback BEFORE init (when refresh starts)
    // With no_fb=true, the callback will be invoked to fill bounce buffers
    // Pass 'this' so callback can access framebuffer and scroll offsets
    esp_lcd_rgb_panel_event_callbacks_t cbs = {};
    cbs.on_bounce_empty = bounceBufferFillCallback;
    ret = esp_lcd_rgb_panel_register_event_callbacks(_panel_handle, &cbs, this);
    if (ret != ESP_OK) {
        Serial.printf("Failed to register bounce callback: %d\n", ret);
        return false;
    }

    ret = esp_lcd_panel_reset(_panel_handle);
    if (ret != ESP_OK) {
        Serial.printf("Failed to reset panel: %d\n", ret);
        return false;
    }

    ret = esp_lcd_panel_init(_panel_handle);
    if (ret != ESP_OK) {
        Serial.printf("Failed to init panel: %d\n", ret);
        return false;
    }

#if LCD7_BOUNCE_DESYNC_WORKAROUND
    // --- Desync detection setup --- (see extras/DESYNC_WORKAROUND.md)
    // Extract the GDMA channel from the panel's internal struct so we can cache a
    // pointer to the hardware register (out.dscr) that reveals which buffer the
    // DMA is reading from.  NOTE on timing: historically this ran right after
    // esp_lcd_new_rgb_panel() and worked; on ESP-IDF 5.5.4 dma_chan was null both
    // there AND here (after init), i.e. the esp_rgb_panel_partial_t offset no
    // longer matches -- re-derive it for your IDF (see the doc) before trusting
    // this.  The bounce callback guards on s_gdma_out_dscr_reg/s_bb_size.
    {
        esp_rgb_panel_partial_t* rgb_panel = (esp_rgb_panel_partial_t*)_panel_handle;
        gdma_channel_handle_t dma_chan = rgb_panel->dma_chan;
        int group_id = -1, channel_id = -1;
        if (gdma_get_group_channel_id(dma_chan, &group_id, &channel_id) == ESP_OK) {
            gdma_dev_t* gdma_hw = GDMA_LL_GET_HW(group_id);
            if (gdma_hw != nullptr) {
                s_gdma_out_dscr_reg = &gdma_hw->channel[channel_id].out.dscr;
                Serial.printf("Desync detection: GDMA group=%d ch=%d, out.dscr @%p\n",
                              group_id, channel_id, s_gdma_out_dscr_reg);
            }
        } else {
            Serial.printf("Desync detection: gdma_get_group_channel_id failed (dma_chan=%p)\n", dma_chan);
        }
        s_bb_size = panel_config.bounce_buffer_size_px * sizeof(uint16_t);
    }
    // --- End desync detection setup ---
#endif // LCD7_BOUNCE_DESYNC_WORKAROUND

    // If PSRAM allocation failed, prefill both bounce buffers with the
    // "MUST ENABLE PSRAM" error message. The DMA will cycle these forever
    // since the ISR does nothing when there's no framebuffer.
    if (!_framebuffer) {
        esp_rgb_panel_partial_t* rgb_panel = (esp_rgb_panel_partial_t*)_panel_handle;
        int bb_pixels = panel_config.bounce_buffer_size_px;
        // Each bounce buffer covers a different vertical slice of the screen.
        // Buffer 0: scanlines 0..19, buffer 1: scanlines 20..39, then they repeat.
        for (int b = 0; b < 2; b++) {
            if (rgb_panel->bounce_buffer[b]) {
                renderNoPsramMessage((uint16_t*)rgb_panel->bounce_buffer[b],
                                     b * bb_pixels, bb_pixels, _width);
            }
        }
    }

    // Clear screen to black
    fillScreen(0x0000);

    // Reveal the display: turn the backlight on (via the CH422G expander).
    setBacklight(100);

    return true;
}

void chipguy_S3_RGB_display::setBacklight(int percentage) {
    // On this board the LCD backlight is a CH422G expander pin (no PWM), so
    // brightness is on/off only: any positive percentage turns it on.
    _board.lcdBacklight(percentage > 0);
}

void chipguy_S3_RGB_display::setBacklight(bool on) {
    _board.lcdBacklight(on);
}

void chipguy_S3_RGB_display::pushImage(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t* pixels) {
    if (_draw_target == nullptr) return;
    // Bounds checking against padded framebuffer size (allows pre-drawing scroll content)
    if (x >= _fb_stride || y >= _fb_rows || x + w <= 0 || y + h <= 0) {
        return;
    }

    // Clip to padded framebuffer bounds
    int16_t x1 = max((int16_t)0, x);
    int16_t y1 = max((int16_t)0, y);
    int16_t x2 = min((int16_t)(_fb_stride - 1), (int16_t)(x + w - 1));
    int16_t y2 = min((int16_t)(_fb_rows - 1), (int16_t)(y + h - 1));

    int16_t src_offset_x = x1 - x;
    int16_t src_offset_y = y1 - y;
    int16_t clipped_w = x2 - x1 + 1;
    int16_t clipped_h = y2 - y1 + 1;

    // Copy pixels row by row using linear addressing
    for (int16_t row = 0; row < clipped_h; row++) {
        const uint16_t* src = pixels + ((src_offset_y + row) * w) + src_offset_x;

        // Linear position in ring buffer
        uint32_t fb_pos = (_pending_scroll_offset + (y1 + row) * _fb_stride + x1) % _fb_size;

        // Each row is contiguous - only need to handle wrap at buffer end
        int16_t pixels_remaining = clipped_w;
        int16_t src_offset = 0;

        while (pixels_remaining > 0) {
            int pixels_to_wrap = _fb_size - fb_pos;
            int segment = (pixels_remaining < pixels_to_wrap) ? pixels_remaining : pixels_to_wrap;

            memcpy(_draw_target + fb_pos, src + src_offset, segment * sizeof(uint16_t));
            cacheWriteBack(_draw_target + fb_pos, segment * sizeof(uint16_t));

            pixels_remaining -= segment;
            src_offset += segment;
            fb_pos = (fb_pos + segment) % _fb_size;
        }
    }
}

void chipguy_S3_RGB_display::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    if (_draw_target == nullptr) return;
    // Bounds checking against padded framebuffer size (allows pre-drawing scroll content)
    if (x >= _fb_stride || y >= _fb_rows || x + w <= 0 || y + h <= 0) {
        return;
    }

    // Clip to padded framebuffer bounds
    int16_t x1 = max((int16_t)0, x);
    int16_t y1 = max((int16_t)0, y);
    int16_t x2 = min((int16_t)(_fb_stride - 1), (int16_t)(x + w - 1));
    int16_t y2 = min((int16_t)(_fb_rows - 1), (int16_t)(y + h - 1));

    int16_t clipped_w = x2 - x1 + 1;
    int16_t clipped_h = y2 - y1 + 1;

    // Fill pixels using linear addressing
    for (int16_t row = 0; row < clipped_h; row++) {
        // Linear position in ring buffer
        uint32_t fb_pos = (_pending_scroll_offset + (y1 + row) * _fb_stride + x1) % _fb_size;

        // Each row is contiguous - only need to handle wrap at buffer end
        int16_t pixels_remaining = clipped_w;

        while (pixels_remaining > 0) {
            int pixels_to_wrap = _fb_size - fb_pos;
            int segment = (pixels_remaining < pixels_to_wrap) ? pixels_remaining : pixels_to_wrap;

            uint16_t* dst = _draw_target + fb_pos;
            for (int i = 0; i < segment; i++) {
                dst[i] = color;
            }
            cacheWriteBack(dst, segment * sizeof(uint16_t));

            pixels_remaining -= segment;
            fb_pos = (fb_pos + segment) % _fb_size;
        }
    }
}

void chipguy_S3_RGB_display::fillScreen(uint16_t color) {
    fillRect(0, 0, _width, _height, color);
}

void chipguy_S3_RGB_display::drawPixel(int16_t x, int16_t y, uint16_t color) {
    if (_draw_target == nullptr) return;
    // Bounds checking against padded framebuffer size (allows pre-drawing scroll content)
    if (x < 0 || x >= _fb_stride || y < 0 || y >= _fb_rows) {
        return;
    }

    // Linear position in ring buffer
    uint32_t fb_pos = (_pending_scroll_offset + y * _fb_stride + x) % _fb_size;

    uint16_t* pixel = _draw_target + fb_pos;
    *pixel = color;

    // Flush this cache line (align to 64-byte boundary for PSRAM)
    uint32_t addr = (uint32_t)pixel & ~63;
    Cache_WriteBack_Addr(addr, 64);
}

void chipguy_S3_RGB_display::flush() {
    if (_framebuffer == nullptr) return;
    // Write back entire framebuffer row by row
    const size_t row_bytes = _fb_stride * sizeof(uint16_t);
    for (int row = 0; row < _fb_rows; row++) {
        cacheWriteBack(_framebuffer + row * _fb_stride, row_bytes);
    }
}

bool chipguy_S3_RGB_display::scrollFramebuffer(int16_t dx, int16_t dy, uint16_t fill_color) {
    if (!_framebuffer) {
        return false;
    }

    // Note: This function performs memory-copy scrolling (legacy).
    // For hardware scrolling without memory copies, use scroll() instead.

    // Vertical scrolling using SRAM staging buffer (dx == 0)
    if (dx == 0 && dy != 0) {
        // Allocate staging buffer on stack (full stride * 2 bytes)
        uint16_t line_buffer[_fb_stride];

        int16_t src_row_start;
        int16_t dst_row_start;
        int16_t num_rows_to_copy;

        if (dy < 0) {
            // Scroll up: move rows [abs(dy)...height-1] to [0...height-abs(dy)-1]
            src_row_start = -dy;
            dst_row_start = 0;
            num_rows_to_copy = _height - (-dy);
        } else {
            // Scroll down: move rows [0...height-dy-1] to [dy...height-1]
            src_row_start = 0;
            dst_row_start = dy;
            num_rows_to_copy = _height - dy;
        }

        // Copy rows line by line using SRAM staging
        // Copy direction matters for overlapping regions
        if (dy > 0) {
            // Scrolling down: copy from bottom to top to avoid overwriting source
            for (int16_t i = num_rows_to_copy - 1; i >= 0; i--) {
                uint16_t* src_line = _framebuffer + (src_row_start + i) * _fb_stride;
                uint16_t* dst_line = _framebuffer + (dst_row_start + i) * _fb_stride;

                // PSRAM -> SRAM -> PSRAM (copy visible width only)
                memcpy(line_buffer, src_line, _width * 2);
                memcpy(dst_line, line_buffer, _width * 2);
                cacheWriteBack(dst_line, _width * 2);
            }
        } else {
            // Scrolling up: copy from top to bottom to avoid overwriting source
            for (int16_t i = 0; i < num_rows_to_copy; i++) {
                uint16_t* src_line = _framebuffer + (src_row_start + i) * _fb_stride;
                uint16_t* dst_line = _framebuffer + (dst_row_start + i) * _fb_stride;

                // PSRAM -> SRAM -> PSRAM (copy visible width only)
                memcpy(line_buffer, src_line, _width * 2);
                memcpy(dst_line, line_buffer, _width * 2);
                cacheWriteBack(dst_line, _width * 2);
            }
        }

        return false;  // Fill not performed - caller should handle exposed area
    }

    // Horizontal or diagonal scrolling - copy line by line with stack-allocated staging buffer
    // Calculate source and destination regions
    int16_t src_x = (dx > 0) ? 0 : -dx;
    int16_t src_y = (dy > 0) ? 0 : -dy;
    int16_t dst_x = (dx > 0) ? dx : 0;
    int16_t dst_y = (dy > 0) ? dy : 0;
    int16_t copy_width = _width - abs(dx);
    int16_t copy_height = _height - abs(dy);

    // Allocate staging buffer on stack (visible width * 2 bytes)
    uint16_t line_buffer[_width];

    // Pre-fill the line buffer with fill_color once
    // This way we only read pixels that will actually move, and write a complete line
    for (int16_t x = 0; x < _width; x++) {
        line_buffer[x] = fill_color;
    }

    // Copy line by line using optimized PSRAM->SRAM->PSRAM
    if (dy > 0) {
        // Scrolling down: copy from bottom to top
        for (int16_t y = copy_height - 1; y >= 0; y--) {
            uint16_t* src_line = _framebuffer + (src_y + y) * _fb_stride + src_x;
            uint16_t* dst_line = _framebuffer + (dst_y + y) * _fb_stride;

            if (dx > 0) {
                // Scrolling right: read pixels directly to their final position in buffer
                memcpy(line_buffer + dx, src_line, copy_width * 2);
                // Left side (0 to dx-1) already has fill_color from pre-fill
            } else if (dx < 0) {
                // Scrolling left: read pixels directly to their final position
                memcpy(line_buffer, src_line, copy_width * 2);
                // Right side (copy_width to _width-1) already has fill_color from pre-fill
            } else {
                // No horizontal scroll: just copy pixels
                memcpy(line_buffer, src_line, copy_width * 2);
            }

            // Write complete line from SRAM to PSRAM (one burst write)
            memcpy(dst_line, line_buffer, _width * 2);
            cacheWriteBack(dst_line, _width * 2);
        }

        // Fill the top exposed rows with fill_color
        for (int16_t y = 0; y < dy; y++) {
            uint16_t* fill_line = _framebuffer + y * _fb_stride;
            memcpy(fill_line, line_buffer, _width * 2);  // Use pre-filled buffer
            cacheWriteBack(fill_line, _width * 2);
        }
    } else {
        // Scrolling up or no vertical scroll: copy from top to bottom
        for (int16_t y = 0; y < copy_height; y++) {
            uint16_t* src_line = _framebuffer + (src_y + y) * _fb_stride + src_x;
            uint16_t* dst_line = _framebuffer + (dst_y + y) * _fb_stride;

            if (dx > 0) {
                // Scrolling right: read pixels directly to their final position in buffer
                memcpy(line_buffer + dx, src_line, copy_width * 2);
                // Left side (0 to dx-1) already has fill_color from pre-fill
            } else if (dx < 0) {
                // Scrolling left: read pixels directly to their final position
                memcpy(line_buffer, src_line, copy_width * 2);
                // Right side (copy_width to _width-1) already has fill_color from pre-fill
            } else {
                // No horizontal scroll: just copy pixels
                memcpy(line_buffer, src_line, copy_width * 2);
            }

            // Write complete line from SRAM to PSRAM (one burst write)
            memcpy(dst_line, line_buffer, _width * 2);
            cacheWriteBack(dst_line, _width * 2);
        }

        // Fill the bottom exposed rows with fill_color
        if (dy < 0) {
            for (int16_t y = _height + dy; y < _height; y++) {
                uint16_t* fill_line = _framebuffer + y * _fb_stride;
                memcpy(fill_line, line_buffer, _width * 2);  // Use pre-filled buffer
                cacheWriteBack(fill_line, _width * 2);
            }
        }
    }

    return true;  // Fill was performed during scroll
}

void chipguy_S3_RGB_display::setScrollOffset(int16_t x, int16_t y) {
    // Compute linear offset from x,y coordinates
    // Normalize to positive values within buffer bounds
    int32_t offset = y * _fb_stride + x;
    // Handle negative values by adding fb_size until positive
    while (offset < 0) offset += _fb_size;
    _pending_scroll_offset = offset % _fb_size;
}

void chipguy_S3_RGB_display::scroll(int16_t dx, int16_t dy, uint16_t fill_color, bool fill) {
    if (_framebuffer == nullptr) return;
    if (dx == 0 && dy == 0) return;

    // Calculate new scroll offset: add linear delta to current offset
    // delta = dx + dy * fb_stride (horizontal + vertical contribution)
    int32_t delta = dx + dy * (int32_t)_fb_stride;
    int32_t new_offset = (int32_t)_pending_scroll_offset + delta;

    // Normalize to positive value within buffer
    while (new_offset < 0) new_offset += _fb_size;
    new_offset = new_offset % _fb_size;

    // Track how much we can pre-draw vs post-draw
    int16_t abs_dy = (dy > 0) ? dy : -dy;
    int16_t abs_dx = (dx > 0) ? dx : -dx;
    int16_t predraw_dy = (abs_dy <= _fb_pad_y) ? abs_dy : _fb_pad_y;
    int16_t predraw_dx = (abs_dx <= _fb_pad_x) ? abs_dx : _fb_pad_x;
    int16_t postdraw_dy = abs_dy - predraw_dy;
    int16_t postdraw_dx = abs_dx - predraw_dx;

    // Pre-draw fill into padding area (before changing scroll offset)
    // This writes to framebuffer positions that will be visible after scroll
    if (fill && (predraw_dy > 0 || predraw_dx > 0)) {
        // Pre-fill vertical exposed area
        if (predraw_dy > 0) {
            int16_t screen_y_base = (dy > 0) ? (_height - predraw_dy) : 0;
            for (int16_t row = 0; row < predraw_dy; row++) {
                int16_t screen_y = screen_y_base + row;
                uint32_t fb_pos = (new_offset + screen_y * _fb_stride) % _fb_size;
                for (int16_t x = 0; x < _width; x++) {
                    _framebuffer[(fb_pos + x) % _fb_size] = fill_color;
                }
                cacheWriteBack(_framebuffer + fb_pos, _width * sizeof(uint16_t));
            }
        }

        // Pre-fill horizontal exposed area
        if (predraw_dx > 0) {
            int16_t screen_x_base = (dx > 0) ? (_width - predraw_dx) : 0;
            for (int16_t y = 0; y < _height; y++) {
                uint32_t fb_pos = (new_offset + y * _fb_stride + screen_x_base) % _fb_size;
                for (int16_t col = 0; col < predraw_dx; col++) {
                    _framebuffer[(fb_pos + col) % _fb_size] = fill_color;
                }
                cacheWriteBack(_framebuffer + fb_pos, predraw_dx * sizeof(uint16_t));
            }
        }
    }

    // Update pending scroll offset (takes effect at next frame start)
    _pending_scroll_offset = new_offset;

    // Post-draw any remainder that didn't fit in padding
    if (fill && (postdraw_dy > 0 || postdraw_dx > 0)) {
        if (postdraw_dy > 0) {
            if (dy > 0) {
                // Scrolling down - fill remaining bottom rows
                fillRect(0, _height - abs_dy, _width, postdraw_dy, fill_color);
            } else {
                // Scrolling up - fill remaining top rows (after the pre-drawn ones)
                fillRect(0, predraw_dy, _width, postdraw_dy, fill_color);
            }
        }

        if (postdraw_dx > 0) {
            if (dx > 0) {
                // Scrolling right - fill remaining right columns
                fillRect(_width - abs_dx, 0, postdraw_dx, _height, fill_color);
            } else {
                // Scrolling left - fill remaining left columns (after the pre-drawn ones)
                fillRect(predraw_dx, 0, postdraw_dx, _height, fill_color);
            }
        }
    }
}

// Sprite API implementations

void chipguy_S3_RGB_display::setSprite(uint8_t slot, const uint16_t* data, int16_t x, int16_t y,
                                uint16_t w, uint16_t h, uint16_t transparent,
                                uint32_t flags) {
    if (slot >= SPRITE_MAX_COUNT) return;

    // Temporarily mark as disabled while updating to ensure atomic visibility
    _pending_sprites[slot].yx = SPRITE_DISABLED;

    // Set all properties
    _pending_sprites[slot].data = data;
    _pending_sprites[slot].hw = SPRITE_HW(h, w);
    _pending_sprites[slot].transparent = transparent;
    _pending_sprites[slot].flags = flags;

    // Enable sprite by setting valid coordinates (do this last)
    _pending_sprites[slot].yx = SPRITE_YX(y, x);
}

void chipguy_S3_RGB_display::clearSprite(uint8_t slot) {
    if (slot >= SPRITE_MAX_COUNT) return;
    _pending_sprites[slot].yx = SPRITE_DISABLED;
}

void chipguy_S3_RGB_display::clearAllSprites() {
    for (int i = 0; i < SPRITE_MAX_COUNT; i++) {
        _pending_sprites[i].yx = SPRITE_DISABLED;
    }
}

bool chipguy_S3_RGB_display::checkSpriteCollision(uint8_t slot) {
    if (slot >= SPRITE_MAX_COUNT) return false;

    // Check if collision flag is set and clear it atomically
    if (_sprite_collisions[slot]) {
        _sprite_collisions[slot] = 0;
        return true;
    }
    return false;
}

void chipguy_S3_RGB_display::beginFlashMessage(int16_t y) {
    // Already active or pending -- don't double-allocate
    if (s_flashMsgPending != nullptr) return;

    // y == -1: blank screen only, no message text.
    // Use the sentinel non-null value so the ISR enters the flash overlay path
    // (which fills black first) but finds no message rows to blit.
    if (y == -1) {
        s_flashMsgW = 0;
        s_flashMsgH = 0;
        s_flashMsgStride = 0;
        s_flashMsgX = 0;
        s_flashMsgY = 0;
        s_flashMsgTile = false;
        __asm__ __volatile__("memw" ::: "memory");
        s_flashMsgPending = &s_blankSentinel;
    } else {
        int w = 0, h = 0, stride = 0;
        uint8_t* buf = renderMessageTo1bpp(s_flashWriteMsg, s_flashWriteMsgLen, 2, &w, &h, &stride);
        if (!buf) return;

        int bounceLines = _bounce_buffer_size_px / _width;  // 20 for 800px wide
        int msgY;
        bool tile = false;

        if (y == -3) {
            // Tile: message appears in every bounce buffer band, so it's visible
            // even if the DMA replays a single chunk during a flash stall.
            // msgY is the offset within each band; center the text vertically.
            msgY = (bounceLines - h) / 2;
            if (msgY < 0) msgY = 0;
            tile = true;
        } else if (y == -2) {
            // Default: center vertically, snapped to bounce buffer boundary
            msgY = ((_height - h) / 2);
            msgY = (msgY / bounceLines) * bounceLines;
        } else {
            msgY = (int)y;
            // Round down to bounce buffer boundary
            msgY = (msgY / bounceLines) * bounceLines;
            // Clamp to screen
            if (msgY < 0) msgY = 0;
            if (msgY + h > _height) msgY = ((_height - h) / bounceLines) * bounceLines;
        }

        s_flashMsgW = w;
        s_flashMsgH = h;
        s_flashMsgStride = stride;
        s_flashMsgX = (_width - w) / 2;
        s_flashMsgY = msgY;
        s_flashMsgTile = tile;

        __asm__ __volatile__("memw" ::: "memory");
        s_flashMsgPending = buf;
    }

    // Block until the ISR has applied the change (next frame boundary),
    // then wait for one more complete frame to ensure every bounce buffer
    // chunk has been rendered in flash-overlay mode before we return.
    // This prevents the caller's flash write from starting while the ISR
    // is still mid-frame reading from PSRAM.
    while (s_flashMsgActive != s_flashMsgPending) {
        vTaskDelay(1);
    }
    // Wait for a full frame: at minimum 24 ISR calls (480 lines / 20 lines per chunk).
    // We detect a full frame by waiting for pos_px == 0 to occur again, but since
    // we can't easily observe pos_px from here, we use a simple frame counter.
    uint32_t startCount = g_isrCallCount;
    while ((g_isrCallCount - startCount) < 25) {
        vTaskDelay(1);
    }
}

void chipguy_S3_RGB_display::endFlashMessage() {
    // Save pointer for freeing after the ISR stops using it
    uint8_t* buf = (uint8_t*)s_flashMsgPending;
    if (!buf) return;

    // Clear pending -- the ISR will copy null to active at the next frame start
    s_flashMsgPending = nullptr;

    // Block until the ISR has applied the change (next frame boundary)
    while (s_flashMsgActive != nullptr) {
        vTaskDelay(1);
    }

    // Now safe to free -- the ISR is no longer referencing the buffer.
    // Don't free the static blank sentinel used by beginFlashMessage(-1).
    if (buf != &s_blankSentinel) {
        heap_caps_free(buf);
    }
}

#endif // CONFIG_IDF_TARGET_ESP32S3
