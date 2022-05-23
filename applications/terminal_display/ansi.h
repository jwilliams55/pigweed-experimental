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

#include <cctype>
#include <cstdint>

#include "pw_containers/vector.h"

class AnsiDecoder {
 public:
  AnsiDecoder()
      : state_(State::kNormal), cur_val_(0), params_(), buffered_chars_() {}

  void ProcessChar(char c) {
    switch (state_) {
      case State::kNormal:
        if (c == kEscapeChar) {
          buffered_chars_.push_back(c);
          state_ = State::kEscape;
        } else {
          EmitChar(c);
        }
        break;

      case State::kEscape:
        buffered_chars_.push_back(c);
        if (c == kCsiChar) {
          state_ = State::kCsi;
        } else {
          Error();
        }
        break;

      case State::kCsi:
        buffered_chars_.push_back(c);

        if (std::isdigit(c)) {
          cur_val_ *= 10;
          cur_val_ += static_cast<int>(c - '0');
        } else if (c == ';') {
          PushParam();
        } else {
          PushParam();
          HandleCsiCommand(c);
          state_ = State::kNormal;
        }

        break;
    }
  }

 protected:
  virtual void SetFgColor(uint8_t r, uint8_t g, uint8_t b) {}
  virtual void SetBgColor(uint8_t r, uint8_t g, uint8_t b) {}
  virtual void EmitChar(char c) {}

 private:
  static constexpr char kEscapeChar = '\e';
  static constexpr char kCsiChar = '[';

  enum State {
    kNormal,
    kEscape,
    kCsi,
  };

  State state_;
  int cur_val_;
  pw::Vector<int, 10> params_;
  pw::Vector<char, 10> buffered_chars_;

  void PushParam() {
    params_.push_back(cur_val_);
    cur_val_ = 0;
  }

  void HandleCsiCommand(char c) {
    switch (c) {
      case 'm':
        for (auto param : params_) {
          HandleSetColor(param);
        }
        break;
    }

    params_.clear();
  }

  struct Color {
    uint8_t r;
    uint8_t g;
    uint8_t b;
  };

  static constexpr Color kNormalColors[8] = {
      {.r = 0, .g = 0, .b = 0},        // Black
      {.r = 170, .g = 0, .b = 0},      // Red
      {.r = 0, .g = 170, .b = 0},      // Green
      {.r = 170, .g = 85, .b = 0},     // Yellow
      {.r = 0, .g = 0, .b = 170},      // Blue
      {.r = 170, .g = 0, .b = 170},    // Magenta
      {.r = 0, .g = 170, .b = 170},    // Cyan
      {.r = 170, .g = 170, .b = 170},  // White
  };

  static constexpr Color kBrightColors[8] = {
      {.r = 85, .g = 85, .b = 85},     // Black
      {.r = 255, .g = 85, .b = 85},    // Red
      {.r = 85, .g = 255, .b = 85},    // Green
      {.r = 255, .g = 255, .b = 85},   // Yellow
      {.r = 85, .g = 85, .b = 255},    // Blue
      {.r = 255, .g = 85, .b = 255},   // Magenta
      {.r = 85, .g = 255, .b = 255},   // Cyan
      {.r = 255, .g = 255, .b = 255},  // White
  };

  void HandleSetColor(int val) {
    if (val < 0) {
      return;
    }

    if (val == 0) {
      auto fg_color = kNormalColors[7];
      auto bg_color = kNormalColors[0];
      SetBgColor(bg_color.r, bg_color.g, bg_color.b);
      SetFgColor(fg_color.r, fg_color.g, fg_color.b);
    }

    auto color_index = val % 10;
    auto color_loc = val / 10;

    if (color_index > 7) {
      return;
    }

    switch (color_loc) {
      case 3: {
        auto color = kNormalColors[color_index];
        SetFgColor(color.r, color.g, color.b);
        break;
      }

      case 4: {
        auto color = kNormalColors[color_index];
        SetBgColor(color.r, color.g, color.b);
        break;
      }

      case 9: {
        auto color = kBrightColors[color_index];
        SetFgColor(color.r, color.g, color.b);
        break;
      }

      case 10: {
        auto color = kBrightColors[color_index];
        SetBgColor(color.r, color.g, color.b);
        break;
      }

      default:
        break;
    }
  }

  void Error() {
    for (auto c : buffered_chars_) {
      EmitChar(c);
    }
    buffered_chars_.clear();
  }
};