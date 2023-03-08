// Copyright 2022 The Pigweed Authors
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not
// use this file except in compliance with the License. You may obtain a copy of
// the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations under
// the License.

#include "pw_touchscreen/touchscreen.h"

#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>

#include <cinttypes>

#include "Adafruit_STMPE610.h"
#include "pw_log/log.h"
#include "pw_math/vector3.h"

namespace pw::touchscreen {
namespace {

constexpr int kTouchscreenMinX = 288;
constexpr int kTouchscreenMaxX = 3715;
constexpr int kTouchscreenMinY = 350;
constexpr int kTouchscreenMaxY = 3800;

constexpr int ScreenPixelWidth = 320;
constexpr int ScreenPixelHeight = 240;

// I2C Usage
Adafruit_STMPE610 touch_screen = Adafruit_STMPE610();

// I2C Pins;
//   SCL: Teensy Pin 19, stm32f429i-disc1 PA8
//   SDA: Teensy Pin 18, stm32f429i-disc1 PC9
//   INT: stm32f429i-disc1 PA15
//     Note: No interrupt support using the Adafruit_STMPE610 library.

// Hardware SPI Usage:
//   Adafruit_STMPE610 touch_screen Adafruit_STMPE610(uint8_t cs);
// Software SPI Usage:
//   Adafruit_STMPE610 touch_screen Adafruit_STMPE610(uint8_t cspin,
//                                                    uint8_t mosipin,
//                                                    uint8_t misopin,
//                                                    uint8_t clkpin);

}  // namespace

void Init() { touch_screen.begin(); }

bool Available() { return true; }

bool NewTouchEvent() { return touch_screen.touched(); }

pw::math::Vector3<int> GetTouchPoint() {
  pw::math::Vector3<int> point;
  uint16_t x, y;
  uint8_t z;
  touch_screen.readData(&x, &y, &z);
  point.x = map(x, kTouchscreenMinX, kTouchscreenMaxX, 0, ScreenPixelWidth);
  point.y = map(y, kTouchscreenMinY, kTouchscreenMaxY, 0, ScreenPixelHeight);
  point.z = z;
  PW_LOG_DEBUG("Touch: x:%d, y:%d, z:%d, x:%d, y:%d, z:%d",
               (int)x,
               (int)y,
               (int)z,
               point.x,
               point.y,
               point.z);
  return point;
}

}  // namespace pw::touchscreen
