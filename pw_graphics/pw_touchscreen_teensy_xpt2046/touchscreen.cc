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

#include <cinttypes>

#include "XPT2046_Touchscreen.h"
#include "pw_coordinates/vector3.h"

namespace pw::touchscreen {
namespace {

constexpr int TS_CS = 7;
constexpr int kTouchscreenMinX = 288;
constexpr int kTouchscreenMaxX = 3715;
constexpr int kTouchscreenMinY = 350;
constexpr int kTouchscreenMaxY = 3800;

constexpr int ScreenPixelWidth = 320;
constexpr int ScreenPixelHeight = 240;

XPT2046_Touchscreen touch_screen(TS_CS);

}  // namespace

void Init() { touch_screen.begin(); }

bool Available() { return true; }

bool NewTouchEvent() { return touch_screen.touched(); }

pw::coordinates::Vector3<int> GetTouchPoint() {
  pw::coordinates::Vector3<int> point;
  TS_Point p = touch_screen.getPoint();
  point.x = map(p.x, kTouchscreenMinX, kTouchscreenMaxX, 0, ScreenPixelWidth);
  point.y = map(p.y, kTouchscreenMinY, kTouchscreenMaxY, 0, ScreenPixelHeight);
  point.z = p.z;
  return point;
}

}  // namespace pw::touchscreen
