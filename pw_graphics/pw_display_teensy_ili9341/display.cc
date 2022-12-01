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

#include <Arduino.h>

#include <cinttypes>
#include <cstdint>

#include "ILI9341_t3n.h"
#include "pw_color/color.h"
#include "pw_display/display_backend.h"
#include "pw_framebuffer/rgb565.h"

using pw::framebuffer::FramebufferRgb565;

namespace pw::display::backend {

namespace {

constexpr int TFT_DC = 9;
constexpr int TFT_CS = 32;
constexpr int TFT_RST = 3;
constexpr int TFT_MOSI = 11;
constexpr int TFT_SCLK = 13;
constexpr int TFT_MISO = 12;

constexpr int kDisplayWidth = 320;
constexpr int kDisplayHeight = 240;
constexpr int kDisplayDataSize = kDisplayWidth * kDisplayHeight;

uint16_t framebuffer_data[kDisplayDataSize];
ILI9341_t3n tft = ILI9341_t3n(TFT_CS, TFT_DC, TFT_RST);

}  // namespace

Display::Display() = default;

Display::~Display() = default;

Status Display::Init() {
  // SPI Clock: 30MHz writes & 20MHz reads.
  // Lower write speed if the display doesn't work.
  tft.begin(30000000u, 2000000);
  tft.useFrameBuffer(1);
  tft.setRotation(3);
  tft.setCursor(0, 0);
  return OkStatus();
}

int Display::GetWidth() const { return tft.width(); }
int Display::GetHeight() const { return tft.height(); }

void Display::Update(FramebufferRgb565& frame_buffer) {
  tft.setFrameBuffer(frame_buffer.GetFramebufferData());
  tft.updateScreen();
}

bool Display::TouchscreenAvailable() const { return false; }

bool Display::NewTouchEvent() { return false; }

pw::coordinates::Vec3Int Display::GetTouchPoint() {
  pw::coordinates::Vec3Int point;
  point.x = 0;
  point.y = 0;
  point.z = 0;
  return point;
}

Status Display::InitFramebuffer(FramebufferRgb565* framebuffer) {
  framebuffer->SetFramebufferData(framebuffer_data,
                                  kDisplayWidth,
                                  kDisplayHeight,
                                  kDisplayWidth * sizeof(uint16_t));
  return OkStatus();
}

}  // namespace pw::display::backend
