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

#include "text_buffer.h"

#include "gtest/gtest.h"
#include "pw_color/colors_pico8.h"

using pw::color::color_rgb565_t;

namespace {
constexpr color_rgb565_t kIndigo = pw::color::colors_pico8_rgb565[13];
constexpr color_rgb565_t kDarkGreen = pw::color::colors_pico8_rgb565[3];
}  // namespace

TEST(TextBufferTest, DimsAsExpected) {
  TextBuffer buffer;
  ASSERT_EQ(kNumCharsWide, static_cast<size_t>(buffer.GetSize().width));
  ASSERT_EQ(kNumRows, static_cast<size_t>(buffer.GetSize().height));
}

TEST(TextBufferTest, ClearedOnConstruction) {
  TextBuffer buffer;

  const auto buffer_size = buffer.GetSize();
  for (int r = 0; r < buffer_size.height; r++) {
    for (int c = 0; c < buffer_size.width; c++) {
      const auto ch = buffer.GetChar({c, r});
      ASSERT_TRUE(ch.ok());
      ASSERT_EQ('\0', ch->ch);
      ASSERT_EQ(kWhiteColor, ch->foreground_color);
      ASSERT_EQ(kBlackColor, ch->background_color);
    }
  }
}

TEST(TextBufferTest, OutOfBoundsColumnNotOk) {
  TextBuffer buffer;
  auto ch = buffer.GetChar({buffer.GetSize().width, 0});
  EXPECT_FALSE(ch.ok());

  ch = buffer.GetChar({-1, 0});
  EXPECT_FALSE(ch.ok());
}

TEST(TextBufferTest, OutOfBoundsRowNotOk) {
  TextBuffer buffer;
  auto ch = buffer.GetChar({0, buffer.GetSize().height});
  EXPECT_FALSE(ch.ok());

  ch = buffer.GetChar({0, -1});
  EXPECT_FALSE(ch.ok());
}

TEST(TextBufferTest, SimpleInsert) {
  TextBuffer buffer;

  buffer.DrawCharacter({'A', kIndigo, kDarkGreen});
  auto ch = buffer.GetChar({0, 0});
  ASSERT_TRUE(ch.ok());
  EXPECT_EQ('A', ch->ch);
  EXPECT_EQ(kIndigo, ch->foreground_color);
  EXPECT_EQ(kDarkGreen, ch->background_color);

  buffer.DrawCharacter({'B', kIndigo, kDarkGreen});
  ch = buffer.GetChar({0, 0});
  ASSERT_TRUE(ch.ok());
  EXPECT_EQ('A', ch->ch);
  EXPECT_EQ(kIndigo, ch->foreground_color);
  EXPECT_EQ(kDarkGreen, ch->background_color);

  ch = buffer.GetChar({1, 0});
  ASSERT_TRUE(ch.ok());
  EXPECT_EQ('B', ch->ch);
  EXPECT_EQ(kIndigo, ch->foreground_color);
  EXPECT_EQ(kDarkGreen, ch->background_color);
}

TEST(TextBufferTest, NewLineInsertsToNextRow) {
  TextBuffer buffer;
  buffer.DrawCharacter({'A', kIndigo, kDarkGreen});
  buffer.DrawCharacter({'\n', kIndigo, kDarkGreen});
  buffer.DrawCharacter({'B', kIndigo, kDarkGreen});
  buffer.DrawCharacter({'\n', kIndigo, kDarkGreen});

  auto ch = buffer.GetChar({1, 0});
  ASSERT_TRUE(ch.ok());
  EXPECT_EQ('\0', ch->ch);
  EXPECT_EQ(kWhiteColor, ch->foreground_color);
  EXPECT_EQ(kBlackColor, ch->background_color);

  ch = buffer.GetChar({0, 1});
  ASSERT_TRUE(ch.ok());
  EXPECT_EQ('B', ch->ch);
  EXPECT_EQ(kIndigo, ch->foreground_color);
  EXPECT_EQ(kDarkGreen, ch->background_color);
}

TEST(TextBufferTest, Scroll) {
  // Insert enough newlines to scroll buffer.
  TextBuffer buffer;
  char next_char = 'A';
  for (size_t i = 0; i < kNumRows; i++) {
    buffer.DrawCharacter({next_char, kIndigo, kDarkGreen});
    buffer.DrawCharacter({'\n', kIndigo, kDarkGreen});
    next_char++;
  }

  // Verify the buffer has scrolled up once,
  auto ch = buffer.GetChar({0, 0});
  ASSERT_TRUE(ch.ok());
  EXPECT_EQ('B', ch->ch);
  EXPECT_EQ(kIndigo, ch->foreground_color);
  EXPECT_EQ(kDarkGreen, ch->background_color);
}
