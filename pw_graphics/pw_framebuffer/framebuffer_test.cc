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
  EXPECT_EQ(fb.GetWidth(), 32);
  EXPECT_EQ(fb.GetHeight(), 32);
}

TEST(FramebufferRgb565, Fill) {
  uint16_t data[8 * 8];
  FramebufferRgb565 fb(data, 8, 8);
  color_rgb565_t* const pixel_data = fb.GetFramebufferData();
  color_rgb565_t indigo = 0x83b3;
  fb.Fill(indigo);
  // First pixel
  EXPECT_EQ(pixel_data[0], 0x83b3);
  // Last pixel
  EXPECT_EQ(pixel_data[8 * 8 - 1], 0x83b3);
}

TEST(FramebufferRgb565, SetPixelGetPixel) {
  uint16_t data[8 * 8];
  FramebufferRgb565 fb(data, 8, 8);
  color_rgb565_t* const pixel_data = fb.GetFramebufferData();
  color_rgb565_t indigo = 0x83b3;
  fb.Fill(0);
  for (int i = 0; i < 8; i++) {
    fb.SetPixel(i, i, indigo);
  }
  EXPECT_EQ(pixel_data[0], indigo);
  EXPECT_EQ(pixel_data[1], 0);
  EXPECT_EQ(pixel_data[8 * 8 - 2], 0);
  EXPECT_EQ(pixel_data[8 * 8 - 1], indigo);
  EXPECT_EQ(fb.GetPixel(0, 0), indigo);
  EXPECT_EQ(fb.GetPixel(0, 1), 0);
  EXPECT_EQ(fb.GetPixel(6, 7), 0);
  EXPECT_EQ(fb.GetPixel(7, 7), indigo);
}

TEST(FramebufferRgb565, Blit) {
  uint16_t data[8 * 8];
  FramebufferRgb565 fb(data, 8, 8);
  color_rgb565_t indigo = colors_pico8_rgb565[12];
  fb.Fill(indigo);
  color_rgb565_t* const pixel_data = fb.GetFramebufferData();
  // First pixel
  EXPECT_EQ(pixel_data[0], indigo);
  // Last pixel
  EXPECT_EQ(pixel_data[8 * 8 - 1], indigo);

  uint16_t data2[4 * 4];
  FramebufferRgb565 fb2(data2, 4, 4);
  color_rgb565_t orange = 0xfd00;
  fb2.Fill(orange);

  // Do the blits
  fb.Blit(&fb2, -3, -3);

  fb.Blit(&fb2, 2, 2);

  // TODO(tonymd): Include PrintFramebufferAsANSI from draw_test.cc
  // PrintFramebufferAsANSI(&fb);

  // One orange pixel in the upper left corner
  EXPECT_EQ(pixel_data[0], orange);
  EXPECT_EQ(pixel_data[1], indigo);
  EXPECT_EQ(pixel_data[8], indigo);
  EXPECT_EQ(pixel_data[9], indigo);

  // Center 4x4 square is orange
  // x = 1
  EXPECT_EQ(pixel_data[(8 * 1) + 1], indigo);
  EXPECT_EQ(pixel_data[(8 * 1) + 2], indigo);
  EXPECT_EQ(pixel_data[(8 * 1) + 3], indigo);
  EXPECT_EQ(pixel_data[(8 * 1) + 4], indigo);
  EXPECT_EQ(pixel_data[(8 * 1) + 5], indigo);
  EXPECT_EQ(pixel_data[(8 * 1) + 6], indigo);

  // x = 2
  EXPECT_EQ(pixel_data[(8 * 2) + 1], indigo);
  EXPECT_EQ(pixel_data[(8 * 2) + 2], orange);
  EXPECT_EQ(pixel_data[(8 * 2) + 3], orange);
  EXPECT_EQ(pixel_data[(8 * 2) + 4], orange);
  EXPECT_EQ(pixel_data[(8 * 2) + 5], orange);
  EXPECT_EQ(pixel_data[(8 * 2) + 6], indigo);

  // x = 5
  EXPECT_EQ(pixel_data[(8 * 5) + 1], indigo);
  EXPECT_EQ(pixel_data[(8 * 5) + 2], orange);
  EXPECT_EQ(pixel_data[(8 * 5) + 3], orange);
  EXPECT_EQ(pixel_data[(8 * 5) + 4], orange);
  EXPECT_EQ(pixel_data[(8 * 5) + 5], orange);
  EXPECT_EQ(pixel_data[(8 * 5) + 6], indigo);

  // x = 6
  EXPECT_EQ(pixel_data[(8 * 6) + 1], indigo);
  EXPECT_EQ(pixel_data[(8 * 6) + 2], indigo);
  EXPECT_EQ(pixel_data[(8 * 6) + 3], indigo);
  EXPECT_EQ(pixel_data[(8 * 6) + 4], indigo);
  EXPECT_EQ(pixel_data[(8 * 6) + 5], indigo);
  EXPECT_EQ(pixel_data[(8 * 6) + 6], indigo);
}

}  // namespace
}  // namespace pw::framebuffer
