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

#include "pw_framebuffer/reader.h"

#include <cstdint>

#include "gtest/gtest.h"
#include "pw_color/color.h"
#include "pw_color/colors_endesga32.h"
#include "pw_color/colors_pico8.h"
#include "pw_framebuffer/framebuffer.h"
#include "pw_framebuffer/reader.h"
#include "pw_framebuffer/writer.h"

using pw::color::color_rgb565_t;

namespace pw::framebuffer {
namespace {

TEST(FramebufferReader, SetPixelGetPixel) {
  color_rgb565_t data[8 * 8];
  Framebuffer fb(data, PixelFormat::RGB565, {8, 8}, 8 * sizeof(data[0]));
  const color_rgb565_t* pixel_data =
      static_cast<const color_rgb565_t*>(fb.data());
  color_rgb565_t indigo = 0x83b3;
  {
    FramebufferWriter writer(fb);
    writer.Fill(0);
    for (uint16_t i = 0; i < 8; i++) {
      writer.SetPixel(i, i, indigo);
    }
  }

  FramebufferReader reader(fb);
  EXPECT_EQ(pixel_data[0], indigo);
  EXPECT_EQ(pixel_data[1], 0);
  EXPECT_EQ(pixel_data[8 * 8 - 2], 0);
  EXPECT_EQ(pixel_data[8 * 8 - 1], indigo);

  Result<color_rgb565_t> c;
  c = reader.GetPixel(0, 0);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), indigo);
  c = reader.GetPixel(0, 1);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), 0);
  c = reader.GetPixel(6, 7);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), 0);
  c = reader.GetPixel(7, 7);
  ASSERT_TRUE(c.ok());
  EXPECT_EQ(c.value(), indigo);
}

}  // namespace
}  // namespace pw::framebuffer
