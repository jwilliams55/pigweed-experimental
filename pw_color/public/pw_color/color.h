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
#pragma once

#include <math.h>
#include <stdint.h>

#include <cinttypes>
#include <cstdint>

namespace pw::color {

typedef uint32_t color_rgba8888_t;
typedef uint16_t color_rgb565_t;
typedef uint8_t color_1bit_t;
typedef uint8_t color_2bit_t;

class ColorRGBA {
 public:
  uint8_t r;
  uint8_t g;
  uint8_t b;
  uint8_t a;

  ColorRGBA(uint8_t ir, uint8_t ig, uint8_t ib) {
    r = ir;
    g = ig;
    b = ib;
    a = 255;
  }

  ColorRGBA(uint8_t ir, uint8_t ig, uint8_t ib, uint8_t ia) {
    r = ir;
    g = ig;
    b = ib;
    a = ia;
  }

  ColorRGBA(color_rgb565_t rgb565) {
    // Grab the RGB bits
    uint8_t ir = (rgb565 & 0xF800) >> 11;
    uint8_t ig = (rgb565 & 0x7E0) >> 5;
    uint8_t ib = rgb565 & 0x1F;
    // Scale RGB values to 8bits each
    r = round(255.0 * ((float)ir / 31.0));
    g = round(255.0 * ((float)ig / 63.0));
    b = round(255.0 * ((float)ib / 31.0));
    a = 255;
  }

  ColorRGBA(color_rgba8888_t rgba8888) {
    a = (rgba8888 & 0xFF000000) >> 24;
    b = (rgba8888 & 0xFF0000) >> 16;
    g = (rgba8888 & 0xFF00) >> 8;
    r = (rgba8888 & 0xFF);
  }

  color_rgb565_t ToRgb565() {
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xF8) >> 3);
  }

  /*
color_rgb565_t ColorToRGB565(color_rgba8888_t rgba8888) {
  uint8_t b = (rgba8888 & 0xFF0000) >> 16;
  uint8_t g = (rgba8888 & 0xFF00) >> 8;
  uint8_t r = (rgba8888 & 0xFF);
  return (color_rgb565_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) |
                          ((b & 0xF8) >> 3));
}
  */
};

}  // namespace pw::color
