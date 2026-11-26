#pragma once
// ═══════════════════════════════════════════════════════════════════════════════
// AMK_User_Setup.h - Custom Hardware Configuration File
// Target: ESP32-S3 Dev Module + 2.8" Red SPI TFT (ILI9341 + XPT2046 Touch)
//
// HOW TO USE:
//   1. Place this file directly in your sketch directory (project-local), OR
//   2. Place it in Arduino/libraries/AMK_CYD_Universal_TFT/ (library-global).
//   3. Uncomment the driver and pin lines matching your hardware wiring.
// ═══════════════════════════════════════════════════════════════════════════════

#define AMK_CUSTOM_SETUP 1

// ── 1. Display Controller Driver (Uncomment ONLY ONE) ─────────────────────────
#define ILI9341_DRIVER       // Standard 2.8" Red SPI Display (240x320)
//#define ST7789_DRIVER      // 2.4" or 2.8" ST7789 SPI Panel (240x320)
//#define ST7796_DRIVER      // 3.5" or 4.0" ST7796 SPI Panel (320x480)

// ── 2. ESP32-S3 SPI Display Bus Pins (TFT_eSPI Standard Names) ────────────────
#define TFT_MOSI  11         // Master Out Slave In (Display SDI)
#define TFT_SCLK  12         // SPI Clock (Display SCK)
#define TFT_MISO  13         // Master In Slave Out (Display SDO / Touch DOUT)
#define TFT_CS    10         // Display Chip Select
#define TFT_DC     9         // Data / Command Select (DC / RS)
#define TFT_RST    8         // Display Hardware Reset (Active LOW)

// ── 3. Backlight & Touch Controller Pins ─────────────────────────────────────
#define TFT_BL     7         // Backlight PWM Control Pin (LCD LED Pin)
#define TOUCH_CS   6         // XPT2046 Touch Chip Select (T_CS)
#define TOUCH_IRQ  5         // XPT2046 Touch Interrupt Pin (T_IRQ)

// ── 4. SPI Frequency (Clock Speed) ───────────────────────────────────────────
#define SPI_FREQUENCY  40000000  // 40 MHz (fast, optimal for short wires / PCB)
//#define SPI_FREQUENCY 27000000 // 27 MHz (stable fallback if using long jumper wires)

// ── 5. AMK Signature Feature Extensions ──────────────────────────────────────
#define TFT_LDR    4         // Auto-Dimming Ambient Light Sensor Pin (ADC1 Channel 3)
