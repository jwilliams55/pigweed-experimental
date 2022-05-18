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

#include "pw_draw/draw.h"

#include <math.h>

#include "pw_color/color.h"
#include "pw_draw/font_set.h"
#include "pw_draw/sprite_sheet.h"
#include "pw_framebuffer/rgb565.h"

namespace pw::draw {

void DrawLine(
    pw::framebuffer::FramebufferRgb565* fb, int x1, int y1, int x2, int y2) {
  // Bresenham's Line Algorithm
  int16_t steep_gradient = abs(y2 - y1) > abs(x2 - x1);
  // Swap values
  int16_t temp;
  if (steep_gradient) {
    temp = x1;
    x1 = y1;
    y1 = temp;
    temp = x2;
    x2 = y2;
    y2 = temp;
  }
  if (x1 > x2) {
    temp = x1;
    x1 = x2;
    x2 = temp;
    temp = y1;
    y1 = y2;
    y2 = temp;
  }

  int16_t dx = x2 - x1;
  int16_t dy = abs(y2 - y1);
  int16_t error_value = dx / 2;
  int16_t ystep = y1 < y2 ? 1 : -1;

  for (; x1 <= x2; x1++) {
    if (steep_gradient) {
      fb->SetPixel(y1, x1);
    } else {
      fb->SetPixel(x1, y1);
    }
    error_value -= dy;
    if (error_value < 0) {
      y1 += ystep;
      error_value += dx;
    }
  }
}

void DrawLine(pw::framebuffer::FramebufferRgb565* fb,
              int x1,
              int y1,
              int x2,
              int y2,
              pw::color::color_rgb565_t pen_color) {
  fb->SetPenColor(pen_color);
  DrawLine(fb, x1, y1, x2, y2);
}

// Draw a circle at center_x, center_y with given radius and color. Only a
// one-pixel outline is drawn if filled is false.
void DrawCircle(pw::framebuffer::FramebufferRgb565* fb,
                int center_x,
                int center_y,
                int radius,
                bool filled = false) {
  int fx = 0, fy = 0;
  int x = -radius, y = 0;
  int error_value = 2 - 2 * radius;
  while (x < 0) {
    if (!filled) {
      fx = x;
      fy = y;
    }
    // Draw each quarter circle
    for (int i = x; i <= fx; i++) {
      // Lower right
      fb->SetPixel(center_x - i, center_y + y);
      // Upper left
      fb->SetPixel(center_x + i, center_y - y);
    }
    for (int i = fy; i <= y; i++) {
      // Lower left
      fb->SetPixel(center_x - i, center_y - x);
      // Upper right
      fb->SetPixel(center_x + i, center_y + x);
    }
    radius = error_value;
    if (radius <= y) {
      y++;
      error_value += y * 2 + 1;
    }
    if (radius > x || error_value > y) {
      x++;
      error_value += x * 2 + 1;
    }
  }
}
void DrawCircle(pw::framebuffer::FramebufferRgb565* fb,
                int center_x,
                int center_y,
                int radius,
                pw::color::color_rgb565_t pen_color,
                bool filled = false) {
  fb->SetPenColor(pen_color);
  DrawCircle(fb, center_x, center_y, radius, filled);
}

void DrawHLine(pw::framebuffer::FramebufferRgb565* fb, int x1, int x2, int y) {
  for (int i = x1; i <= x2; i++) {
    fb->SetPixel(i, y);
  }
}

void DrawHLine(pw::framebuffer::FramebufferRgb565* fb,
               int x1,
               int x2,
               int y,
               color_rgb565_t pen_color) {
  fb->SetPenColor(pen_color);
  DrawHLine(fb, x1, x2, y);
}

void DrawRect(pw::framebuffer::FramebufferRgb565* fb,
              int x1,
              int y1,
              int x2,
              int y2,
              bool filled = false) {
  // Draw top and bottom lines.
  DrawHLine(fb, x1, x2, y1);
  DrawHLine(fb, x1, x2, y2);
  if (filled) {
    for (int y = y1 + 1; y < y2; y++) {
      DrawHLine(fb, x1, x2, y);
    }
  } else {
    for (int y = y1 + 1; y < y2; y++) {
      fb->SetPixel(x1, y);
      fb->SetPixel(x2, y);
    }
  }
}
void DrawRect(pw::framebuffer::FramebufferRgb565* fb,
              int x1,
              int y1,
              int x2,
              int y2,
              color_rgb565_t pen_color,
              bool filled = false) {
  fb->SetPenColor(pen_color);
  DrawRect(fb, x1, y1, x2, y2, filled);
}

void DrawRectWH(pw::framebuffer::FramebufferRgb565* fb,
                int x,
                int y,
                int w,
                int h,
                color_rgb565_t pen_color,
                bool filled = false) {
  fb->SetPenColor(pen_color);
  DrawRect(fb, x, y, x - 1 + w, y - 1 + h, filled);
}
void DrawRectWH(pw::framebuffer::FramebufferRgb565* fb,
                int x,
                int y,
                int w,
                int h,
                bool filled = false) {
  DrawRect(fb, x, y, x - 1 + w, y - 1 + h, filled);
}

void Fill(pw::framebuffer::FramebufferRgb565* fb) { fb->Fill(); }

void Fill(pw::framebuffer::FramebufferRgb565* fb, color_rgb565_t pen_color) {
  fb->SetPenColor(pen_color);
  fb->Fill();
}

void DrawSprite(pw::framebuffer::FramebufferRgb565* fb,
                int x,
                int y,
                pw::draw::SpriteSheet* sprite_sheet,
                int integer_scale = 1) {
  uint16_t color;
  int start_x, start_y;
  for (int current_x = 0; current_x < sprite_sheet->width; current_x++) {
    for (int current_y = 0; current_y < sprite_sheet->height; current_y++) {
      color = sprite_sheet->GetColor(
          current_x, current_y, sprite_sheet->current_index);
      if (color != sprite_sheet->transparent_color) {
        if (integer_scale == 1) {
          fb->SetPixel(x + current_x, y + current_y, color);
        }
        // If integer_scale > 1: draw a rectangle
        else if (integer_scale > 1) {
          start_x = x + (integer_scale * current_x);
          start_y = y + (integer_scale * current_y);
          DrawRectWH(
              fb, start_x, start_y, integer_scale, integer_scale, color, true);
        }
      }
    }
  }
}

void DrawTestPattern(pw::framebuffer::FramebufferRgb565* fb) {
  color_rgb565_t color = ColorRGBA(0x00, 0xFF, 0xFF).ToRgb565();
  // Create a Test Pattern
  for (int x = 0; x < fb->width; x++) {
    for (int y = 0; y < fb->height; y++) {
      if (y % 10 != x % 10) {
        fb->SetPixel(x, y, color);
      }
    }
  }
}

TextArea::TextArea(pw::framebuffer::FramebufferRgb565* fb, FontSet* font) {
  framebuffer = fb;
  cursor_x = 0;
  cursor_y = 0;
  current_font = font;
}

// Change the current font.
void TextArea::SetFont(FontSet* new_font) { current_font = new_font; }

void TextArea::SetCursor(int x, int y) {
  cursor_x = x;
  cursor_y = y;
  column_count = 0;
}

void TextArea::DrawCharacter(int character) {
  if (character == '\n') {
    cursor_y = cursor_y + current_font->height;
    cursor_x = cursor_x - (column_count * current_font->width);
    column_count = 0;
    return;
  }

  if ((int)character < current_font->starting_character ||
      (int)character > current_font->ending_character) {
    return;
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
        framebuffer->SetPixel(cursor_x + font_column, cursor_y + font_row);
      }
    }
  }
  cursor_x = cursor_x + current_font->width;
  column_count++;
}

void TextArea::DrawCharacter(int character,
                             color_rgb565_t rgb565_foreground_color) {
  framebuffer->SetPenColor(rgb565_foreground_color);
  DrawCharacter(character);
}

void TextArea::DrawCharacter(int character,
                             int x,
                             int y,
                             color_rgb565_t rgb565_foreground_color) {
  framebuffer->SetPenColor(rgb565_foreground_color);
  SetCursor(x, y);
  DrawCharacter(character);
}

void TextArea::DrawTestFontSheet(int character_width,
                                 int x,
                                 int y,
                                 color_rgb565_t rgb565_foreground_color) {
  framebuffer->SetPenColor(rgb565_foreground_color);
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
  char character;
  do {
    character = str[character_index];
    DrawCharacter(character);
    character_index++;
  } while (character != '\0');
}

void TextArea::DrawText(const char* str,
                        color_rgb565_t rgb565_foreground_color) {
  framebuffer->SetPenColor(rgb565_foreground_color);
  DrawText(str);
}

void TextArea::DrawText(const char* str,
                        int x,
                        int y,
                        color_rgb565_t rgb565_foreground_color) {
  framebuffer->SetPenColor(rgb565_foreground_color);
  SetCursor(x, y);
  DrawText(str);
}

}  // namespace pw::draw
