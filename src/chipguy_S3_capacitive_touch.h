/*
 * chipguy_ESP32S3_Touch_LCD_7_touch - GT911 capacitive touch driver for the
 * Waveshare ESP32-S3-Touch-LCD-7 development board.
 *
 * This file is a derivative of the TAMC_GT911 Arduino library:
 *   https://github.com/tamctec/gt911-arduino
 *   Copyright (c) TAMC <tamctec@gmail.com>
 *   Licensed under the Apache License, Version 2.0.
 *
 * Modifications by chipguyhere: adapted for the ESP32-S3-Touch-LCD-7 board
 * (fixed I2C pins and 800x480 resolution; reset sequence via the CH422G I2C
 * expander IO1 plus the INT GPIO, which this board does wire). These
 * modifications are likewise provided under the Apache License, Version 2.0.
 *
 * You may obtain a copy of the License in the file LICENSE-Apache-2.0.txt
 * included with this library, or at:
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations
 * under the License.
 */

#ifndef CHIPGUY_S3_CAPACITIVE_TOUCH_H
#define CHIPGUY_S3_CAPACITIVE_TOUCH_H

#include "Arduino.h"
#include <Wire.h>
#include "AMK_S3_RGB_Board.h"   // shared I2C bus + expander

#define GT911_ADDR1 (uint8_t)0x5D
#define GT911_ADDR2 (uint8_t)0x14

#define ROTATION_LEFT      (uint8_t)0
#define ROTATION_INVERTED  (uint8_t)1
#define ROTATION_RIGHT     (uint8_t)2
#define ROTATION_NORMAL    (uint8_t)3

// Real-time command (Write only)
#define GT911_COMMAND       (uint16_t)0x8040
#define GT911_ESD_CHECK     (uint16_t)0x8041
#define GT911_COMMAND_CHECK (uint16_t)0x8046

// Configuration information (R/W)
#define GT911_CONFIG_START             (uint16_t)0x8047
#define GT911_CONFIG_VERSION           (uint16_t)0x8047
#define GT911_X_OUTPUT_MAX_LOW         (uint16_t)0x8048
#define GT911_X_OUTPUT_MAX_HIGH        (uint16_t)0x8049
#define GT911_Y_OUTPUT_MAX_LOW         (uint16_t)0x804A
#define GT911_Y_OUTPUT_MAX_HIGH        (uint16_t)0x804B
#define GT911_TOUCH_NUMBER             (uint16_t)0x804C
#define GT911_MODULE_SWITCH_1          (uint16_t)0x804D
#define GT911_MODULE_SWITCH_2          (uint16_t)0x804E
#define GT911_SHAKE_COUNT              (uint16_t)0x804F
#define GT911_FILTER                   (uint16_t)0x8050
#define GT911_LARGE_TOUCH              (uint16_t)0x8051
#define GT911_NOISE_REDUCTION          (uint16_t)0x8052
#define GT911_SCREEN_TOUCH_LEVEL       (uint16_t)0x8053
#define GT911_SCREEN_RELEASE_LEVEL     (uint16_t)0x8054
#define GT911_LOW_POWER_CONTROL        (uint16_t)0x8055
#define GT911_REFRESH_RATE             (uint16_t)0x8056
#define GT911_X_THRESHOLD              (uint16_t)0x8057
#define GT911_Y_THRESHOLD              (uint16_t)0x8058
#define GT911_X_SPEED_LIMIT            (uint16_t)0x8059
#define GT911_Y_SPEED_LIMIT            (uint16_t)0x805A
#define GT911_SPACE_TOP_BOTTOM         (uint16_t)0x805B
#define GT911_SPACE_LEFT_RIGHT         (uint16_t)0x805C
#define GT911_MINI_FILTER              (uint16_t)0x805D
#define GT911_STRETCH_R0               (uint16_t)0x805E
#define GT911_STRETCH_R1               (uint16_t)0x805F
#define GT911_STRETCH_R2               (uint16_t)0x8060
#define GT911_STRETCH_RM               (uint16_t)0x8061
#define GT911_DRV_GROUPA_NUM           (uint16_t)0x8062
#define GT911_DRV_GROUPB_NUM           (uint16_t)0x8063
#define GT911_SENSOR_NUM               (uint16_t)0x8064
#define GT911_FREQ_A_FACTOR            (uint16_t)0x8065
#define GT911_FREQ_B_FACTOR            (uint16_t)0x8066
#define GT911_PANEL_BIT_FREQ_L         (uint16_t)0x8067
#define GT911_PANEL_BIT_FREQ_H         (uint16_t)0x8068
#define GT911_PANEL_SENSOR_TIME_L      (uint16_t)0x8069
#define GT911_PANEL_SENSOR_TIME_H      (uint16_t)0x806A
#define GT911_PANEL_TX_GAIN            (uint16_t)0x806B
#define GT911_PANEL_RX_GAIN            (uint16_t)0x806C
#define GT911_PANEL_DUMP_SHIFT         (uint16_t)0x806D
#define GT911_DRV_FRAME_CONTROL        (uint16_t)0x806E
#define GT911_CHARGING_LEVEL_UP        (uint16_t)0x806F
#define GT911_MODULE_SWITCH3           (uint16_t)0x8070
#define GT911_GESTURE_DIS              (uint16_t)0X8071
#define GT911_GESTURE_LONG_PRESS_TIME  (uint16_t)0x8072
#define GT911_X_Y_SLOPE_ADJUST         (uint16_t)0X8073
#define GT911_GESTURE_CONTROL          (uint16_t)0X8074
#define GT911_GESTURE_SWITCH1          (uint16_t)0X8075
#define GT911_GESTURE_SWITCH2          (uint16_t)0X8076
#define GT911_GESTURE_REFRESH_RATE     (uint16_t)0x8077
#define GT911_GESTURE_TOUCH_LEVEL      (uint16_t)0x8078
#define GT911_NEWGREENWAKEUPLEVEL      (uint16_t)0x8079
#define GT911_FREQ_HOPPING_START       (uint16_t)0x807A
#define GT911_FREQ_HOPPING_END         (uint16_t)0X807B
#define GT911_NOISE_DETECT_TIMES       (uint16_t)0x807C
#define GT911_HOPPING_FLAG             (uint16_t)0X807D
#define GT911_HOPPING_THRESHOLD        (uint16_t)0X807E
#define GT911_NOISE_THRESHOLD          (uint16_t)0X807F
#define GT911_NOISE_MIN_THRESHOLD      (uint16_t)0X8080
#define GT911_HOPPING_SENSOR_GROUP     (uint16_t)0X8082
#define GT911_HOPPING_SEG1_NORMALIZE   (uint16_t)0X8083
#define GT911_HOPPING_SEG1_FACTOR      (uint16_t)0X8084
#define GT911_MAIN_CLOCK_AJDUST        (uint16_t)0X8085
#define GT911_HOPPING_SEG2_NORMALIZE   (uint16_t)0X8086
#define GT911_HOPPING_SEG2_FACTOR      (uint16_t)0X8087
#define GT911_HOPPING_SEG3_NORMALIZE   (uint16_t)0X8089
#define GT911_HOPPING_SEG3_FACTOR      (uint16_t)0X808A
#define GT911_HOPPING_SEG4_NORMALIZE   (uint16_t)0X808C
#define GT911_HOPPING_SEG4_FACTOR      (uint16_t)0X808D
#define GT911_HOPPING_SEG5_NORMALIZE   (uint16_t)0X808F
#define GT911_HOPPING_SEG5_FACTOR      (uint16_t)0X8090
#define GT911_HOPPING_SEG6_NORMALIZE   (uint16_t)0X8092
#define GT911_KEY_1                    (uint16_t)0X8093
#define GT911_KEY_2                    (uint16_t)0X8094
#define GT911_KEY_3                    (uint16_t)0X8095
#define GT911_KEY_4                    (uint16_t)0X8096
#define GT911_KEY_AREA                 (uint16_t)0X8097
#define GT911_KEY_TOUCH_LEVEL          (uint16_t)0X8098
#define GT911_KEY_LEAVE_LEVEL          (uint16_t)0X8099
#define GT911_KEY_SENS_1_2             (uint16_t)0X809A
#define GT911_KEY_SENS_3_4             (uint16_t)0X809B
#define GT911_KEY_RESTRAIN             (uint16_t)0X809C
#define GT911_KEY_RESTRAIN_TIME        (uint16_t)0X809D
#define GT911_GESTURE_LARGE_TOUCH      (uint16_t)0X809E
#define GT911_HOTKNOT_NOISE_MAP        (uint16_t)0X80A1
#define GT911_LINK_THRESHOLD           (uint16_t)0X80A2
#define GT911_PXY_THRESHOLD            (uint16_t)0X80A3
#define GT911_GHOT_DUMP_SHIFT          (uint16_t)0X80A4
#define GT911_GHOT_RX_GAIN             (uint16_t)0X80A5
#define GT911_FREQ_GAIN0               (uint16_t)0X80A6
#define GT911_FREQ_GAIN1               (uint16_t)0X80A7
#define GT911_FREQ_GAIN2               (uint16_t)0X80A8
#define GT911_FREQ_GAIN3               (uint16_t)0X80A9
#define GT911_COMBINE_DIS              (uint16_t)0X80B3
#define GT911_SPLIT_SET                (uint16_t)0X80B4
#define GT911_SENSOR_CH0               (uint16_t)0X80B7
#define GT911_DRIVER_CH0               (uint16_t)0X80D5
#define GT911_CONFIG_CHKSUM            (uint16_t)0X80FF
#define GT911_CONFIG_FRESH             (uint16_t)0X8100
#define GT911_CONFIG_SIZE              (uint16_t)0xFF-0x46

// Coordinate information
#define GT911_PRODUCT_ID        (uint16_t)0X8140
#define GT911_FIRMWARE_VERSION  (uint16_t)0X8140
#define GT911_RESOLUTION        (uint16_t)0X8140
#define GT911_VENDOR_ID         (uint16_t)0X8140
#define GT911_IMFORMATION       (uint16_t)0X8140
#define GT911_POINT_INFO        (uint16_t)0X814E
#define GT911_POINT_1           (uint16_t)0X814F
#define GT911_POINT_2           (uint16_t)0X8157
#define GT911_POINT_3           (uint16_t)0X815F
#define GT911_POINT_4           (uint16_t)0X8167
#define GT911_POINT_5           (uint16_t)0X816F

class TP_Point {
  public:
    TP_Point(void);
    TP_Point(uint8_t id, uint16_t x, uint16_t y, uint16_t size);

    bool operator==(TP_Point);
    bool operator!=(TP_Point);

    uint8_t id;
    uint16_t x;
    uint16_t y;
    uint8_t size;
};

class chipguy_S3_capacitive_touch {
  public:
    // Takes the board that owns the shared I2C bus and the CH422G expander;
    // the GT911 talks over board.Wire and is reset through it.
    chipguy_S3_capacitive_touch(AMK_S3_Board& board = amk_get_default_board());
    void begin(uint8_t _addr=GT911_ADDR1);
    template<typename T>
    void begin(T& disp, uint8_t _addr=GT911_ADDR1) {
        begin(_addr);
        setRotation(disp.rotation());
    }
    void reset();
    void setRotation(uint8_t rot);
    void read(void);

    // Universal compatibility helpers
    bool read(uint16_t &x, uint16_t &y) {
        this->read();
        if (this->isTouched && this->touches > 0) {
            x = this->points[0].x;
            y = this->points[0].y;
            return true;
        }
        return false;
    }
    bool readRaw(uint16_t &x, uint16_t &y) {
        return read(x, y);
    }
    uint16_t pressure() {
        return this->isTouched ? 1000 : 0;
    }
    void setCalibration(uint16_t, uint16_t, uint16_t, uint16_t) {}
    void getCalibration(uint16_t &xMin, uint16_t &xMax, uint16_t &yMin, uint16_t &yMax) const {
        xMin = 0; xMax = width; yMin = 0; yMax = height;
    }
    bool loadCalibration() { return true; }
    bool saveCalibration() { return true; }
    bool loadFilters() { return false; }
    void saveFilters() {}
    void setFilters(uint16_t /*minPressure*/, uint16_t /*maxJump*/) {}
    void getFilters(uint16_t &minPressure, uint16_t &maxJump) const { minPressure = 0; maxJump = 0; }
    int16_t nativeWidth() const { return width; }
    int16_t nativeHeight() const { return height; }
    bool mirrorsX() const { return false; }
    bool mirrorsY() const { return false; }
    uint8_t isLargeDetect;
    uint8_t touches = 0;
    bool isTouched = false;
    TP_Point points[5];

  private:
    TP_Point readPoint(uint8_t *data);
    void writeByteData(uint16_t reg, uint8_t val);
    uint8_t readByteData(uint16_t reg);
    void writeBlockData(uint16_t reg, uint8_t *val, uint8_t size);
    void readBlockData(uint8_t *buf, uint16_t reg, uint8_t size);

    // The board owns the shared I2C bus (board.Wire), the CH422G expander, and
    // the GT911 INT pin used for reset/address selection.
    AMK_S3_Board& _board;

    // The GT911 on this board reports coordinates already aligned with the
    // panel (matching Waveshare's demo), so the default is a pass-through.
    uint8_t rotation = ROTATION_INVERTED;
    uint8_t addr;
    static const uint16_t width = 800;
    static const uint16_t height = 480;
};


// Hardware and backward-compatibility aliases
typedef chipguy_S3_capacitive_touch ESP32S3_Touch_LCD_7_Touch;
typedef chipguy_S3_capacitive_touch chipguy_ESP32S3_Touch_LCD_7_touch;
typedef chipguy_S3_capacitive_touch AMK_S3_touch;
typedef chipguy_S3_capacitive_touch AMK_S3_Capacitive_touch;
typedef chipguy_S3_capacitive_touch ESP32S3_Touch;

#endif // CHIPGUY_S3_CAPACITIVE_TOUCH_H
