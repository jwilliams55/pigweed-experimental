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

namespace pw::framebuffer {

Framebuffer::Framebuffer()
    : pixel_data_(nullptr),
      pixel_format_(PixelFormat::None),
      size_{0, 0},
      row_bytes_(0) {}

Framebuffer::Framebuffer(void* data,
                         PixelFormat pixel_format,
                         pw::math::Size<uint16_t> size,
                         uint16_t row_bytes)
    : pixel_data_(data),
      pixel_format_(pixel_format),
      size_(size),
      row_bytes_(row_bytes) {
  PW_ASSERT(data != nullptr);
  PW_ASSERT(pixel_format != PixelFormat::None);
}

Framebuffer::Framebuffer(Framebuffer&& other)
    : pixel_data_(other.pixel_data_),
      pixel_format_(other.pixel_format_),
      size_(other.size_),
      row_bytes_(other.row_bytes_) {
  other.pixel_data_ = nullptr;
  other.pixel_format_ = PixelFormat::None;
}

Framebuffer& Framebuffer::operator=(Framebuffer&& rhs) {
  pixel_data_ = rhs.pixel_data_;
  pixel_format_ = rhs.pixel_format_;
  size_ = rhs.size_;
  row_bytes_ = rhs.row_bytes_;
  rhs.pixel_data_ = nullptr;
  rhs.pixel_format_ = PixelFormat::None;
  return *this;
}

}  // namespace pw::framebuffer
