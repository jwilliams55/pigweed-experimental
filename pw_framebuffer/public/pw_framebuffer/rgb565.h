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

  FramebufferRgb565();

  FramebufferRgb565(color_rgb565_t* data,
                    int desired_width,
                    int desired_height);

  void SetDefaultColors();

  color_rgb565_t* GetFramebufferData();

  void SetFramebufferData(color_rgb565_t* data,
                          int desired_width,
                          int desired_height);

  // Return the RGB565 color at position x, y. Bounds are checked.
  color_rgb565_t GetPixel(int x, int y);

  // Draw a color at (x, y) if it's a valid positon.
  void SetPixel(int x, int y, color_rgb565_t rgb565_color);

  // Draw the current pen color at x, y. Check that x, y is a valid positon.
  void SetPixel(int x, int y);

  // Copy the colors from another framebuffer into this one at position x, y.
  void Blit(FramebufferRgb565* fb, int x, int y);

  // Fill the entire buffer with a color.
  void Fill(color_rgb565_t color);

  // Fill the entire buffer with the pen color.
  void Fill();

  void SetPenColor(color_rgb565_t color);

  color_rgb565_t GetPenColor();

  void SetTransparentColor(color_rgb565_t color);

  color_rgb565_t GetTransparentColor();
};

}  // namespace pw::framebuffer
