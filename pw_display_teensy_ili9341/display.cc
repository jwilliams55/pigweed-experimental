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

#include "pw_display/display.h"

#include <Arduino.h>

#include <cinttypes>
#include <cstdint>

#include "ILI9341_t3n.h"
#include "pw_color/color.h"
#include "pw_framebuffer/rgb565.h"

namespace pw::display {

namespace {

constexpr int TFT_DC = 10;    // stm32f429i-disc1: PD13
constexpr int TFT_CS = 9;     // stm32f429i-disc1: PC2
constexpr int TFT_RST = 8;    // stm32f429i-disc1: NRST
constexpr int TFT_MOSI = 11;  // stm32f429i-disc1: PF9
constexpr int TFT_SCLK = 13;  // stm32f429i-disc1: PF7
constexpr int TFT_MISO = 12;  // stm32f429i-disc1: PF8

constexpr int kDisplayWidth = 320;
constexpr int kDisplayHeight = 240;
constexpr int kDisplayDataSize = kDisplayWidth * kDisplayHeight;

ILI9341_t3n tft = ILI9341_t3n(TFT_CS, TFT_DC, TFT_RST);
uint16_t internal_framebuffer[kDisplayDataSize];

}  // namespace

uint16_t* GetInternalFramebuffer() { return internal_framebuffer; }

void Init() {
  for (int i = 0; i < 320 * 240; i++) {
    internal_framebuffer[i] = 0x0726;
  }
  // SPI Clock: 40MHz writes & 20MHz reads.
  // Lower write speed to 30MHz if the display doesn't work.
  tft.begin(30000000u, 2000000);
  tft.setFrameBuffer(internal_framebuffer);
  tft.useFrameBuffer(1);
  tft.setRotation(3);
  tft.setCursor(0, 0);
  tft.updateScreen();
}

int GetWidth() { return tft.width(); }
int GetHeight() { return tft.height(); }

void Update(pw::framebuffer::FramebufferRgb565* frame_buffer) {
  tft.updateScreen();
}

bool TouchscreenAvailable() { return false; }

bool NewTouchEvent() { return false; }

pw::coordinates::Vec3Int GetTouchPoint() {
  pw::coordinates::Vec3Int point;
  point.x = 0;
  point.y = 0;
  point.z = 0;
  return point;
}

}  // namespace pw::display
