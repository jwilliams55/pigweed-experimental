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
#include "pw_draw/font_set.h"
#include "pw_draw/text_area.h"
#include "pw_framebuffer/framebuffer.h"
#include "pw_framebuffer/writer.h"
#include "pw_log/log.h"
#include "pw_string/string_builder.h"

using namespace pw::draw;
using namespace pw::framebuffer;
using pw::color::color_rgb565_t;

namespace pw::draw {
namespace {

constexpr color_rgb565_t kBlack = 0x0;

void PrintFramebufferAsANSI(const Framebuffer& fb) {
  pw::StringBuffer<4096> line;
  pw::StringBuffer<128> color_string;
  FramebufferReader reader(fb);

  for (int y = 0; y < fb.size().height; y += 2) {
    line.clear();

    for (int x = 0; x < fb.size().width; x++) {
      color_string.clear();
      auto row1_color = reader.GetPixel(x, y);
      pw::color::ColorRGBA row1(row1_color.ok() ? row1_color.value() : kBlack);
      auto row2_color = reader.GetPixel(x, y + 1);
      if (!row2_color.ok()) {
        color_string.Format("[m[38;2;%d;%d;%dm▀", row1.r, row1.g, row1.b);
      } else {
        pw::color::ColorRGBA row2(row2_color.value());
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
  Framebuffer fb(data, {4, 4}, 4 * sizeof(data[0]));
  FramebufferWriter writer(fb);
  color_rgb565_t indigo = color::colors_pico8_rgb565[12];

  writer.Fill(0);

  DrawLine(fb, 0, 0, fb.size().width, fb.size().height, indigo);
  PrintFramebufferAsANSI(fb);

  // Check the diagonal is set to indigo, everything else should be 0.
  Result<color_rgb565_t> c;
  for (int x = 0; x < fb.size().width; x++) {
    for (int y = 0; y < fb.size().height; y++) {
      c = writer.GetPixel(x, y);
      ASSERT_TRUE(c.ok());
      if (x == y) {
        EXPECT_EQ(c.value(), indigo);
      } else {
        EXPECT_EQ(c.value(), 0);
      }
    }
  }
}

TEST(DrawHLine, Top) {
  uint16_t data[4 * 4];
  Framebuffer fb(data, {4, 4}, 4 * sizeof(data[0]));
  FramebufferWriter writer(fb);
  color_rgb565_t indigo = color::colors_pico8_rgb565[12];
  writer.Fill(0);

  // Horizonal line at y = 0
  DrawHLine(fb, 0, fb.size().width, 0, indigo);
  PrintFramebufferAsANSI(fb);

  // Check color at y = 0 is indigo, y = 1 should be 0.
  Result<color_rgb565_t> c;
  for (int x = 0; x < fb.size().width; x++) {
    c = writer.GetPixel(x, 0);
    ASSERT_TRUE(c.ok());
    EXPECT_EQ(c.value(), indigo);
    c = writer.GetPixel(x, 1);
    ASSERT_TRUE(c.ok());
    EXPECT_EQ(c.value(), 0);
  }
}

TEST(DrawRect, Empty) {
  uint16_t data[5 * 5];
  Framebuffer fb(data, {5, 5}, 5 * sizeof(data[0]));
  FramebufferWriter writer(fb);
  color_rgb565_t indigo = color::colors_pico8_rgb565[12];
  writer.Fill(0);

  // 4x4 rectangle, not filled
  DrawRect(fb, 0, 0, 3, 3, indigo, false);
  PrintFramebufferAsANSI(fb);

  Result<color_rgb565_t> c;
  c = writer.GetPixel(0, 0);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), indigo);
  c = writer.GetPixel(1, 0);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), indigo);
  c = writer.GetPixel(2, 0);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), indigo);
  c = writer.GetPixel(3, 0);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), indigo);
  c = writer.GetPixel(4, 0);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), 0);

  c = writer.GetPixel(0, 1);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), indigo);
  c = writer.GetPixel(1, 1);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), 0);
  c = writer.GetPixel(2, 1);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), 0);
  c = writer.GetPixel(3, 1);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), indigo);
  c = writer.GetPixel(4, 1);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), 0);

  c = writer.GetPixel(0, 2);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), indigo);
  c = writer.GetPixel(1, 2);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), 0);
  c = writer.GetPixel(2, 2);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), 0);
  c = writer.GetPixel(3, 2);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), indigo);
  c = writer.GetPixel(4, 2);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), 0);

  c = writer.GetPixel(0, 3);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), indigo);
  c = writer.GetPixel(1, 3);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), indigo);
  c = writer.GetPixel(2, 3);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), indigo);
  c = writer.GetPixel(3, 3);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), indigo);
  c = writer.GetPixel(4, 3);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), 0);

  c = writer.GetPixel(0, 4);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), 0);
  c = writer.GetPixel(1, 4);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), 0);
  c = writer.GetPixel(2, 4);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), 0);
  c = writer.GetPixel(3, 4);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), 0);
  c = writer.GetPixel(4, 4);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), 0);
}

TEST(DrawRect, Filled) {
  uint16_t data[5 * 5];
  Framebuffer fb(data, {5, 5}, 5 * sizeof(data[0]));
  FramebufferWriter writer(fb);
  color_rgb565_t indigo = color::colors_pico8_rgb565[12];
  writer.Fill(0);

  // 4x4 rectangle, filled
  DrawRect(fb, 0, 0, 3, 3, indigo, true);
  PrintFramebufferAsANSI(fb);

  Result<color_rgb565_t> c;
  c = writer.GetPixel(0, 0);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), indigo);
  c = writer.GetPixel(1, 0);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), indigo);
  c = writer.GetPixel(2, 0);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), indigo);
  c = writer.GetPixel(3, 0);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), indigo);
  c = writer.GetPixel(4, 0);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), 0);

  c = writer.GetPixel(0, 1);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), indigo);
  c = writer.GetPixel(1, 1);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), indigo);
  c = writer.GetPixel(2, 1);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), indigo);
  c = writer.GetPixel(3, 1);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), indigo);
  c = writer.GetPixel(4, 1);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), 0);

  c = writer.GetPixel(0, 2);
  EXPECT_EQ(c.value(), indigo);
  c = writer.GetPixel(1, 2);
  EXPECT_EQ(c.value(), indigo);
  c = writer.GetPixel(2, 2);
  EXPECT_EQ(c.value(), indigo);
  c = writer.GetPixel(3, 2);
  EXPECT_EQ(c.value(), indigo);
  c = writer.GetPixel(4, 2);
  EXPECT_EQ(c.value(), 0);

  c = writer.GetPixel(0, 3);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), indigo);
  c = writer.GetPixel(1, 3);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), indigo);
  c = writer.GetPixel(2, 3);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), indigo);
  c = writer.GetPixel(3, 3);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), indigo);
  c = writer.GetPixel(4, 3);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), 0);

  c = writer.GetPixel(0, 4);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), 0);
  c = writer.GetPixel(1, 4);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), 0);
  c = writer.GetPixel(2, 4);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), 0);
  c = writer.GetPixel(3, 4);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), 0);
  c = writer.GetPixel(4, 4);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), 0);
}

TEST(DrawRectWH, WidthHeightCorrect) {
  uint16_t data[5 * 5];
  Framebuffer fb(data, {5, 5}, 5 * sizeof(data[0]));
  FramebufferWriter writer(fb);
  color_rgb565_t indigo = color::colors_pico8_rgb565[12];
  writer.Fill(0);

  // 4x4 rectangle, not filled
  DrawRectWH(fb, 0, 0, 4, 4, indigo, false);
  PrintFramebufferAsANSI(fb);

  Result<color_rgb565_t> c;
  c = writer.GetPixel(0, 0);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), indigo);
  c = writer.GetPixel(1, 0);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), indigo);
  c = writer.GetPixel(2, 0);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), indigo);
  c = writer.GetPixel(3, 0);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), indigo);
  c = writer.GetPixel(4, 0);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), 0);

  c = writer.GetPixel(0, 1);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), indigo);
  c = writer.GetPixel(1, 1);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), 0);
  c = writer.GetPixel(2, 1);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), 0);
  c = writer.GetPixel(3, 1);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), indigo);
  c = writer.GetPixel(4, 1);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), 0);

  c = writer.GetPixel(0, 2);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), indigo);
  c = writer.GetPixel(1, 2);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), 0);
  c = writer.GetPixel(2, 2);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), 0);
  c = writer.GetPixel(3, 2);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), indigo);
  c = writer.GetPixel(4, 2);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), 0);

  c = writer.GetPixel(0, 3);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), indigo);
  c = writer.GetPixel(1, 3);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), indigo);
  c = writer.GetPixel(2, 3);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), indigo);
  c = writer.GetPixel(3, 3);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), indigo);
  c = writer.GetPixel(4, 3);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), 0);

  c = writer.GetPixel(0, 4);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), 0);
  c = writer.GetPixel(1, 4);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), 0);
  c = writer.GetPixel(2, 4);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), 0);
  c = writer.GetPixel(3, 4);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), 0);
  c = writer.GetPixel(4, 4);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), 0);
}

TEST(DrawCircle, Empty) {
  uint16_t data[7 * 7];
  Framebuffer fb(data, {7, 7}, 7 * sizeof(data[0]));
  FramebufferWriter writer(fb);
  color_rgb565_t indigo = color::colors_pico8_rgb565[12];
  writer.Fill(0);

  DrawCircle(fb, 3, 3, 3, indigo, false);

  PrintFramebufferAsANSI(fb);

  // ..xxx..
  // .x...x.
  // x.....x
  // x.....x
  // x.....x
  // .x...x.
  // ..xxx..

  Result<color_rgb565_t> c;
  c = writer.GetPixel(0, 0);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), 0);
  c = writer.GetPixel(1, 0);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), 0);
  c = writer.GetPixel(2, 0);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), indigo);
  c = writer.GetPixel(3, 0);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), indigo);
  c = writer.GetPixel(4, 0);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), indigo);
  c = writer.GetPixel(5, 0);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), 0);
  c = writer.GetPixel(6, 0);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), 0);

  c = writer.GetPixel(0, 1);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), 0);
  c = writer.GetPixel(1, 1);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), indigo);
  c = writer.GetPixel(2, 1);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), 0);
  c = writer.GetPixel(3, 1);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), 0);
  c = writer.GetPixel(4, 1);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), 0);
  c = writer.GetPixel(5, 1);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), indigo);
  c = writer.GetPixel(6, 1);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), 0);

  c = writer.GetPixel(0, 2);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), indigo);
  c = writer.GetPixel(1, 2);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), 0);
  c = writer.GetPixel(2, 2);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), 0);
  c = writer.GetPixel(3, 2);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), 0);
  c = writer.GetPixel(4, 2);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), 0);
  c = writer.GetPixel(5, 2);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), 0);
  c = writer.GetPixel(6, 2);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), indigo);

  c = writer.GetPixel(0, 3);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), indigo);
  c = writer.GetPixel(1, 3);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), 0);
  c = writer.GetPixel(2, 3);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), 0);
  c = writer.GetPixel(3, 3);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), 0);
  c = writer.GetPixel(4, 3);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), 0);
  c = writer.GetPixel(5, 3);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), 0);
  c = writer.GetPixel(6, 3);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), indigo);

  c = writer.GetPixel(0, 4);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), indigo);
  c = writer.GetPixel(1, 4);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), 0);
  c = writer.GetPixel(2, 4);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), 0);
  c = writer.GetPixel(3, 4);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), 0);
  c = writer.GetPixel(4, 4);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), 0);
  c = writer.GetPixel(5, 4);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), 0);
  c = writer.GetPixel(6, 4);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), indigo);

  c = writer.GetPixel(0, 5);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), 0);
  c = writer.GetPixel(1, 5);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), indigo);
  c = writer.GetPixel(2, 5);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), 0);
  c = writer.GetPixel(3, 5);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), 0);
  c = writer.GetPixel(4, 5);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), 0);
  c = writer.GetPixel(5, 5);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), indigo);
  c = writer.GetPixel(6, 5);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), 0);

  c = writer.GetPixel(0, 6);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), 0);
  c = writer.GetPixel(1, 6);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), 0);
  c = writer.GetPixel(2, 6);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), indigo);
  c = writer.GetPixel(3, 6);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), indigo);
  c = writer.GetPixel(4, 6);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), indigo);
  c = writer.GetPixel(5, 6);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), 0);
  c = writer.GetPixel(6, 6);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), 0);
}

TEST(DrawText, WithFgBg) {
  uint16_t data[(5 * 6) * (3 * 8)];
  Framebuffer fb(data, {5 * 6, 3 * 8}, (5 * 6) * sizeof(data[0]));
  FramebufferWriter writer(fb);
  writer.Fill(0);

  pw::draw::TextArea text_area(fb, &font6x8);
  text_area.SetForegroundColor(color::colors_pico8_rgb565[COLOR_PINK]);
  text_area.SetBackgroundColor(0);
  text_area.DrawText("Hell\noT\nhere.\nWorld");

  PrintFramebufferAsANSI(fb);
}

}  // namespace
}  // namespace pw::draw
