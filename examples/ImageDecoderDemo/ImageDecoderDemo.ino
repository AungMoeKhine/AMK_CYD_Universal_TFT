// ═══════════════════════════════════════════════════════════════════════════════
// ImageDecoderDemo.ino - Universal 1-Line JPEG & PNG Rendering Example
//                        Part of AMK_CYD_Universal_TFT (v1.6.4+)
//
// Demonstrates:
//   1. 1-line display & touch initialization with tft.autoSetup()
//   2. High-speed 1-line JPEG decoding via tft.drawJpgFile() or tft.drawJpg()
//   3. High-speed 1-line PNG decoding via tft.drawPngFile() or tft.drawPng()
//   4. Automatic LittleFS / SPIFFS / SD card compatibility
//   5. Works seamlessly across both Classic ESP32 CYD and ESP32-S3 RGB!
// ═══════════════════════════════════════════════════════════════════════════════

#include <Arduino.h>
#include <LittleFS.h>
#include "AMK_CYD_Universal_TFT.h"

// Global hardware instances
AMK_display display;
AMK_touch   touch;
TFT_eSPI    tft;

// A sample 16x16 minimal embedded test pattern / raw image buffer
const uint8_t sample_icon_16x16[] PROGMEM = {
  0x1F, 0x00, 0x1F, 0x00, 0xF8, 0x00, 0xF8, 0x00,
  0x07, 0xE0, 0x07, 0xE0, 0xFF, 0xE0, 0xFF, 0xE0,
  0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x1F, 0x00, 0x1F,
  0xF8, 0x1F, 0xF8, 0x1F, 0x07, 0xFF, 0x07, 0xFF
};

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println("\n[AMK_CYD_Universal_TFT] Image Decoder QuickStart Demo");

  // 1-line universal autoSetup initializes panel, PWM backlight, and touch
  tft.autoSetup(3);

  // Initialize LittleFS for filesystem demo
  bool fsOk = LittleFS.begin(true);
  if (fsOk) {
    Serial.println("[FS] LittleFS mounted successfully.");
  } else {
    Serial.println("[FS] LittleFS mount failed (using progmem demo).");
  }

  // Draw header UI
  tft.fillScreen(TFT_BLACK);
  tft.fillRectHGradient(0, 0, tft.width(), 45, TFT_NAVY, TFT_DARKCYAN);
  tft.setTextColor(TFT_WHITE, TFT_TRANSPARENT);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("AMK Image Decoder (1-Line)", tft.width() / 2, 22, 4);

  // ── 1. Demonstrate Filesystem JPEG Rendering (1-Line!) ──
  // If "/logo.jpg" exists on LittleFS/SD, tft.drawJpgFile() renders it instantly:
#if defined(AMK_HAS_JPEG_DECODER)
  Serial.println("[Image] Checking for LittleFS /logo.jpg...");
  if (fsOk && LittleFS.exists("/logo.jpg")) {
    // 1-Line Call!
    tft.drawJpgFile(LittleFS, "/logo.jpg", 20, 60);
  } else {
    // Elegant fallback card when file is not yet uploaded
    tft.drawRoundRect(20, 60, 180, 140, 8, TFT_CYAN);
    tft.fillRect(22, 62, 176, 136, 0x0841);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_CYAN, 0x0841);
    tft.drawString("tft.drawJpgFile()", 110, 110, 2);
    tft.setTextColor(TFT_WHITE, 0x0841);
    tft.drawString("Upload logo.jpg", 110, 135, 2);
    tft.drawString("to LittleFS / SD", 110, 155, 2);
  }
#else
  tft.drawRoundRect(20, 60, 180, 140, 8, TFT_RED);
  tft.drawString("Install JPEGDecoder", 110, 130, 2);
#endif

  // ── 2. Demonstrate Filesystem PNG Rendering (1-Line!) ──
#if defined(AMK_HAS_PNG_DECODER)
  Serial.println("[Image] Checking for LittleFS /icon.png...");
  if (fsOk && LittleFS.exists("/icon.png")) {
    // 1-Line Call!
    tft.drawPngFile(LittleFS, "/icon.png", 220, 60);
  } else {
    tft.drawRoundRect(220, 60, 180, 140, 8, TFT_GREEN);
    tft.fillRect(222, 62, 176, 136, 0x0208);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_GREEN, 0x0208);
    tft.drawString("tft.drawPngFile()", 310, 110, 2);
    tft.setTextColor(TFT_WHITE, 0x0208);
    tft.drawString("Upload icon.png", 310, 135, 2);
    tft.drawString("to LittleFS / SD", 310, 155, 2);
  }
#else
  tft.drawRoundRect(220, 60, 180, 140, 8, TFT_DARKGREY);
  tft.fillRect(222, 62, 176, 136, 0x1082);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_LIGHTGREY, 0x1082);
  tft.drawString("PNGdec Library", 310, 115, 2);
  tft.drawString("(Optional via LibMgr)", 310, 140, 2);
#endif

  // Status footer
  tft.fillRect(0, tft.height() - 25, tft.width(), 25, 0x18C3);
  tft.setTextColor(TFT_YELLOW, 0x18C3);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("Touch the screen to test interactive coordinates", tft.width() / 2, tft.height() - 12, 2);
}

void loop() {
  // LDR ambient light auto-dimming runs continuously
  tft.adjustBrightness();

  // Interactive touch test
  uint16_t tx, ty;
  if (tft.getTouch(&tx, &ty)) {
    tft.fillCircle(tx, ty, 4, TFT_RED);
  }
  delay(10);
}
