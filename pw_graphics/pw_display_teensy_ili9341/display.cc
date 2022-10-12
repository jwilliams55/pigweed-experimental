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

}  // namespace

void Init() {
  // SPI Clock: 30MHz writes & 20MHz reads.
  // Lower write speed if the display doesn't work.
  tft.begin(30000000u, 2000000);
  tft.useFrameBuffer(1);
  tft.setRotation(3);
  tft.setCursor(0, 0);
}

int GetWidth() { return tft.width(); }
int GetHeight() { return tft.height(); }

void UpdatePixelDouble(pw::framebuffer::FramebufferRgb565* frame_buffer) {
  // Not implemented.
}

void Update(pw::framebuffer::FramebufferRgb565* frame_buffer) {
  tft.setFrameBuffer(frame_buffer->pixel_data);
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

Status InitFramebuffer(FramebufferRgb565* framebuffer) {
  framebuffer->SetFramebufferData(
      framebuffer_data, kDisplayWidth, kDisplayHeight);
  return OkStatus();
}

}  // namespace pw::display
