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

#include "pw_color/color.h"

namespace pw::draw {

// Grab the x'th bit from a number
#define PW_FONT_BIT(x, number) (((number) >> (x)) & 1);

struct FontSet {
  constexpr FontSet(
      const uint8_t* d, uint8_t w, uint8_t h, int start_char, int end_char)
      : data(d),
        width(w),
        height(h),
        starting_character(start_char),
        ending_character(end_char) {}

  const uint8_t* data;
  const uint8_t width;
  const uint8_t height;
  const int starting_character;
  const int ending_character;
};

extern const FontSet font6x8;
extern const FontSet font6x8_box_chars;

}  // namespace pw::draw
