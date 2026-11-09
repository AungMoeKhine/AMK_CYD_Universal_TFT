#pragma once
// ═══════════════════════════════════════════════════════════════════════════════
// AMK_Hardware_Profiles.h - Universal Hardware Database for CYD & S3 & C5 Boards
//                           Extracted & verified from actual board schematics,
//                           OWLOS flight radar engine, and manufacturer pinouts.
// ═══════════════════════════════════════════════════════════════════════════════

#include <Arduino.h>

enum AMK_BoardId : uint8_t {
    AMK_BOARD_UNKNOWN = 0,
    // Classic ESP32 CYD Family (Resistive & Capacitive)
    AMK_BOARD_CYD_2432S028,       // ESP32-2432S028R 2.8" ILI9341/ST7789 (Own touch bus, Micro-USB)
    AMK_BOARD_CYD_2432S028C,      // ESP32-2432S028C 2.8" CST820 Capacitive
    AMK_BOARD_ESP32_32E_28,       // ESP32-32E 2.8" USB-C (Amp on GPIO4, Red LED on GPIO22)
    AMK_BOARD_ESP32_32E_32,       // ESP32-32E 3.2" ST7789 (Shared touch bus)
    AMK_BOARD_ESP32_32E_35_40,    // ESP32-32E 3.5" / 4.0" ST7796 320x480
    AMK_BOARD_ESP32_024,          // ESP32-024 2.4" ST7789/ILI9341
    AMK_BOARD_ESP32_035,          // ESP32-035 3.5" ST7796
    AMK_BOARD_ESP32_2432S032,     // ESP32-2432S032 3.2" ST7789 (Resistive XPT2046)
    AMK_BOARD_ESP32_2432S032C,    // ESP32-2432S032C 3.2" ST7789 (Capacitive GT911)
    AMK_BOARD_ESP32_2432S024C,    // ESP32-2432S024C 2.4" ILI9341 (Capacitive CST820)
    AMK_BOARD_GUITION_JC2432W328, // Guition JC2432W328 2.8" ST7789 (Capacitive CST820)
    AMK_BOARD_ESP32_3248S035C,    // ESP32-3248S035C 3.5" ST7796 (Capacitive GT911)

    // ESP32-S3 HMI / SPI & Capacitive Boards (New in v1.6.4)
    AMK_BOARD_ELECROW_ADVANCE_28, // Elecrow CrowPanel Advance 2.8" (ESP32-S3, ST7789, FT6336U)
    AMK_BOARD_LCDWIKI_ES3C28P,    // LCDWIKI ES3C28P 2.8" (ESP32-S3, ILI9341V, FT6336G, WS2812)
    AMK_BOARD_NM_CYD_C5,          // NM-CYD-C5 2.8" (ESP32-C5/S3 CYD form factor, ST7789, XPT2046)

    // ESP32-S3 Parallel RGB Boards (4.3", 5.0", 7.0")
    AMK_BOARD_WAVESHARE_S3_RGB,   // Waveshare ESP32-S3-Touch-LCD 4.3"/5.0"/7.0" (ST7262 RGB, GT911, CH422G)
    AMK_BOARD_SUNTON_S3_RGB_50,   // Sunton ESP32-8048S050 / Guition JC8048W550C 5.0" (ST7262 RGB, GT911, Direct GPIO BL 2)
    AMK_BOARD_ELECROW_ADVANCE_50, // Elecrow CrowPanel Advance 5.0" (ST7262 RGB, GT911, custom IO)

    AMK_BOARD_COUNT
};

struct AMK_BoardProfile {
    AMK_BoardId id;
    const char* modelName;       // Friendly model name
    const char* mcu;             // "ESP32", "ESP32-S3", "ESP32-C5"
    const char* panel;           // "ILI9341", "ST7789", "ST7796", "ST7262"
    uint16_t    nativeWidth;     // Native display width
    uint16_t    nativeHeight;    // Native display height
    float       diagonalInches;  // e.g. 2.4, 2.8, 3.2, 3.5, 4.0, 5.0, 7.0
    const char* touchChip;       // "XPT2046", "GT911", "CST820", "FT6336"
    bool        isCapacitive;    // true = I2C capacitive, false = SPI/bitbang resistive
    int8_t      pinSclk;         // Display SPI SCLK (or PCLK for RGB)
    int8_t      pinMosi;         // Display SPI MOSI (or DE for RGB)
    int8_t      pinMiso;         // Display SPI MISO (or VSYNC for RGB)
    int8_t      pinCs;           // Display CS (or HSYNC for RGB)
    int8_t      pinDc;           // Display DC
    int8_t      pinRst;          // Display RST (-1 if tied high or expander)
    int8_t      pinBacklight;    // Auto-detected backlight pin (or -1 if expander)
    int8_t      pinTouchCs;      // Resistive Touch CS (or I2C SDA for capacitive)
    int8_t      pinTouchIrq;     // Touch IRQ / INT (or I2C SCL for capacitive)
    int8_t      pinAuxLed;       // Onboard LED (or WS2812 pin)
    int8_t      pinAudioAmp;     // Audio amplifier enable pin (-1 if none)
    int8_t      pinLdr;          // LDR ambient light sensor pin (typically GPIO 34)
    const char* description;     // Hardware notes
};

// Static board profiles lookup table
static const AMK_BoardProfile s_amk_board_profiles[] = {
    // 0: Unknown
    {
        AMK_BOARD_UNKNOWN, "Unknown CYD / S3 Board", "ESP32", "Auto",
        240, 320, 2.8f, "Auto", false,
        14, 13, 12, 15, 2, -1, 21,
        33, 36, 4, -1, 34,
        "Unknown board configuration"
    },
    // 1: ESP32-2432S028R (CYD 2.8" micro-USB)
    {
        AMK_BOARD_CYD_2432S028, "ESP32-2432S028R (CYD 2.8\")", "ESP32", "ILI9341 / ST7789",
        240, 320, 2.8f, "XPT2046", false,
        14, 13, 12, 15, 2, -1, 21,
        33, 36, 4, -1, 34,
        "Original CYD 2.8in with dedicated touch bus, Red LED on GPIO4"
    },
    // 2: ESP32-2432S028C (CYD 2.8" capacitive)
    {
        AMK_BOARD_CYD_2432S028C, "ESP32-2432S028C 2.8\" Cap", "ESP32", "ILI9341 / ST7789",
        240, 320, 2.8f, "CST820", true,
        14, 13, 12, 15, 2, -1, 21,
        33, 32, 4, -1, 34,
        "CYD 2.8in with CST820 capacitive touch on I2C (SDA 33, SCL 32)"
    },
    // 3: ESP32-32E 2.8" (USB-C)
    {
        AMK_BOARD_ESP32_32E_28, "ESP32-32E 2.8\" (USB-C)", "ESP32", "ILI9341 / ST7789",
        240, 320, 2.8f, "XPT2046", false,
        14, 13, 12, 15, 2, -1, 21,
        33, 36, 22, 4, 34,
        "USB-C CYD 2.8in: Red LED on GPIO22, PAM8302 audio amp enable on GPIO4"
    },
    // 4: ESP32-32E 3.2"
    {
        AMK_BOARD_ESP32_32E_32, "ESP32-32E 3.2\" (USB-C)", "ESP32", "ST7789",
        240, 320, 3.2f, "XPT2046", false,
        14, 13, 12, 15, 2, -1, 27,
        33, 36, 22, 4, 34,
        "USB-C CYD 3.2in: Backlight GPIO27, touch on shared SPI bus"
    },
    // 5: ESP32-32E 3.5" / 4.0"
    {
        AMK_BOARD_ESP32_32E_35_40, "ESP32-32E 3.5\"/4.0\"", "ESP32", "ST7796",
        320, 480, 3.5f, "XPT2046", false,
        14, 13, 12, 15, 2, -1, 27,
        33, 36, 22, 4, 34,
        "USB-C CYD 3.5in/4.0in: 320x480 resolution, touch on shared SPI bus"
    },
    // 6: ESP32-024 2.4"
    {
        AMK_BOARD_ESP32_024, "ESP32-024 2.4\"", "ESP32", "ST7789 / ILI9341",
        240, 320, 2.4f, "XPT2046", false,
        14, 13, 12, 15, 2, -1, 27,
        33, 36, 4, -1, 34,
        "CYD 2.4in: Battery connector, Backlight GPIO27, touch on shared SPI bus"
    },
    // 7: ESP32-035 3.5"
    {
        AMK_BOARD_ESP32_035, "ESP32-035 3.5\"", "ESP32", "ST7796",
        320, 480, 3.5f, "XPT2046", false,
        14, 13, 12, 15, 2, -1, 27,
        33, 36, 4, -1, 34,
        "CYD 3.5in: Battery connector, Backlight GPIO27, ST7796 320x480"
    },
    // 8: ESP32-2432S032
    {
        AMK_BOARD_ESP32_2432S032, "ESP32-2432S032 3.2\"", "ESP32", "ST7789",
        240, 320, 3.2f, "XPT2046", false,
        14, 13, 12, 15, 2, -1, 27,
        33, 36, 4, -1, 34,
        "CYD 3.2in: ST7789 single-line readback, touch on shared SPI bus"
    },
    // 9: ESP32-2432S032C
    {
        AMK_BOARD_ESP32_2432S032C, "ESP32-2432S032C 3.2\" Cap", "ESP32", "ST7789",
        240, 320, 3.2f, "GT911", true,
        14, 13, 12, 15, 2, -1, 27,
        33, 32, 4, -1, 34,
        "CYD 3.2in with GT911 capacitive touch on I2C (SDA 33, SCL 32)"
    },
    // 10: ESP32-2432S024C
    {
        AMK_BOARD_ESP32_2432S024C, "ESP32-2432S024C 2.4\" Cap", "ESP32", "ILI9341",
        240, 320, 2.4f, "CST820", true,
        14, 13, 12, 15, 2, -1, 27,
        33, 32, 4, -1, 34,
        "CYD 2.4in with CST820 capacitive touch on I2C (SDA 33, SCL 32, INT 21)"
    },
    // 11: Guition JC2432W328
    {
        AMK_BOARD_GUITION_JC2432W328, "Guition JC2432W328 2.8\"", "ESP32", "ST7789",
        240, 320, 2.8f, "CST820", true,
        14, 13, 12, 15, 2, -1, 27,
        33, 32, 4, -1, 34,
        "Guition 2.8in with CST820 capacitive touch on I2C (SDA 33, SCL 32)"
    },
    // 12: ESP32-3248S035C
    {
        AMK_BOARD_ESP32_3248S035C, "ESP32-3248S035C 3.5\" Cap", "ESP32", "ST7796",
        320, 480, 3.5f, "GT911", true,
        14, 13, 12, 15, 2, -1, 27,
        33, 32, 4, -1, 34,
        "CYD 3.5in ST7796 320x480 with GT911 capacitive touch on I2C (SDA 33, SCL 32)"
    },
    // 13: Elecrow CrowPanel Advance 2.8" (ESP32-S3 HMI)
    {
        AMK_BOARD_ELECROW_ADVANCE_28, "Elecrow CrowPanel Advance 2.8\"", "ESP32-S3", "ST7789",
        240, 320, 2.8f, "FT6336U", true,
        42, 39, -1, 40, 41, -1, 38,
        15, 16, -1, -1, -1,
        "Elecrow CrowPanel Advance 2.8in: ESP32-S3, FT6336U capacitive touch (SDA 15, SCL 16, RST 48)"
    },
    // 14: LCDWIKI ES3C28P 2.8" (ESP32-S3)
    {
        AMK_BOARD_LCDWIKI_ES3C28P, "LCDWIKI ES3C28P 2.8\"", "ESP32-S3", "ILI9341V",
        240, 320, 2.8f, "FT6336G", true,
        12, 11, 13, 10, 46, -1, 45,
        16, 15, 42, -1, -1,
        "LCDWIKI ES3C28P 2.8in: ESP32-S3 N16R8, FT6336G capacitive touch, WS2812B RGB on GPIO42"
    },
    // 15: NM-CYD-C5 2.8" (ESP32-C5 CYD Form Factor)
    {
        AMK_BOARD_NM_CYD_C5, "NM-CYD-C5 2.8\"", "ESP32-C5", "ST7789",
        240, 320, 2.8f, "XPT2046", false,
        6, 7, 2, 23, 24, -1, 25,
        1, 3, 27, -1, -1,
        "NM-CYD-C5 2.8in: ESP32-C5 CYD form factor, shared SPI touch (CS 1, IRQ 3), WS2812 on GPIO27"
    },
    // 16: Waveshare ESP32-S3-Touch-LCD (4.3" / 5.0" / 7.0" RGB)
    {
        AMK_BOARD_WAVESHARE_S3_RGB, "Waveshare ESP32-S3-Touch-LCD 4.3\"/5.0\"/7.0\"", "ESP32-S3", "ST7262",
        800, 480, 7.0f, "GT911", true,
        7, 5, 3, 46, -1, -1, -1,
        8, 9, -1, -1, -1,
        "Waveshare S3 Parallel RGB 800x480: CH422G I2C Expander manages Backlight & LCD/Touch Reset"
    },
    // 17: Sunton ESP32-8048S050 / Guition JC8048W550C 5.0" RGB
    {
        AMK_BOARD_SUNTON_S3_RGB_50, "Sunton ESP32-8048S050 / Guition 5.0\"", "ESP32-S3", "ST7262",
        800, 480, 5.0f, "GT911", true,
        42, 40, 41, 39, -1, -1, 2,
        19, 20, -1, -1, -1,
        "Sunton / Guition 5.0in RGB: Direct GPIO Backlight on GPIO2, GT911 on I2C (SDA 19, SCL 20)"
    },
    // 18: Elecrow CrowPanel Advance 5.0" RGB
    {
        AMK_BOARD_ELECROW_ADVANCE_50, "Elecrow CrowPanel Advance 5.0\"", "ESP32-S3", "ST7262",
        800, 480, 5.0f, "GT911", true,
        39, 42, 41, 40, -1, -1, -1,
        15, 16, -1, -1, -1,
        "Elecrow CrowPanel Advance 5.0in: ST7262 Parallel RGB, GT911 on I2C (SDA 15, SCL 16)"
    }
};

static const size_t s_amk_board_profile_count = sizeof(s_amk_board_profiles) / sizeof(s_amk_board_profiles[0]);

inline const AMK_BoardProfile* AMK_getBoardProfile(AMK_BoardId id) {
    if ((size_t)id < s_amk_board_profile_count) {
        return &s_amk_board_profiles[(size_t)id];
    }
    return &s_amk_board_profiles[0];
}

inline const AMK_BoardProfile* AMK_findProfileByName(const char* name) {
    if (!name) return &s_amk_board_profiles[0];
    for (size_t i = 0; i < s_amk_board_profile_count; i++) {
        if (strstr(s_amk_board_profiles[i].modelName, name) != nullptr) {
            return &s_amk_board_profiles[i];
        }
    }
    return &s_amk_board_profiles[0];
}
