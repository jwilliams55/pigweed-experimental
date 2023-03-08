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

#include "pw_draw/text_area.h"

#include "pw_color/color.h"
#include "pw_draw/draw.h"
#include "pw_draw/font_set.h"
#include "pw_framebuffer/framebuffer.h"

using pw::color::color_rgb565_t;
using pw::math::Vector2;

namespace pw::draw {

TextArea::TextArea(pw::framebuffer::Framebuffer& fb, const FontSet* font)
    : framebuffer(fb) {
  SetFont(font);
  // Default colors: White on Black
  character_wrap_enabled = true;
  foreground_color = 0xFFFF;
  background_color = 0;
  SetCursor(0, 0);
}

// Change the current font.
void TextArea::SetFont(const FontSet* new_font) { current_font = new_font; }

void TextArea::SetCursor(int x, int y) {
  cursor_x = x;
  cursor_y = y;
  column_count = 0;
}

void TextArea::SetForegroundColor(color_rgb565_t color) {
  foreground_color = color;
}

void TextArea::SetBackgroundColor(color_rgb565_t color) {
  background_color = color;
}

void TextArea::SetCharacterWrap(bool new_setting) {
  character_wrap_enabled = new_setting;
}

void TextArea::MoveCursorRightOnce() {
  cursor_x = cursor_x + current_font->width;
  column_count++;
}

void TextArea::InsertLineBreak() {
  cursor_y = cursor_y + current_font->height;
  cursor_x = cursor_x - (column_count * current_font->width);
  column_count = 0;

  if (cursor_y >= framebuffer.size().height) {
    ScrollUp(1);
    cursor_y = cursor_y - current_font->height;
  }
}

void TextArea::DrawCharacter(int character) {
  if (character == '\n') {
    InsertLineBreak();
    return;
  }

  if ((int)character < current_font->starting_character ||
      (int)character > current_font->ending_character) {
    // Unprintable character
    MoveCursorRightOnce();
    return;
  }

  if (character_wrap_enabled &&
      (current_font->width + cursor_x) > framebuffer.size().width) {
    InsertLineBreak();
  }

  pw::draw::DrawCharacter(character,
                          Vector2<int>{cursor_x, cursor_y},
                          foreground_color,
                          background_color,
                          *current_font,
                          framebuffer);

  // Move cursor to the right by 1 glyph.
  MoveCursorRightOnce();
}

void TextArea::DrawCharacter(int character, int x, int y) {
  SetCursor(x, y);
  DrawCharacter(character);
}

void TextArea::DrawTestFontSheet(int character_column_width, int x, int y) {
  SetCursor(x, y);
  for (int c = current_font->starting_character;
       c <= current_font->ending_character;
       c++) {
    int index = c - current_font->starting_character;
    if (index > 0 && index % character_column_width == 0) {
      DrawCharacter('\n');
    }
    DrawCharacter(c);
  }
}

// DrawText at x, y (upper left pixel of font). Carriage returns will move
// text to the next line.
void TextArea::DrawText(const char* str) {
  for (const char* ch = str; *ch != '\0'; ch++) {
    DrawCharacter(*ch);
  }
}

void TextArea::DrawText(const char* str, int x, int y) {
  SetCursor(x, y);
  DrawText(str);
}

void TextArea::DrawText(const wchar_t* str) {
  for (const wchar_t* ch = str; *ch != L'\0'; ch++) {
    DrawCharacter(*ch);
  }
}

void TextArea::DrawText(const wchar_t* str, int x, int y) {
  SetCursor(x, y);
  DrawText(str);
}

void TextArea::ScrollUp(int lines) {
  int pixel_height = lines * current_font->height;
  int start_x = 0;
  int start_y = pixel_height;

  for (int current_x = 0; current_x < framebuffer.size().width; current_x++) {
    for (int current_y = start_y; current_y < framebuffer.size().height;
         current_y++) {
      if (auto pixel_color = framebuffer.GetPixel(current_x, current_y);
          pixel_color.ok()) {
        framebuffer.SetPixel(
            start_x + current_x, current_y - start_y, *pixel_color);
      }
    }
  }

  // Draw a filled background_color rectangle at the bottom to erase the old
  // text.
  for (int x = 0; x < framebuffer.size().width; x++) {
    for (int y = framebuffer.size().height - pixel_height;
         y < framebuffer.size().height;
         y++) {
      framebuffer.SetPixel(x, y, background_color);
    }
  }
}

}  // namespace pw::draw
