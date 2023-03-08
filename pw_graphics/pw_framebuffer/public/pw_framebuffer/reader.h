// Copyright 2023 The Pigweed Authors
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

#include "pw_color/color.h"
#include "pw_framebuffer/framebuffer.h"
#include "pw_result/result.h"

namespace pw::framebuffer {

// An interface to Framebuffer to simplify reading pixel values from a
// framebuffer.
//
// Note: This implementation is not designed for performance, and is intended
// to be used for development (testing) and other cases where drawing
// performance is not important.
class FramebufferReader {
 public:
  FramebufferReader(const Framebuffer& framebuffer);

  // Return the pixel value at position (x, y). Bounds are checked.
  Result<pw::color::color_rgb565_t> GetPixel(uint16_t x, uint16_t y) const;

 protected:
  const Framebuffer& framebuffer_;
};

}  // namespace pw::framebuffer
