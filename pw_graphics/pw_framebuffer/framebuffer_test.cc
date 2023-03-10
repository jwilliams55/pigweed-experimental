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

using pw::color::color_rgb565_t;

namespace pw::framebuffer {
namespace {

TEST(Framebuffer, Init) {
  color_rgb565_t data[32 * 32];
  Framebuffer fb(data, PixelFormat::RGB565, {32, 32}, 32 * sizeof(data[0]));
  EXPECT_EQ(fb.size().width, 32);
  EXPECT_EQ(fb.size().height, 32);
}

}  // namespace
}  // namespace pw::framebuffer
