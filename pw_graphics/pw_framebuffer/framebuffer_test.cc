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

#include "pw_framebuffer/framebuffer.h"

#include <cstdint>

#include "gtest/gtest.h"
#include "pw_color/color.h"
#include "pw_color/colors_endesga32.h"
#include "pw_color/colors_pico8.h"
#include "pw_framebuffer/writer.h"
#include "pw_log/log.h"
#include "pw_math/size.h"

using pw::color::color_rgb565_t;

namespace pw::framebuffer {
namespace {

static_assert(!std::is_copy_constructible_v<Framebuffer>,
              "Framebuffer must not be copy constructable");
static_assert(std::is_move_constructible_v<Framebuffer>,
              "Framebuffer needs to be moveable");
static_assert(!std::is_trivially_move_constructible_v<Framebuffer>,
              "Framebuffer move may not be trivial");

TEST(Framebuffer, Default) {
  Framebuffer fb;

  EXPECT_EQ(false, fb.is_valid());
  EXPECT_EQ(0, fb.size().width);
  EXPECT_EQ(0, fb.size().height);
  EXPECT_EQ(0, fb.row_bytes());
  EXPECT_EQ(PixelFormat::None, fb.pixel_format());
  EXPECT_EQ(nullptr, fb.data());
}

TEST(Framebuffer, Init) {
  constexpr pw::math::Size<uint16_t> kDimensions = {32, 40};
  constexpr uint16_t kRowBytes = kDimensions.width * sizeof(color_rgb565_t);

  color_rgb565_t data[kDimensions.width * kDimensions.height];
  Framebuffer fb(data, PixelFormat::RGB565, kDimensions, kRowBytes);

  EXPECT_EQ(true, fb.is_valid());
  EXPECT_EQ(32, fb.size().width);
  EXPECT_EQ(40, fb.size().height);
  EXPECT_EQ(kRowBytes, fb.row_bytes());
  EXPECT_EQ(PixelFormat::RGB565, fb.pixel_format());
  EXPECT_EQ(data, fb.data());
}

}  // namespace
}  // namespace pw::framebuffer
