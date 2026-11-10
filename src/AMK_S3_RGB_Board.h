#pragma once
// ═══════════════════════════════════════════════════════════════════════════════
// AMK_S3_RGB_Board.h - Hardware Board Controller for ESP32-S3 RGB Display Boards
//                      Designed for Waveshare ESP32-S3-Touch-LCD 4.3", 5.0", 7.0"
//                      (ST7262 Parallel RGB + GT911 Capacitive Touch with CH422G IO Expander)
//                      and Sunton ESP32-8048Sxxx / Guition JC8048W550
//                      (ST7262 Parallel RGB + GT911 Capacitive Touch with Direct GPIO)
//
// Features:
//   - Dynamic runtime auto-detection of Waveshare CH422G expander vs Sunton Direct GPIO
//   - Automatic I2C bus reconfiguration (SDA 8/SCL 9 for Waveshare vs SDA 19/SCL 20 for Sunton)
//   - Backlight PWM on Direct GPIO 2 (Sunton) or CH422G IO2 (Waveshare)
//   - Capacitive touch reset sequence handling for both Expander IO1 and Direct GPIO 38
//   - Threadsafe Wire bus with FreeRTOS mutex guard
//   - Self-contained: No external board library dependency required!
// ═══════════════════════════════════════════════════════════════════════════════

#include <Arduino.h>
#include <Wire.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

// CH422G I2C Expander Register Addresses
#define CH422G_I2C_ADDR_CFG  0x24  // write enable register
#define CH422G_I2C_ADDR_OUT  0x38  // output data register

// CH422G Output Pin Mappings on ESP32-S3 RGB Boards
#define CH422G_PIN_TP_RST    1     // Touch Reset (active low)
#define CH422G_PIN_LCD_BL    2     // LCD Backlight (HIGH = ON)
#define CH422G_PIN_LCD_RST   3     // LCD Reset (active low)
#define CH422G_PIN_SD_CS     4     // SD Card CS (active low)
#define CH422G_PIN_USB_SEL   5     // USB/CAN Mux (LOW = USB)
#define CH422G_PIN_LCD_PWR   6     // LCD VDD Power (HIGH = ON)

// Default I2C pins for CH422G and GT911 on Waveshare ESP32-S3 RGB boards
#ifndef WS_S3_I2C_SDA
#define WS_S3_I2C_SDA        8
#endif
#ifndef WS_S3_I2C_SCL
#define WS_S3_I2C_SCL        9
#endif
#ifndef WS_S3_TP_INT
#define WS_S3_TP_INT         4
#endif

// Default I2C pins for GT911 on Sunton ESP32-8048Sxxx RGB boards
#ifndef SUNTON_S3_I2C_SDA
#define SUNTON_S3_I2C_SDA    19
#endif
#ifndef SUNTON_S3_I2C_SCL
#define SUNTON_S3_I2C_SCL    20
#endif
#ifndef SUNTON_S3_TP_INT
#define SUNTON_S3_TP_INT     18
#endif
#ifndef SUNTON_S3_TP_RST
#define SUNTON_S3_TP_RST     38
#endif
#ifndef SUNTON_S3_LCD_BL
#define SUNTON_S3_LCD_BL     2
#endif

// Thread-safe Wire RAII Mutex Guard for multi-task / interrupt safety
class ThreadsafeWire {
public:
    class Guard {
    private:
        SemaphoreHandle_t _mutex;
    public:
        Guard(TwoWire& /*wire*/) {
            _mutex = ThreadsafeWire::getMutex();
            if (_mutex) {
                xSemaphoreTake(_mutex, portMAX_DELAY);
            }
        }
        ~Guard() {
            if (_mutex) {
                xSemaphoreGive(_mutex);
            }
        }
    };

    static SemaphoreHandle_t getMutex() {
        static SemaphoreHandle_t s_mutex = nullptr;
        if (!s_mutex) {
            s_mutex = xSemaphoreCreateMutex();
        }
        return s_mutex;
    }
};

class AMK_S3_Board {
public:
    TwoWire& Wire;
    uint8_t  shadow = 0;
    bool     initialized = false;

    AMK_S3_Board(TwoWire& wire = ::Wire) : Wire(wire) {}

    bool   hasExpander   = true;
    bool   isSunton      = false;
    int8_t backlightGpio = -1;
    int8_t touchIntPin   = WS_S3_TP_INT;     // GPIO 4 (Waveshare) or GPIO 18 (Sunton)
    int8_t touchRstPin   = -1;               // GPIO 38 (Sunton) or Expander IO1 (Waveshare)
    int8_t i2cSdaPin     = WS_S3_I2C_SDA;    // GPIO 8 (Waveshare) or GPIO 19 (Sunton)
    int8_t i2cSclPin     = WS_S3_I2C_SCL;    // GPIO 9 (Waveshare) or GPIO 20 (Sunton)

    void begin(int sda = WS_S3_I2C_SDA, int scl = WS_S3_I2C_SCL, uint32_t freq = 400000) {
        if (initialized) return;

        // Step 1: Probe default or specified pins for CH422G IO Expander (Waveshare)
        Wire.begin(sda, scl, freq);
        Wire.beginTransmission(CH422G_I2C_ADDR_CFG);
        hasExpander = (Wire.endTransmission() == 0);

        if (hasExpander) {
            // Waveshare ESP32-S3-Touch-LCD detected (CH422G ACKed on 0x24)
            isSunton = false;
            backlightGpio = -1;
            touchIntPin = WS_S3_TP_INT;
            touchRstPin = -1;
            i2cSdaPin = sda;
            i2cSclPin = scl;

            // Initial shadow state: LCD power ON, Backlight ON, Touch normal, LCD normal, SD CS deselected
            shadow = (1 << CH422G_PIN_LCD_PWR) |
                     (1 << CH422G_PIN_LCD_BL)  |
                     (1 << CH422G_PIN_TP_RST)  |
                     (1 << CH422G_PIN_LCD_RST) |
                     (1 << CH422G_PIN_SD_CS);
            writeShadow();
        } else {
            // Direct GPIO board fallback (Sunton ESP32-8048Sxxx / Guition JC8048W550)
            // Switch I2C to Sunton standard pins (SDA=19, SCL=20) if default Waveshare pins were used
            if (sda == WS_S3_I2C_SDA && scl == WS_S3_I2C_SCL) {
                Wire.end();
                Wire.begin(SUNTON_S3_I2C_SDA, SUNTON_S3_I2C_SCL, freq);
                i2cSdaPin = SUNTON_S3_I2C_SDA;
                i2cSclPin = SUNTON_S3_I2C_SCL;
            } else {
                i2cSdaPin = sda;
                i2cSclPin = scl;
            }

            isSunton = true;
            backlightGpio = SUNTON_S3_LCD_BL; // Direct GPIO 2 Backlight
            touchIntPin = SUNTON_S3_TP_INT;   // Direct GPIO 18 Touch INT
            touchRstPin = SUNTON_S3_TP_RST;   // Direct GPIO 38 Touch RST

            pinMode(backlightGpio, OUTPUT);
            digitalWrite(backlightGpio, HIGH);
        }
        initialized = true;
    }

    void writePin(uint8_t pin, bool state) {
        if (!hasExpander) return;
        if (state) {
            shadow |= (1 << pin);
        } else {
            shadow &= ~(1 << pin);
        }
        writeShadow();
    }

    void writeShadow() {
        if (!hasExpander) return;
        ThreadsafeWire::Guard g(Wire);
        Wire.beginTransmission(CH422G_I2C_ADDR_CFG);
        Wire.write(0x01);
        Wire.endTransmission();

        Wire.beginTransmission(CH422G_I2C_ADDR_OUT);
        Wire.write(shadow);
        Wire.endTransmission();
    }

    void lcdReset() {
        if (hasExpander) {
            writePin(CH422G_PIN_LCD_RST, false);
            delay(20);
            writePin(CH422G_PIN_LCD_RST, true);
            delay(50);
        }
    }

    void lcdBacklight(bool on) {
        if (hasExpander) {
            writePin(CH422G_PIN_LCD_BL, on);
        } else if (backlightGpio >= 0) {
            pinMode(backlightGpio, OUTPUT);
            digitalWrite(backlightGpio, on ? HIGH : LOW);
        }
    }

    void setBacklight(uint8_t level) {
        if (hasExpander) {
            writePin(CH422G_PIN_LCD_BL, level > 0);
        } else if (backlightGpio >= 0) {
            pinMode(backlightGpio, OUTPUT);
            analogWrite(backlightGpio, level);
        }
    }

    int8_t backlightPin() const {
        return backlightGpio;
    }

    void touchReset(int intPin = -1) {
        int pin = (intPin >= 0) ? intPin : touchIntPin;
        if (hasExpander) {
            pinMode(pin, OUTPUT);
            digitalWrite(pin, LOW);
            delay(10);
            writePin(CH422G_PIN_TP_RST, false);
            delay(20);
            writePin(CH422G_PIN_TP_RST, true);
            delay(15);
            pinMode(pin, INPUT);
            delay(50);
        } else {
            // Direct GPIO Touch Reset (Sunton ESP32-8048Sxxx / Guition JC8048W550)
            if (pin >= 0) {
                pinMode(pin, OUTPUT);
                digitalWrite(pin, LOW);
            }
            if (touchRstPin >= 0) {
                pinMode(touchRstPin, OUTPUT);
                digitalWrite(touchRstPin, LOW);
                delay(20);
                digitalWrite(touchRstPin, HIGH);
                delay(15);
            }
            if (pin >= 0) {
                pinMode(pin, INPUT);
            }
            delay(50);
        }
    }
};

// Aliases for generalized board naming
using ESP32S3_Touch_LCD_7_Board = AMK_S3_Board;
using ESP32S3_RGB_Board         = AMK_S3_Board;

#ifndef CYDR_PANEL_AUTO
enum cydr_panel_t : uint8_t {
    CYDR_PANEL_AUTO   = 0,
    CYDR_PANEL_ILI9341 = 1,
    CYDR_PANEL_ST7789  = 2,
    CYDR_PANEL_ST7796  = 3,
    CYDR_PANEL_ST7262  = 4
};
#endif

#ifndef CYDR_BOARD_UNKNOWN
enum cydr_board_t : uint8_t {
    CYDR_BOARD_UNKNOWN       = 0,
    CYDR_BOARD_OWN_TOUCH     = 1,
    CYDR_BOARD_SHARED_TOUCH  = 2,
    CYDR_BOARD_S3_RGB        = 3
};
#endif

// Global default board accessor
AMK_S3_Board& amk_get_default_board();
