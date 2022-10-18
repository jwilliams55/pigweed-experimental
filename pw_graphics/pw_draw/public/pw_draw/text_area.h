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
#include "pw_draw/font_set.h"
#include "pw_framebuffer/rgb565.h"

namespace pw::draw {

class TextArea {
 public:
  int cursor_x;
  int cursor_y;
  int column_count;
  bool character_wrap_enabled;
  const FontSet* current_font;
  pw::color::color_rgb565_t foreground_color;
  pw::color::color_rgb565_t background_color;
  pw::framebuffer::FramebufferRgb565* framebuffer;

  TextArea(pw::framebuffer::FramebufferRgb565* fb, const FontSet* font);

  // Change the current font.
  void SetFont(const FontSet* new_font);
  void SetCharacterWrap(bool new_setting);
  void SetCursor(int x, int y);
  void ScrollUp(int lines);

  void DrawCharacter(int character);
  void DrawCharacter(int character, int x, int y);

  void SetForegroundColor(pw::color::color_rgb565_t color);
  void SetBackgroundColor(pw::color::color_rgb565_t color);

  void DrawTestFontSheet(int character_width, int x, int y);

  // DrawText at x, y (upper left pixel of font). Carriage returns will move
  // text to the next line.
  void DrawText(const char* str);
  void DrawText(const char* str, int x, int y);

  void DrawText(const wchar_t* str);
  void DrawText(const wchar_t* str, int x, int y);

  void MoveCursorRightOnce();
  void DrawSpace();
  void InsertLineBreak();
};

}  // namespace pw::draw
