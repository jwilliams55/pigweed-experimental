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

#include "pw_assert/assert.h"

using pw::color::color_rgb565_t;

namespace pw::framebuffer {

FramebufferRgb565::FramebufferRgb565()
    : pixel_data_(nullptr), width_(0), height_(0), row_bytes_(0) {}

FramebufferRgb565::FramebufferRgb565(color_rgb565_t* data,
                                     int width,
                                     int height,
                                     int row_bytes)
    : pixel_data_(data),
      width_(width),
      height_(height),
      row_bytes_(row_bytes) {}

void FramebufferRgb565::SetFramebufferData(color_rgb565_t* data,
                                           int width,
                                           int height,
                                           int row_bytes) {
  width_ = width;
  height_ = height;
  row_bytes_ = row_bytes;
  pixel_data_ = data;
}

// Return the RGB565 color at position x, y. Bounds are checked.
Result<color_rgb565_t> FramebufferRgb565::GetPixel(int x, int y) const {
  PW_ASSERT(IsValid());
  if (x >= 0 && x < width_ && y >= 0 && y < height_) {
    return pixel_data_[y * width_ + x];
  }
  return Status::OutOfRange();
}

// Draw a color at (x, y) if it's a valid position.
void FramebufferRgb565::SetPixel(int x, int y, color_rgb565_t rgb565_color) {
  PW_ASSERT(IsValid());
  if (x >= 0 && x < width_ && y >= 0 && y < height_) {
    pixel_data_[y * width_ + x] = rgb565_color;
  }
}

// Copy the colors from another framebuffer into this one at position x, y.
void FramebufferRgb565::Blit(FramebufferRgb565* fb, int x, int y) {
  PW_ASSERT(fb->IsValid());
  for (int current_x = 0; current_x < fb->width_; current_x++) {
    for (int current_y = 0; current_y < fb->height_; current_y++) {
      if (auto pixel_color = fb->GetPixel(current_x, current_y);
          pixel_color.ok()) {
        SetPixel(x + current_x, y + current_y, pixel_color.value());
      }
    }
  }
}

// Fill the entire buffer with a color.
void FramebufferRgb565::Fill(color_rgb565_t color) {
  PW_ASSERT(IsValid());
  for (int i = 0; i < width_ * height_; i++) {
    pixel_data_[i] = color;
  }
}

}  // namespace pw::framebuffer
