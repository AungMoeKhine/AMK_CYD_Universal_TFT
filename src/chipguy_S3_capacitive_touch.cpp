/*
 * chipguy_ESP32S3_Touch_LCD_7_touch - GT911 capacitive touch driver for the
 * Waveshare ESP32-S3-Touch-LCD-7 development board.
 *
 * This file is a derivative of the TAMC_GT911 Arduino library:
 *   https://github.com/tamctec/gt911-arduino
 *   Copyright (c) TAMC <tamctec@gmail.com>
 *   Licensed under the Apache License, Version 2.0.
 *
 * Modified by chipguyhere for the ESP32-S3-Touch-LCD-7 board (fixed I2C pins
 * and 800x480 resolution; reset via the CH422G expander IO1 plus the INT GPIO).
 * These modifications are likewise provided under the Apache License,
 * Version 2.0. See LICENSE-Apache-2.0.txt or
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#include "Arduino.h"

#if defined(CONFIG_IDF_TARGET_ESP32S3) || defined(BOARD_ESP32_S3_RGB) || defined(BOARD_ESP32_S3_TOUCH_7) || defined(ARDUINO_WAVESHARE_ESP32_S3_TOUCH_LCD_7)

#include "chipguy_S3_capacitive_touch.h"

chipguy_S3_capacitive_touch::chipguy_S3_capacitive_touch(AMK_S3_Board& board)
  : _board(board) {
}

void chipguy_S3_capacitive_touch::begin(uint8_t _addr) {
  addr = _addr;
  // Bring up the board's shared I2C bus + expander (idempotent; display.begin()
  // may already have done it), then reset the GT911 through the board.
  _board.begin();
  reset();
  delay(50);
}

void chipguy_S3_capacitive_touch::reset() {
  // The board owns the GT911's reset line (IO1) and INT pin, so it performs
  // the power-on reset (which also latches the 0x5D I2C address).
  _board.touchReset();
}

void chipguy_S3_capacitive_touch::setRotation(uint8_t rot) {
  switch (rot % 4) {
    case 0:
      rotation = ROTATION_INVERTED; // Native 800x480 landscape pass-through (x=x, y=y)
      break;
    case 1:
      rotation = ROTATION_RIGHT;    // 90 deg clockwise
      break;
    case 2:
      rotation = ROTATION_NORMAL;   // 180 deg flipped (x = width - x, y = height - y)
      break;
    case 3:
      rotation = ROTATION_LEFT;     // 270 deg
      break;
  }
}

void chipguy_S3_capacitive_touch::read(void) {
  uint8_t data[7];

  uint8_t pointInfo = readByteData(GT911_POINT_INFO);
  uint8_t bufferStatus = pointInfo >> 7 & 1;
  isLargeDetect = pointInfo >> 6 & 1;
  touches = pointInfo & 0xF;
  isTouched = touches > 0;

  if (bufferStatus == 1 && isTouched) {
    for (uint8_t i = 0; i < touches; i++) {
      readBlockData(data, GT911_POINT_1 + i * 8, 7);
      points[i] = readPoint(data);
    }
  }
  writeByteData(GT911_POINT_INFO, 0);
}

TP_Point chipguy_S3_capacitive_touch::readPoint(uint8_t *data) {
  uint16_t temp;
  uint8_t id = data[0];
  uint16_t x = data[1] + (data[2] << 8);
  uint16_t y = data[3] + (data[4] << 8);
  uint16_t size = data[5] + (data[6] << 8);

  switch (rotation) {
    case ROTATION_NORMAL:
      x = width - x;
      y = height - y;
      break;
    case ROTATION_LEFT:
      temp = x;
      x = width - y;
      y = temp;
      break;
    case ROTATION_INVERTED:
      // x = x; y = y;
      break;
    case ROTATION_RIGHT:
      temp = x;
      x = y;
      y = height - temp;
      break;
    default:
      break;
  }
  return TP_Point(id, x, y, size);
}

// The GT911 uses a 16-bit register address, so these do their own
// beginTransmission/write/endTransmission/requestFrom sequences rather than the
// 8-bit-register convenience helpers.  Each holds the shared bus with a Guard so
// the multi-step transaction can't interleave with another task's bus traffic.

void chipguy_S3_capacitive_touch::writeByteData(uint16_t reg, uint8_t val) {
  ThreadsafeWire::Guard g(_board.Wire);
  _board.Wire.beginTransmission(addr);
  _board.Wire.write(highByte(reg));
  _board.Wire.write(lowByte(reg));
  _board.Wire.write(val);
  _board.Wire.endTransmission();
}

uint8_t chipguy_S3_capacitive_touch::readByteData(uint16_t reg) {
  ThreadsafeWire::Guard g(_board.Wire);
  uint8_t x;
  _board.Wire.beginTransmission(addr);
  _board.Wire.write(highByte(reg));
  _board.Wire.write(lowByte(reg));
  _board.Wire.endTransmission();
  _board.Wire.requestFrom(addr, (size_t)1);
  x = _board.Wire.read();
  return x;
}

void chipguy_S3_capacitive_touch::writeBlockData(uint16_t reg, uint8_t *val, uint8_t size) {
  ThreadsafeWire::Guard g(_board.Wire);
  _board.Wire.beginTransmission(addr);
  _board.Wire.write(highByte(reg));
  _board.Wire.write(lowByte(reg));
  for (uint8_t i = 0; i < size; i++) {
    _board.Wire.write(val[i]);
  }
  _board.Wire.endTransmission();
}

void chipguy_S3_capacitive_touch::readBlockData(uint8_t *buf, uint16_t reg, uint8_t size) {
  ThreadsafeWire::Guard g(_board.Wire);
  _board.Wire.beginTransmission(addr);
  _board.Wire.write(highByte(reg));
  _board.Wire.write(lowByte(reg));
  _board.Wire.endTransmission();
  _board.Wire.requestFrom(addr, (size_t)size);
  for (uint8_t i = 0; i < size; i++) {
    buf[i] = _board.Wire.read();
  }
}

TP_Point::TP_Point(void) {
  id = x = y = size = 0;
}

TP_Point::TP_Point(uint8_t _id, uint16_t _x, uint16_t _y, uint16_t _size) {
  id = _id;
  x = _x;
  y = _y;
  size = _size;
}

bool TP_Point::operator==(TP_Point point) {
  return ((point.x == x) && (point.y == y) && (point.size == size));
}

bool TP_Point::operator!=(TP_Point point) {
  return ((point.x != x) || (point.y != y) || (point.size != size));
}

#endif // CONFIG_IDF_TARGET_ESP32S3
