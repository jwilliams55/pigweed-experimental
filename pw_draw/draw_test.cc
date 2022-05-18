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

#include "pw_draw/draw.h"

#include "gtest/gtest.h"
#include "pw_color/color.h"
#include "pw_color/colors_pico8.h"
#include "pw_framebuffer/rgb565.h"
#include "pw_log/log.h"
#include "pw_string/string_builder.h"

using namespace pw::draw;
using namespace pw::framebuffer;

namespace pw::draw {
namespace {

void PrintFramebufferAsANSI(FramebufferRgb565* fb) {
  pw::StringBuffer<1024> line;
  pw::StringBuffer<128> color_string;
  for (int y = 0; y < fb->height; y += 2) {
    line.clear();
    for (int x = 0; x < fb->width; x++) {
      color_string.clear();
      ColorRGBA row1(fb->GetPixel(x, y));
      color_rgb565_t row2_color = fb->GetPixel(x, y + 1);
      if (row2_color == fb->transparent_color) {
        color_string.Format("[m[38;2;%d;%d;%dm▀", row1.r, row1.g, row1.b);
      } else {
        ColorRGBA row2(row2_color);
        color_string.Format("[m[38;2;%d;%d;%dm[48;2;%d;%d;%dm▀",
                            row1.r,
                            row1.g,
                            row1.b,
                            row2.r,
                            row2.g,
                            row2.b);
      }
      line.append(color_string.data());
    }
    line.append("[m\n");
    // TODO(tonymd): Log seems to drop characters, maybe a size limit?
    // PW_LOG_INFO("%s", line.data());
    printf("%s", line.data());
  }
}

TEST(DrawLine, Diagonal) {
  uint16_t data[4 * 4];
  FramebufferRgb565 fb(data, 4, 4);
  color_rgb565_t indigo = colors_pico8_rgb565[12];

  fb.Fill(0);

  DrawLine(&fb, 0, 0, fb.width, fb.height, indigo);
  PrintFramebufferAsANSI(&fb);

  // Check the diagonal is set to indigo, everything else should be 0.
  for (int x = 0; x < fb.width; x++) {
    for (int y = 0; y < fb.height; y++) {
      if (x == y)
        EXPECT_EQ(fb.GetPixel(x, y), indigo);
      else
        EXPECT_EQ(fb.GetPixel(x, y), 0);
    }
  }
}

TEST(DrawHLine, Top) {
  uint16_t data[4 * 4];
  FramebufferRgb565 fb(data, 4, 4);
  color_rgb565_t indigo = colors_pico8_rgb565[12];
  fb.Fill(0);

  // Horizonal line at y = 0
  DrawHLine(&fb, 0, fb.width, 0, indigo);
  PrintFramebufferAsANSI(&fb);

  // Check color at y = 0 is indigo, y = 1 should be 0.
  for (int x = 0; x < fb.width; x++) {
    EXPECT_EQ(fb.GetPixel(x, 0), indigo);
    EXPECT_EQ(fb.GetPixel(x, 1), 0);
  }
}

TEST(DrawRect, Empty) {
  uint16_t data[5 * 5];
  FramebufferRgb565 fb(data, 5, 5);
  color_rgb565_t indigo = colors_pico8_rgb565[12];
  fb.Fill(0);

  // 4x4 rectangle, not filled
  DrawRect(&fb, 0, 0, 3, 3, indigo, false);
  PrintFramebufferAsANSI(&fb);

  EXPECT_EQ(fb.GetPixel(0, 0), indigo);
  EXPECT_EQ(fb.GetPixel(1, 0), indigo);
  EXPECT_EQ(fb.GetPixel(2, 0), indigo);
  EXPECT_EQ(fb.GetPixel(3, 0), indigo);
  EXPECT_EQ(fb.GetPixel(4, 0), 0);

  EXPECT_EQ(fb.GetPixel(0, 1), indigo);
  EXPECT_EQ(fb.GetPixel(1, 1), 0);
  EXPECT_EQ(fb.GetPixel(2, 1), 0);
  EXPECT_EQ(fb.GetPixel(3, 1), indigo);
  EXPECT_EQ(fb.GetPixel(4, 1), 0);

  EXPECT_EQ(fb.GetPixel(0, 2), indigo);
  EXPECT_EQ(fb.GetPixel(1, 2), 0);
  EXPECT_EQ(fb.GetPixel(2, 2), 0);
  EXPECT_EQ(fb.GetPixel(3, 2), indigo);
  EXPECT_EQ(fb.GetPixel(4, 2), 0);

  EXPECT_EQ(fb.GetPixel(0, 3), indigo);
  EXPECT_EQ(fb.GetPixel(1, 3), indigo);
  EXPECT_EQ(fb.GetPixel(2, 3), indigo);
  EXPECT_EQ(fb.GetPixel(3, 3), indigo);
  EXPECT_EQ(fb.GetPixel(4, 3), 0);

  EXPECT_EQ(fb.GetPixel(0, 4), 0);
  EXPECT_EQ(fb.GetPixel(1, 4), 0);
  EXPECT_EQ(fb.GetPixel(2, 4), 0);
  EXPECT_EQ(fb.GetPixel(3, 4), 0);
  EXPECT_EQ(fb.GetPixel(4, 4), 0);
}

TEST(DrawRect, Filled) {
  uint16_t data[5 * 5];
  FramebufferRgb565 fb(data, 5, 5);
  color_rgb565_t indigo = colors_pico8_rgb565[12];
  fb.Fill(0);

  // 4x4 rectangle, filled
  DrawRect(&fb, 0, 0, 3, 3, indigo, true);
  PrintFramebufferAsANSI(&fb);

  EXPECT_EQ(fb.GetPixel(0, 0), indigo);
  EXPECT_EQ(fb.GetPixel(1, 0), indigo);
  EXPECT_EQ(fb.GetPixel(2, 0), indigo);
  EXPECT_EQ(fb.GetPixel(3, 0), indigo);
  EXPECT_EQ(fb.GetPixel(4, 0), 0);

  EXPECT_EQ(fb.GetPixel(0, 1), indigo);
  EXPECT_EQ(fb.GetPixel(1, 1), indigo);
  EXPECT_EQ(fb.GetPixel(2, 1), indigo);
  EXPECT_EQ(fb.GetPixel(3, 1), indigo);
  EXPECT_EQ(fb.GetPixel(4, 1), 0);

  EXPECT_EQ(fb.GetPixel(0, 2), indigo);
  EXPECT_EQ(fb.GetPixel(1, 2), indigo);
  EXPECT_EQ(fb.GetPixel(2, 2), indigo);
  EXPECT_EQ(fb.GetPixel(3, 2), indigo);
  EXPECT_EQ(fb.GetPixel(4, 2), 0);

  EXPECT_EQ(fb.GetPixel(0, 3), indigo);
  EXPECT_EQ(fb.GetPixel(1, 3), indigo);
  EXPECT_EQ(fb.GetPixel(2, 3), indigo);
  EXPECT_EQ(fb.GetPixel(3, 3), indigo);
  EXPECT_EQ(fb.GetPixel(4, 3), 0);

  EXPECT_EQ(fb.GetPixel(0, 4), 0);
  EXPECT_EQ(fb.GetPixel(1, 4), 0);
  EXPECT_EQ(fb.GetPixel(2, 4), 0);
  EXPECT_EQ(fb.GetPixel(3, 4), 0);
  EXPECT_EQ(fb.GetPixel(4, 4), 0);
}

TEST(DrawRectWH, WidthHeightCorrect) {
  uint16_t data[5 * 5];
  FramebufferRgb565 fb(data, 5, 5);
  color_rgb565_t indigo = colors_pico8_rgb565[12];
  fb.Fill(0);

  // 4x4 rectangle, not filled
  DrawRectWH(&fb, 0, 0, 4, 4, indigo, false);
  PrintFramebufferAsANSI(&fb);

  EXPECT_EQ(fb.GetPixel(0, 0), indigo);
  EXPECT_EQ(fb.GetPixel(1, 0), indigo);
  EXPECT_EQ(fb.GetPixel(2, 0), indigo);
  EXPECT_EQ(fb.GetPixel(3, 0), indigo);
  EXPECT_EQ(fb.GetPixel(4, 0), 0);

  EXPECT_EQ(fb.GetPixel(0, 1), indigo);
  EXPECT_EQ(fb.GetPixel(1, 1), 0);
  EXPECT_EQ(fb.GetPixel(2, 1), 0);
  EXPECT_EQ(fb.GetPixel(3, 1), indigo);
  EXPECT_EQ(fb.GetPixel(4, 1), 0);

  EXPECT_EQ(fb.GetPixel(0, 2), indigo);
  EXPECT_EQ(fb.GetPixel(1, 2), 0);
  EXPECT_EQ(fb.GetPixel(2, 2), 0);
  EXPECT_EQ(fb.GetPixel(3, 2), indigo);
  EXPECT_EQ(fb.GetPixel(4, 2), 0);

  EXPECT_EQ(fb.GetPixel(0, 3), indigo);
  EXPECT_EQ(fb.GetPixel(1, 3), indigo);
  EXPECT_EQ(fb.GetPixel(2, 3), indigo);
  EXPECT_EQ(fb.GetPixel(3, 3), indigo);
  EXPECT_EQ(fb.GetPixel(4, 3), 0);

  EXPECT_EQ(fb.GetPixel(0, 4), 0);
  EXPECT_EQ(fb.GetPixel(1, 4), 0);
  EXPECT_EQ(fb.GetPixel(2, 4), 0);
  EXPECT_EQ(fb.GetPixel(3, 4), 0);
  EXPECT_EQ(fb.GetPixel(4, 4), 0);
}

TEST(DrawCircle, Empty) {
  uint16_t data[7 * 7];
  FramebufferRgb565 fb(data, 7, 7);
  color_rgb565_t indigo = colors_pico8_rgb565[12];
  fb.Fill(0);

  DrawCircle(&fb, 3, 3, 3, indigo, false);

  PrintFramebufferAsANSI(&fb);

  // ..xxx..
  // .x...x.
  // x.....x
  // x.....x
  // x.....x
  // .x...x.
  // ..xxx..

  EXPECT_EQ(fb.GetPixel(0, 0), 0);
  EXPECT_EQ(fb.GetPixel(1, 0), 0);
  EXPECT_EQ(fb.GetPixel(2, 0), indigo);
  EXPECT_EQ(fb.GetPixel(3, 0), indigo);
  EXPECT_EQ(fb.GetPixel(4, 0), indigo);
  EXPECT_EQ(fb.GetPixel(5, 0), 0);
  EXPECT_EQ(fb.GetPixel(6, 0), 0);

  EXPECT_EQ(fb.GetPixel(0, 1), 0);
  EXPECT_EQ(fb.GetPixel(1, 1), indigo);
  EXPECT_EQ(fb.GetPixel(2, 1), 0);
  EXPECT_EQ(fb.GetPixel(3, 1), 0);
  EXPECT_EQ(fb.GetPixel(4, 1), 0);
  EXPECT_EQ(fb.GetPixel(5, 1), indigo);
  EXPECT_EQ(fb.GetPixel(6, 1), 0);

  EXPECT_EQ(fb.GetPixel(0, 2), indigo);
  EXPECT_EQ(fb.GetPixel(1, 2), 0);
  EXPECT_EQ(fb.GetPixel(2, 2), 0);
  EXPECT_EQ(fb.GetPixel(3, 2), 0);
  EXPECT_EQ(fb.GetPixel(4, 2), 0);
  EXPECT_EQ(fb.GetPixel(5, 2), 0);
  EXPECT_EQ(fb.GetPixel(6, 2), indigo);

  EXPECT_EQ(fb.GetPixel(0, 3), indigo);
  EXPECT_EQ(fb.GetPixel(1, 3), 0);
  EXPECT_EQ(fb.GetPixel(2, 3), 0);
  EXPECT_EQ(fb.GetPixel(3, 3), 0);
  EXPECT_EQ(fb.GetPixel(4, 3), 0);
  EXPECT_EQ(fb.GetPixel(5, 3), 0);
  EXPECT_EQ(fb.GetPixel(6, 3), indigo);

  EXPECT_EQ(fb.GetPixel(0, 4), indigo);
  EXPECT_EQ(fb.GetPixel(1, 4), 0);
  EXPECT_EQ(fb.GetPixel(2, 4), 0);
  EXPECT_EQ(fb.GetPixel(3, 4), 0);
  EXPECT_EQ(fb.GetPixel(4, 4), 0);
  EXPECT_EQ(fb.GetPixel(5, 4), 0);
  EXPECT_EQ(fb.GetPixel(6, 4), indigo);

  EXPECT_EQ(fb.GetPixel(0, 5), 0);
  EXPECT_EQ(fb.GetPixel(1, 5), indigo);
  EXPECT_EQ(fb.GetPixel(2, 5), 0);
  EXPECT_EQ(fb.GetPixel(3, 5), 0);
  EXPECT_EQ(fb.GetPixel(4, 5), 0);
  EXPECT_EQ(fb.GetPixel(5, 5), indigo);
  EXPECT_EQ(fb.GetPixel(6, 5), 0);

  EXPECT_EQ(fb.GetPixel(0, 6), 0);
  EXPECT_EQ(fb.GetPixel(1, 6), 0);
  EXPECT_EQ(fb.GetPixel(2, 6), indigo);
  EXPECT_EQ(fb.GetPixel(3, 6), indigo);
  EXPECT_EQ(fb.GetPixel(4, 6), indigo);
  EXPECT_EQ(fb.GetPixel(5, 6), 0);
  EXPECT_EQ(fb.GetPixel(6, 6), 0);
}
}  // namespace
}  // namespace pw::draw
