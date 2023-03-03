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

#include <array>

#include "pw_color/color.h"
#include "pw_coordinates/size.h"
#include "pw_coordinates/vector2.h"

namespace pw::framebuffer::pool {

constexpr size_t kMaxFramebufferCount = 3;

struct PoolData {
  std::array<pw::color::color_rgb565_t*, kMaxFramebufferCount> fb_addr;
  size_t num_fb;  // <= fb_addr.size().
  pw::coordinates::Size<int> size;
  int row_bytes;
  pw::coordinates::Vector2<int> start;
};

}  // namespace pw::framebuffer::pool
