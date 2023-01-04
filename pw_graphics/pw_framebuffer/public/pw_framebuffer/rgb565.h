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
#include "pw_result/result.h"

namespace pw::framebuffer {

class FramebufferRgb565 {
 public:
  // Construct a default invalid framebuffer. Using an
  // invalid framebuffer will result in a failed PW_CHECK.
  FramebufferRgb565();

  // Construct a framebuffer of the specified dimensions which *does not* own
  // the |data| - i.e. this instance may write to the data, but will never
  // attempt to free it.
  FramebufferRgb565(pw::color::color_rgb565_t* data,
                    int width,
                    int height,
                    int row_bytes);

  FramebufferRgb565(const FramebufferRgb565&) = delete;
  FramebufferRgb565(FramebufferRgb565&& other);

  FramebufferRgb565& operator=(const FramebufferRgb565&) = delete;
  FramebufferRgb565& operator=(FramebufferRgb565&&);

  // Has the framebuffer been properly initialized?
  bool IsValid() const { return pixel_data_ != nullptr; };

  pw::color::color_rgb565_t* GetFramebufferData() const { return pixel_data_; }

  // Return the RGB565 color at position x, y. Bounds are checked.
  Result<pw::color::color_rgb565_t> GetPixel(int x, int y) const;

  // Draw a color at (x, y) if it's a valid position.
  void SetPixel(int x, int y, pw::color::color_rgb565_t rgb565_color);

  // Copy the colors from another framebuffer into this one at position x, y.
  void Blit(FramebufferRgb565* fb, int x, int y);

  // Fill the entire buffer with a color.
  void Fill(pw::color::color_rgb565_t color);

  // Return framebuffer width in pixels.
  int GetWidth() const { return width_; }

  // Return framebuffer height in pixels.
  int GetHeight() const { return height_; }

  // Return the number of bytes per row of pixel data.
  int GetRowBytes() const { return row_bytes_; }

 private:
  pw::color::color_rgb565_t* pixel_data_;
  int width_;
  int height_;
  int row_bytes_;
};

}  // namespace pw::framebuffer
