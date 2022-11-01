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
#pragma once

#include <array>

#include "pw_color/color.h"
#include "pw_coordinates/vec_int.h"
#include "pw_result/result.h"

constexpr size_t kNumCharsWide = 52;
constexpr size_t kNumRows = 9;

constexpr pw::color::color_rgb565_t kBlackColor = 0x0000;
constexpr pw::color::color_rgb565_t kWhiteColor = 0xffff;

// Maintain a scrolling buffer of text. The "drawing" cursor location starts
// at (0,0), and new characters are inserted right-to-left. Newline ('\n')
// characters cause the text to be scrolled up - eventually rolling off the
// top of the buffer to make space for new text rows at the bottom.
class TextBuffer {
 public:
  // An ASCII character with a foreground and background color.
  struct Char {
    // Set to a cleared default state.
    void Reset() {
      ch = '\0';
      foreground_color = kWhiteColor;
      background_color = kBlackColor;
    }

    char ch = '\0';
    pw::color::color_rgb565_t foreground_color = kWhiteColor;
    pw::color::color_rgb565_t background_color = kBlackColor;
  };

  // A row of ASCII text characters.
  struct TextRow {
    void Clear() {
      for (Char& ch : chars) {
        ch.Reset();
      }
    }
    std::array<Char, kNumCharsWide> chars;
  };

  // Insert a character at the current cursor location. The cursor will be
  // moved right by one slot. Newline ('\n') characters will move the cursor to
  // the next line, at column 0.
  void DrawCharacter(const Char& ch);

  // Return the size, in characters, of the text buffer.
  pw::coordinates::Size<int> GetSize() const {
    return pw::coordinates::Size<int>{kNumCharsWide, kNumRows};
  }

  // Return the character at the specified location.
  pw::Result<Char> GetChar(pw::coordinates::Vector2<int> loc) const;

 private:
  void ScrollUp();
  void InsertNewline();

  pw::coordinates::Vector2<int> cursor_ = {0, 0};
  bool character_wrap_enabled_ = false;
  std::array<TextRow, kNumRows> text_rows_;
};
