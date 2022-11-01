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

using pw::color::color_rgb565_t;
using pw::coordinates::Vector2;

namespace {
constexpr int kMaxColIdx = kNumCharsWide - 1;
constexpr int kMaxRowIdx = kNumRows - 1;

static_assert(kNumRows > 1, "Text buffer too small");
}  // namespace

void TextBuffer::InsertNewline() {
  if (cursor_.y == kMaxRowIdx) {
    ScrollUp();
  } else {
    cursor_.y++;
  }
  cursor_.x = 0;
}

pw::Result<TextBuffer::Char> TextBuffer::GetChar(Vector2<int> loc) const {
  if (loc.x < 0 || static_cast<size_t>(loc.x) > kMaxColIdx || loc.y < 0 ||
      static_cast<size_t>(loc.y) > kMaxRowIdx) {
    return pw::Status::OutOfRange();
  }
  return text_rows_[loc.y].chars[loc.x];
}

void TextBuffer::DrawCharacter(const Char& ch) {
  if (ch.ch == '\n') {
    InsertNewline();
    cursor_.x = 0;
    return;
  }

  if (character_wrap_enabled_ && cursor_.x > kMaxColIdx) {
    InsertNewline();
  }

  if (cursor_.x > kMaxColIdx) {
    // The current line has grown too long.
    return;
  }

  PW_ASSERT(cursor_.x >= 0 && cursor_.y >= 0);
  PW_ASSERT(cursor_.y <= kMaxRowIdx);
  PW_ASSERT(cursor_.x <= kMaxColIdx);

  text_rows_[cursor_.y].chars[cursor_.x] = ch;

  cursor_.x++;
}

void TextBuffer::ScrollUp() {
  for (size_t r = 0; r < kMaxRowIdx; r++) {
    text_rows_[r] = text_rows_[r + 1];
  }
  text_rows_.back().Clear();
}
