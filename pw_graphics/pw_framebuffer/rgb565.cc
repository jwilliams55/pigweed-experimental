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
#include "public/pw_framebuffer/rgb565.h"

#include <cstddef>

#include "pw_color/color.h"

using namespace pw::color;

namespace pw::framebuffer {

FramebufferRgb565::FramebufferRgb565() {
  width = 0;
  height = 0;
  pixel_data = NULL;
  SetDefaultColors();
}

FramebufferRgb565::FramebufferRgb565(color_rgb565_t* data,
                                     int desired_width,
                                     int desired_height) {
  width = desired_width;
  height = desired_height;
  pixel_data = data;
  SetDefaultColors();
}

void FramebufferRgb565::SetDefaultColors() {
  pen_color = ColorRGBA(0xff, 0xff, 0xff).ToRgb565();          // White
  transparent_color = ColorRGBA(0xff, 0x00, 0xff).ToRgb565();  // Magenta
}

color_rgb565_t* FramebufferRgb565::GetFramebufferData() { return pixel_data; }

void FramebufferRgb565::SetFramebufferData(color_rgb565_t* data,
                                           int desired_width,
                                           int desired_height) {
  width = desired_width;
  height = desired_height;
  pixel_data = data;
}

// Return the RGB565 color at position x, y. Bounds are checked.
color_rgb565_t FramebufferRgb565::GetPixel(int x, int y) {
  color_rgb565_t value = transparent_color;
  if (x >= 0 && x < width && y >= 0 && y < height) {
    value = pixel_data[y * width + x];
  }
  return value;
}

// Draw a color at (x, y) if it's a valid positon.
void FramebufferRgb565::SetPixel(int x, int y, color_rgb565_t rgb565_color) {
  if (x >= 0 && x < width && y >= 0 && y < height) {
    pixel_data[y * width + x] = rgb565_color;
  }
}

// Draw the current pen color at x, y. Check that x, y is a valid positon.
void FramebufferRgb565::SetPixel(int x, int y) { SetPixel(x, y, pen_color); }

// Copy the colors from another framebuffer into this one at position x, y.
void FramebufferRgb565::Blit(FramebufferRgb565* fb, int x, int y) {
  color_rgb565_t pixel_color;
  for (int current_x = 0; current_x < fb->width; current_x++) {
    for (int current_y = 0; current_y < fb->height; current_y++) {
      pixel_color = fb->GetPixel(current_x, current_y);
      if (pixel_color != fb->transparent_color &&
          pixel_color != transparent_color) {
        SetPixel(x + current_x, y + current_y, pixel_color);
      }
    }
  }
}

// Fill the entire buffer with a color.
void FramebufferRgb565::Fill(color_rgb565_t color) {
  for (int i = 0; i < width * height; i++) {
    pixel_data[i] = color;
  }
}

// Fill the entire buffer with the pen color.
void FramebufferRgb565::Fill() {
  for (int i = 0; i < width * height; i++) {
    pixel_data[i] = pen_color;
  }
}

void FramebufferRgb565::SetPenColor(color_rgb565_t color) { pen_color = color; }

color_rgb565_t FramebufferRgb565::GetPenColor() { return pen_color; }

void FramebufferRgb565::SetTransparentColor(color_rgb565_t color) {
  transparent_color = color;
}

color_rgb565_t FramebufferRgb565::GetTransparentColor() {
  return transparent_color;
}

}  // namespace pw::framebuffer
