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
#include "pw_framebuffer/rgb565.h"

namespace pw::draw {

TextArea::TextArea(pw::framebuffer::FramebufferRgb565* fb,
                   const FontSet* font) {
  framebuffer = fb;
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

void TextArea::SetBackgroundTransparent() {
  background_color = framebuffer->GetTransparentColor();
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

  if (cursor_y >= framebuffer->GetHeight()) {
    ScrollUp(1);
    cursor_y = cursor_y - current_font->height;
  }
}

void TextArea::DrawSpace() {
  for (int font_row = 0; font_row < current_font->height; font_row++) {
    for (int font_column = 0; font_column < current_font->width;
         font_column++) {
      if (background_color != framebuffer->GetTransparentColor()) {
        framebuffer->SetPixel(
            cursor_x + font_column, cursor_y + font_row, background_color);
      }
    }
  }
}

void TextArea::DrawCharacter(int character) {
  if (character == '\n') {
    InsertLineBreak();
    return;
  }

  if (character == ' ') {
    DrawSpace();
    MoveCursorRightOnce();
    return;
  }

  if ((int)character < current_font->starting_character ||
      (int)character > current_font->ending_character) {
    // Unprintable character
    MoveCursorRightOnce();
    return;
  }

  if (character_wrap_enabled &&
      (current_font->width + cursor_x) > framebuffer->GetWidth()) {
    InsertLineBreak();
  }

  int character_index = (int)character - current_font->starting_character;
  uint8_t pixel_on;

  for (int font_row = 0; font_row < current_font->height; font_row++) {
    for (int font_column = 0; font_column < current_font->width;
         font_column++) {
      pixel_on = PW_FONT_BIT(
          current_font->width - font_column - 1,
          current_font
              ->data[current_font->height * character_index + font_row]);
      if (pixel_on) {
        framebuffer->SetPixel(
            cursor_x + font_column, cursor_y + font_row, foreground_color);
      } else if (background_color != framebuffer->GetTransparentColor()) {
        framebuffer->SetPixel(
            cursor_x + font_column, cursor_y + font_row, background_color);
      }
    }
  }
  // Move cursor to the right by 1 glyph.
  MoveCursorRightOnce();
}

void TextArea::DrawCharacter(int character, int x, int y) {
  SetCursor(x, y);
  DrawCharacter(character);
}

void TextArea::DrawTestFontSheet(int character_width, int x, int y) {
  SetCursor(x, y);
  for (int c = current_font->starting_character;
       c <= current_font->ending_character;
       c++) {
    if (c % character_width == 0) {
      DrawCharacter('\n');
    }
    DrawCharacter(c);
  }
}

// DrawText at x, y (upper left pixel of font). Carriage returns will move
// text to the next line.
void TextArea::DrawText(const char* str) {
  int character_index = 0;
  int character;
  do {
    character = str[character_index];
    DrawCharacter(character);
    character_index++;
  } while (character != '\0');
}

void TextArea::DrawText(const char* str, int x, int y) {
  SetCursor(x, y);
  DrawText(str);
}

void TextArea::DrawText(const wchar_t* str) {
  int character_index = 0;
  int character;
  do {
    character = str[character_index];
    DrawCharacter(character);
    character_index++;
  } while (character != '\0');
}

void TextArea::DrawText(const wchar_t* str, int x, int y) {
  SetCursor(x, y);
  DrawText(str);
}

void TextArea::ScrollUp(int lines) {
  int pixel_height = lines * current_font->height;
  int start_x = 0;
  int start_y = pixel_height;

  color_rgb565_t pixel_color;
  for (int current_x = 0; current_x < framebuffer->GetWidth(); current_x++) {
    for (int current_y = start_y; current_y < framebuffer->GetHeight();
         current_y++) {
      pixel_color = framebuffer->GetPixel(current_x, current_y);
      framebuffer->SetPixel(
          start_x + current_x, current_y - start_y, pixel_color);
    }
  }

  // Draw a filled background_color rectangle at the bottom to erase the old
  // text.
  for (int x = 0; x < framebuffer->GetWidth(); x++) {
    for (int y = framebuffer->GetHeight() - pixel_height;
         y < framebuffer->GetHeight();
         y++) {
      framebuffer->SetPixel(x, y, background_color);
    }
  }
}

}  // namespace pw::draw
