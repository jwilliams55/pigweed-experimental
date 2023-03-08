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
#include "public/pw_framebuffer/framebuffer.h"

#include <cstddef>

#include "pw_assert/assert.h"

using pw::color::color_rgb565_t;

namespace pw::framebuffer {

Framebuffer::Framebuffer() : pixel_data_(nullptr), size_{0, 0}, row_bytes_(0) {}

Framebuffer::Framebuffer(color_rgb565_t* data,
                         pw::math::Size<uint16_t> size,
                         uint16_t row_bytes)
    : pixel_data_(data), size_(size), row_bytes_(row_bytes) {}

Framebuffer::Framebuffer(Framebuffer&& other)
    : pixel_data_(other.pixel_data_),
      size_(other.size_),
      row_bytes_(other.row_bytes_) {
  other.pixel_data_ = nullptr;
}

Framebuffer& Framebuffer::operator=(Framebuffer&& rhs) {
  pixel_data_ = rhs.pixel_data_;
  size_ = rhs.size_;
  row_bytes_ = rhs.row_bytes_;
  rhs.pixel_data_ = nullptr;
  return *this;
}

// Return the RGB565 color at position x, y. Bounds are checked.
Result<color_rgb565_t> Framebuffer::GetPixel(uint16_t x, uint16_t y) const {
  PW_ASSERT(IsValid());
  if (x < size_.width && y < size_.height) {
    return pixel_data_[y * size_.width + x];
  }
  return Status::OutOfRange();
}

// Draw a color at (x, y) if it's a valid position.
void Framebuffer::SetPixel(uint16_t x,
                           uint16_t y,
                           color_rgb565_t rgb565_color) {
  PW_ASSERT(IsValid());
  if (x < size_.width && y < size_.height) {
    pixel_data_[y * size_.width + x] = rgb565_color;
  }
}

// Copy the colors from another framebuffer into this one at position x, y.
void Framebuffer::Blit(const Framebuffer& fb, uint16_t x, uint16_t y) {
  PW_ASSERT(fb.IsValid());
  for (uint16_t current_x = 0; current_x < fb.size_.width; current_x++) {
    for (uint16_t current_y = 0; current_y < fb.size_.height; current_y++) {
      if (auto pixel_color = fb.GetPixel(current_x, current_y);
          pixel_color.ok()) {
        SetPixel(x + current_x, y + current_y, pixel_color.value());
      }
    }
  }
}

// Fill the entire buffer with a color.
void Framebuffer::Fill(color_rgb565_t color) {
  PW_ASSERT(IsValid());
  for (size_t i = 0; i < size_.width * size_.height; i++) {
    pixel_data_[i] = color;
  }
}

}  // namespace pw::framebuffer
