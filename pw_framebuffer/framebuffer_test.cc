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

#include <cstdint>

#include "gtest/gtest.h"
#include "pw_color/color.h"
#include "pw_color/colors_endesga32.h"
#include "pw_color/colors_pico8.h"
#include "pw_framebuffer/rgb565.h"
#include "pw_log/log.h"

namespace pw::framebuffer {
namespace {

TEST(FramebufferRgb565, Init) {
  uint16_t data[32 * 32];
  FramebufferRgb565 fb(data, 32, 32);
  EXPECT_EQ(fb.width, 32);
  EXPECT_EQ(fb.height, 32);
}

TEST(FramebufferRgb565, Fill) {
  uint16_t data[8 * 8];
  FramebufferRgb565 fb(data, 8, 8);
  color_rgb565_t indigo = 0x83b3;
  fb.Fill(indigo);
  // First pixel
  EXPECT_EQ(fb.pixel_data[0], 0x83b3);
  // Last pixel
  EXPECT_EQ(fb.pixel_data[8 * 8 - 1], 0x83b3);
}

TEST(FramebufferRgb565, SetPixelGetPixel) {
  uint16_t data[8 * 8];
  FramebufferRgb565 fb(data, 8, 8);
  color_rgb565_t indigo = 0x83b3;
  fb.Fill(0);
  for (int i = 0; i < 8; i++) {
    fb.SetPixel(i, i, indigo);
  }
  EXPECT_EQ(fb.pixel_data[0], indigo);
  EXPECT_EQ(fb.pixel_data[1], 0);
  EXPECT_EQ(fb.pixel_data[8 * 8 - 2], 0);
  EXPECT_EQ(fb.pixel_data[8 * 8 - 1], indigo);
  EXPECT_EQ(fb.GetPixel(0, 0), indigo);
  EXPECT_EQ(fb.GetPixel(0, 1), 0);
  EXPECT_EQ(fb.GetPixel(6, 7), 0);
  EXPECT_EQ(fb.GetPixel(7, 7), indigo);
}

}  // namespace
}  // namespace pw::framebuffer
