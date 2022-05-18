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

#include <cstddef>
#include <cstdint>

#include "pw_color/color.h"

using namespace pw::color;

namespace pw::framebuffer {

class FramebufferRgb565 {
 public:
  // TODO(tonymd): Add a stride variable. Right now width is being treated as
  // the stride vale.
  int width, height;
  color_rgb565_t* pixel_data;
  color_rgb565_t pen_color;
  color_rgb565_t transparent_color;

  FramebufferRgb565() {
    width = 0;
    height = 0;
    pixel_data = NULL;
    SetDefaultColors();
  }

  FramebufferRgb565(color_rgb565_t* data,
                    int desired_width,
                    int desired_height) {
    width = desired_width;
    height = desired_height;
    pixel_data = data;
    SetDefaultColors();
  }

  void SetDefaultColors() {
    pen_color = ColorRGBA(0xff, 0xff, 0xff).ToRgb565();          // White
    transparent_color = ColorRGBA(0xff, 0x00, 0xff).ToRgb565();  // Magenta
  }

  color_rgb565_t* GetFramebufferData() { return pixel_data; }

  void SetFramebufferData(color_rgb565_t* data,
                          int desired_width,
                          int desired_height) {
    width = desired_width;
    height = desired_height;
    pixel_data = data;
  }

  // Return the RGB565 color at position x, y. Bounds are checked.
  color_rgb565_t GetPixel(int x, int y) {
    color_rgb565_t value = transparent_color;
    if (x >= 0 && x < width && y >= 0 && y < height) {
      value = pixel_data[y * width + x];
    }
    return value;
  }

  // Draw a color at (x, y) if it's a valid positon.
  void SetPixel(int x, int y, color_rgb565_t rgb565_color) {
    if (x >= 0 && x < width && y >= 0 && y < height) {
      pixel_data[y * width + x] = rgb565_color;
    }
  }

  // Draw the current pen color at x, y. Check that x, y is a valid positon.
  void SetPixel(int x, int y) { SetPixel(x, y, pen_color); }

  // Fill the entire buffer with a color.
  void Fill(color_rgb565_t color) {
    for (int i = 0; i < width * height; i++) {
      pixel_data[i] = color;
    }
  }

  // Fill the entire buffer with the pen color.
  void Fill() {
    for (int i = 0; i < width * height; i++) {
      pixel_data[i] = pen_color;
    }
  }

  void SetPenColor(color_rgb565_t color) { pen_color = color; }

  color_rgb565_t GetPenColor() { return pen_color; }

  void SetTransparentColor(color_rgb565_t color) { transparent_color = color; }

  color_rgb565_t GetTransparentColor() { return transparent_color; }
};

}  // namespace pw::framebuffer
