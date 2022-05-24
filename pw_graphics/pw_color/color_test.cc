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

#include "pw_color/color.h"

#include "gtest/gtest.h"
#include "pw_color/colors_endesga32.h"
#include "pw_color/colors_pico8.h"
#include "pw_log/log.h"

namespace pw::color {
namespace {

TEST(ColorsPico8Rgb565, Exists) { EXPECT_EQ(colors_pico8_rgb565[1], 0x194a); }

TEST(ColorsEndesga32Rgb565, Exists) {
  EXPECT_EQ(colors_endesga32_rgb565[1], 0xd3a8);
}

TEST(ColorToRGB565, FromRGB) {
  EXPECT_EQ(ColorRGBA(0x1d, 0x2b, 0x53).ToRgb565(), colors_pico8_rgb565[1]);
}

TEST(ColorToRGB565, FromRGBA) {
  // color_rgba8888_t dark_blue = 0xff532b1d;  // A B G R
  EXPECT_EQ(ColorRGBA(colors_pico8_rgba8888[1]).ToRgb565(),
            colors_pico8_rgb565[1]);
}

TEST(SplitColor, FromRGBA888) {
  // color_rgba8888_t dark_blue = 0xff532b1d;  // A B G R
  ColorRGBA color(colors_pico8_rgba8888[13]);
  EXPECT_EQ(color.a, 0xff);
  EXPECT_EQ(color.r, 0x83);
  EXPECT_EQ(color.g, 0x76);
  EXPECT_EQ(color.b, 0x9c);
}

TEST(SplitColor, FromRGB565) {
  // color_rgba8888_t dark_blue = 0xff532b1d;  // A B G R
  ColorRGBA color(colors_pico8_rgb565[13]);
  EXPECT_EQ(color.a, 0xff);
  EXPECT_EQ(color.r, 0x84);  // Slightly off
  EXPECT_EQ(color.g, 0x75);  // Slightly off
  EXPECT_EQ(color.b, 0x9c);
}

}  // namespace
}  // namespace pw::color
