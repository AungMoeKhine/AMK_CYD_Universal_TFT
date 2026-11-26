/*
 * Custom_Hardware_Setup.ino - AMK_CYD_Universal_TFT Custom Setup Example
 * 
 * Demonstrates how to use AMK_CYD_Universal_TFT with custom pin assignments
 * across any ESP32, ESP32-S3, or custom board using AMK_User_Setup.h.
 * 
 * Hardware Wiring:
 *   Display:
 *     VCC        -> 3.3V or 5V (depending on J1 jumper)
 *     GND        -> GND
 *     CS         -> GPIO 10
 *     RESET      -> GPIO 8
 *     DC / RS    -> GPIO 9
 *     SDI (MOSI) -> GPIO 11
 *     SCK        -> GPIO 12
 *     LED (BL)   -> GPIO 7
 *     SDO (MISO) -> GPIO 13
 *   Touch (XPT2046):
 *     T_CLK      -> GPIO 12 (Shared SCK)
 *     T_DIN      -> GPIO 11 (Shared MOSI)
 *     T_DO       -> GPIO 13 (Shared MISO)
 *     T_CS       -> GPIO 6
 *     T_IRQ      -> GPIO 5
 *   Sensors:
 *     LDR (Light)-> GPIO 4 (ADC1 Channel 3 with 10k pull-down to GND)
 */

// ═══════════════════════════════════════════════════════════════════════════════
// HARDWARE SETUP CONFIGURATION (Custom Setup vs Standard CYD Auto-Detection):
// ═══════════════════════════════════════════════════════════════════════════════
// 1. AUTO-DISCOVERY (Default & Recommended):
//    The library automatically checks for "AMK_User_Setup.h" in your sketch
//    folder via C++ __has_include. If found, custom pins & settings are loaded
//    automatically with zero extra lines of code!
//
// 2. EXPLICIT DECLARATION (Self-Documenting):
//    You can also explicitly include "AMK_User_Setup.h" right here in your sketch.
//    This clearly documents to anyone reading the code that custom hardware pins
//    are active, and ensures IDE IntelliSense immediately resolves all pin macros:
//      #include "AMK_User_Setup.h"
//
// 3. ZERO-CONFIG CYD AUTO-PROBING:
//    To run this sketch on a standard Cheap Yellow Display (e.g. ESP32-2432S028R,
//    ST7789, ST7796), simply rename or delete "AMK_User_Setup.h" (e.g. to
//    AMK_User_Setup.h.bak). The library will automatically fall back to 100%
//    zero-config CYD auto-detection!
// ═══════════════════════════════════════════════════════════════════════════════

#include <Arduino.h>

// Step 1: Explicitly include custom setup header (or let library auto-discover it)
#if __has_include("AMK_User_Setup.h")
  #include "AMK_User_Setup.h"
#endif

// Step 2: Include the Universal TFT library
#include <AMK_CYD_Universal_TFT.h>

// Global Driver Objects
AMK_display display;
AMK_touch   touch;
TFT_eSPI    tft;
TFT_eSprite sprite(&tft);

const uint8_t screenRotation = 3; // Landscape (320x240)
int lastTouchX = -1;
int lastTouchY = -1;

void drawDashboard() {
  tft.fillScreen(TFT_BLACK);

  // Header Bar
  tft.fillRect(0, 0, 320, 32, TFT_NAVY);
  tft.setTextColor(TFT_WHITE, TFT_NAVY);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("AMK Custom Hardware Setup", 160, 16, 2);

  // Hardware Status Cards
  tft.drawRoundRect(10, 42, 145, 80, 6, TFT_DARKGREY);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setTextDatum(TL_DATUM);
  tft.drawString("MCU & MEMORY", 18, 48, 1);
  
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
#if defined(CONFIG_IDF_TARGET_ESP32S3)
  tft.drawString("Target: ESP32-S3", 18, 62, 1);
#elif defined(CONFIG_IDF_TARGET_ESP32)
  tft.drawString("Target: ESP32", 18, 62, 1);
#else
  tft.drawString("Target: ESP Family", 18, 62, 1);
#endif
  tft.drawString("Free Heap: " + String(ESP.getFreeHeap() / 1024) + " KB", 18, 76, 1);
#if defined(BOARD_HAS_PSRAM)
  tft.drawString("PSRAM: " + String(ESP.getFreePsram() / 1024) + " KB", 18, 90, 1);
#else
  tft.drawString("PSRAM: Disabled", 18, 90, 1);
#endif
  tft.drawString("Min Heap: " + String(ESP.getMinFreeHeap() / 1024) + " KB", 18, 104, 1);

  // Pin Configuration Card (Dynamic Macro Resolution)
  tft.drawRoundRect(165, 42, 145, 80, 6, TFT_DARKGREY);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.drawString("ASSIGNED PINS", 173, 48, 1);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);

#if (AMK_CUSTOM_SETUP_ACTIVE == 1)
  #if defined(TFT_MOSI) && defined(TFT_SCLK)
  tft.drawString("MOSI: " + String(TFT_MOSI) + " | SCK: " + String(TFT_SCLK), 173, 62, 1);
  #elif defined(AMK_PIN_MOSI) && defined(AMK_PIN_SCLK)
  tft.drawString("MOSI: " + String(AMK_PIN_MOSI) + " | SCK: " + String(AMK_PIN_SCLK), 173, 62, 1);
  #endif

  #if defined(TFT_MISO) && defined(TFT_CS)
  tft.drawString("MISO: " + String(TFT_MISO) + " | CS:  " + String(TFT_CS), 173, 76, 1);
  #elif defined(AMK_PIN_MISO) && defined(AMK_PIN_CS)
  tft.drawString("MISO: " + String(AMK_PIN_MISO) + " | CS:  " + String(AMK_PIN_CS), 173, 76, 1);
  #endif

  #if defined(TFT_DC) && defined(TFT_RST)
  tft.drawString("DC:   " + String(TFT_DC) + " | RST: " + String(TFT_RST), 173, 90, 1);
  #elif defined(AMK_PIN_DC)
  tft.drawString("DC:   " + String(AMK_PIN_DC), 173, 90, 1);
  #endif

  #if defined(TFT_BL) && defined(AMK_ACTIVE_LDR_PIN)
  tft.drawString("BL:   " + String(TFT_BL) + " | LDR: " + String(AMK_ACTIVE_LDR_PIN), 173, 104, 1);
  #else
  tft.drawString("BL:   " + String(display.backlightPin()), 173, 104, 1);
  #endif
#else
  tft.drawString("Auto CYD Wiring", 173, 62, 1);
  tft.drawString("BL:  GPIO " + String(display.backlightPin()), 173, 76, 1);
  tft.drawString("LDR: GPIO " + String(AMK_ACTIVE_LDR_PIN), 173, 90, 1);
  tft.drawString("Touch: Auto-Probe", 173, 104, 1);
#endif

  // Interactive Touch Area
  tft.drawRoundRect(10, 130, 300, 75, 6, TFT_BLUE);
  tft.setTextColor(TFT_GREENYELLOW, TFT_BLACK);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("TOUCH TEST PAD", 160, 145, 2);
  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.drawString("Touch anywhere in this box to test XPT2046", 160, 168, 1);
  tft.drawString("Coordinates: (Waiting for touch...)", 160, 185, 1);

  // Footer status bar
  tft.fillRect(0, 218, 320, 22, 0x18E3); // Dark slate
  tft.setTextColor(TFT_WHITE, 0x18E3);
  tft.setTextDatum(ML_DATUM);
  tft.drawString("Auto-Dimming: Active (GPIO " + String(AMK_ACTIVE_LDR_PIN) + ")", 10, 229, 1);
}

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("\n══════════════════════════════════════════════");
  Serial.println("  AMK_CYD_Universal_TFT v1.7.0 Custom Setup   ");
  Serial.println("  Universal Custom Hardware Configuration    ");
  Serial.println("══════════════════════════════════════════════");
#if (AMK_CUSTOM_SETUP_ACTIVE == 1)
  Serial.println("[Setup Mode] Custom Hardware Setup ACTIVE (AMK_User_Setup.h)");
#else
  Serial.println("[Setup Mode] Standard CYD Auto-Probing ACTIVE (Zero-Config)");
#endif

  // 1. Initialize TFT Display hardware (Reads AMK_User_Setup.h automatically)
  tft.init();
  tft.setRotation(screenRotation);
  tft.setSwapBytes(true);

  // 2. Configure Backlight PWM on auto-detected / custom pin
  uint8_t blPin = display.backlightPin();
  pinMode(blPin, OUTPUT);
  analogWriteResolution(blPin, 8);
  analogWrite(blPin, 160);

  // 3. Prime the LDR filter moving average buffer at boot
  for (int i = 0; i < 10; i++) {
    tft.adjustBrightness();
    delay(10);
  }

  // 4. Draw Dashboard UI
  drawDashboard();

  // 5. Create a dynamic test sprite
  sprite.setColorDepth(16);
  sprite.createSprite(100, 20);
}

void loop() {
  // First-line execution standard: smooth LDR auto-dimming
  tft.adjustBrightness();

  // Handle Touch Screen Coordinates
  uint16_t x = 0, y = 0;
  if (touch.touched()) {
    if (touch.getXY(x, y)) {
      if (x != lastTouchX || y != lastTouchY) {
        lastTouchX = x;
        lastTouchY = y;

        // Draw crosshair on touched coordinate
        tft.fillCircle(x, y, 3, TFT_RED);

        // Update coordinate readouts using sprite (flicker-free)
        sprite.fillSprite(TFT_BLACK);
        sprite.setTextColor(TFT_GREEN, TFT_BLACK);
        sprite.setTextDatum(MC_DATUM);
        sprite.drawString("X:" + String(x) + " Y:" + String(y), 50, 10, 2);
        sprite.pushSprite(110, 178);

        Serial.printf("[Touch] X: %u, Y: %u\n", x, y);
      }
    }
  }

  delay(20);
}
