#pragma once
// ═══════════════════════════════════════════════════════════════════════════════
// AMK_S3_RGB_Board.h - Hardware Board Controller for ESP32-S3 RGB Display Boards
//                      Supports Waveshare & Sunton 4.3", 5.0", 7.0" RGB panels
//
// Features:
//   - Onboard CH422G IO Expander management (LCD Reset, Backlight, Touch Reset, Power)
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

// Default I2C pins for CH422G and GT911 on ESP32-S3 RGB boards
#ifndef WS_S3_I2C_SDA
#define WS_S3_I2C_SDA        8
#endif
#ifndef WS_S3_I2C_SCL
#define WS_S3_I2C_SCL        9
#endif
#ifndef WS_S3_TP_INT
#define WS_S3_TP_INT         4
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

    void begin(int sda = WS_S3_I2C_SDA, int scl = WS_S3_I2C_SCL, uint32_t freq = 400000) {
        if (initialized) return;
        Wire.begin(sda, scl, freq);

        // Initial shadow state: LCD power ON, Backlight ON, Touch normal, LCD normal, SD CS deselected
        shadow = (1 << CH422G_PIN_LCD_PWR) |
                 (1 << CH422G_PIN_LCD_BL)  |
                 (1 << CH422G_PIN_TP_RST)  |
                 (1 << CH422G_PIN_LCD_RST) |
                 (1 << CH422G_PIN_SD_CS);
        writeShadow();
        initialized = true;
    }

    void writePin(uint8_t pin, bool state) {
        if (state) {
            shadow |= (1 << pin);
        } else {
            shadow &= ~(1 << pin);
        }
        writeShadow();
    }

    void writeShadow() {
        ThreadsafeWire::Guard g(Wire);
        Wire.beginTransmission(CH422G_I2C_ADDR_CFG);
        Wire.write(0x01);
        Wire.endTransmission();

        Wire.beginTransmission(CH422G_I2C_ADDR_OUT);
        Wire.write(shadow);
        Wire.endTransmission();
    }

    void lcdReset() {
        writePin(CH422G_PIN_LCD_RST, false);
        delay(20);
        writePin(CH422G_PIN_LCD_RST, true);
        delay(50);
    }

    void lcdBacklight(bool on) {
        writePin(CH422G_PIN_LCD_BL, on);
    }

    void touchReset(int intPin = WS_S3_TP_INT) {
        pinMode(intPin, OUTPUT);
        digitalWrite(intPin, LOW);
        delay(10);
        writePin(CH422G_PIN_TP_RST, false);
        delay(20);
        writePin(CH422G_PIN_TP_RST, true);
        delay(15);
        pinMode(intPin, INPUT);
        delay(50);
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
