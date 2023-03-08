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

#include <cstdint>

#include "pw_math/size.h"
#include "pw_result/result.h"

namespace pw::framebuffer {

// A Framebuffer refers to a buffer of pixel data and the various attributes
// of that pixel data (such as dimensions, rowbytes, etc.).
class Framebuffer {
 public:
  // Construct a default invalid framebuffer.
  Framebuffer();

  // Construct a framebuffer of the specified dimensions which *does not* own
  // the |data| - i.e. this instance will never attempt to free it.
  Framebuffer(void* data, pw::math::Size<uint16_t> size, uint16_t row_bytes);

  Framebuffer(const Framebuffer&) = delete;
  Framebuffer(Framebuffer&& other);

  Framebuffer& operator=(const Framebuffer&) = delete;
  Framebuffer& operator=(Framebuffer&&);

  // Has the framebuffer been properly initialized?
  bool IsValid() const { return pixel_data_ != nullptr; };

  // Return a pointer to the framebuffer pixel buffer.
  void* GetFramebufferData() const { return pixel_data_; }

  // Return the framebuffer size which is the width and height of the
  // framebuffer in pixels.
  pw::math::Size<uint16_t> size() const { return size_; }

  // Return the number of bytes per row of pixel data.
  uint16_t GetRowBytes() const { return row_bytes_; }

 private:
  void* pixel_data_;               // The pixel buffer.
  pw::math::Size<uint16_t> size_;  // width/height (in pixels) of |pixel_data_|.
  uint16_t row_bytes_;             // The number of bytes in each row.
};

}  // namespace pw::framebuffer
