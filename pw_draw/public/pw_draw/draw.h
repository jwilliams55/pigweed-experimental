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

#include <math.h>

#include "pw_color/color.h"
#include "pw_draw/font_set.h"
#include "pw_draw/sprite_sheet.h"
#include "pw_framebuffer/rgb565.h"

namespace pw::draw {

void DrawLine(
    pw::framebuffer::FramebufferRgb565* fb, int x1, int y1, int x2, int y2);

void DrawLine(pw::framebuffer::FramebufferRgb565* fb,
              int x1,
              int y1,
              int x2,
              int y2,
              pw::color::color_rgb565_t pen_color);

// Draw a circle at center_x, center_y with given radius and color. Only a
// one-pixel outline is drawn if filled is false.
void DrawCircle(pw::framebuffer::FramebufferRgb565* fb,
                int center_x,
                int center_y,
                int radius,
                bool filled);

void DrawCircle(pw::framebuffer::FramebufferRgb565* fb,
                int center_x,
                int center_y,
                int radius,
                pw::color::color_rgb565_t pen_color,
                bool filled);

void DrawHLine(pw::framebuffer::FramebufferRgb565* fb, int x1, int x2, int y);
void DrawHLine(pw::framebuffer::FramebufferRgb565* fb,
               int x1,
               int x2,
               int y,
               color_rgb565_t pen_color);
void DrawRect(pw::framebuffer::FramebufferRgb565* fb,
              int x1,
              int y1,
              int x2,
              int y2,
              bool filled);

void DrawRect(pw::framebuffer::FramebufferRgb565* fb,
              int x1,
              int y1,
              int x2,
              int y2,
              color_rgb565_t pen_color,
              bool filled);

void DrawRectWH(pw::framebuffer::FramebufferRgb565* fb,
                int x,
                int y,
                int w,
                int h,
                color_rgb565_t pen_color,
                bool filled);

void DrawRectWH(pw::framebuffer::FramebufferRgb565* fb,
                int x,
                int y,
                int w,
                int h,
                bool filled);

void Fill(pw::framebuffer::FramebufferRgb565* fb);

void Fill(pw::framebuffer::FramebufferRgb565* fb, color_rgb565_t pen_color);

void DrawSprite(pw::framebuffer::FramebufferRgb565* fb,
                int x,
                int y,
                pw::draw::SpriteSheet* sprite_sheet,
                int integer_scale);

void DrawTestPattern();

class TextArea {
 public:
  int cursor_x;
  int cursor_y;
  int column_count;
  FontSet* current_font;
  pw::framebuffer::FramebufferRgb565* framebuffer;

  TextArea(pw::framebuffer::FramebufferRgb565* fb, FontSet* font);

  // Change the current font.
  void SetFont(FontSet* new_font);

  void SetCursor(int x, int y);

  void DrawCharacter(int character);

  void DrawCharacter(int character, color_rgb565_t rgb565_foreground_color);

  void DrawCharacter(int character,
                     int x,
                     int y,
                     color_rgb565_t rgb565_foreground_color);

  void DrawTestFontSheet(int character_width,
                         int x,
                         int y,
                         color_rgb565_t rgb565_foreground_color);

  // DrawText at x, y (upper left pixel of font). Carriage returns will move
  // text to the next line.
  void DrawText(const char* str);

  void DrawText(const char* str, color_rgb565_t rgb565_foreground_color);

  void DrawText(const wchar_t* str);

  void DrawText(const wchar_t* str, color_rgb565_t rgb565_foreground_color);

  void DrawText(const char* str,
                int x,
                int y,
                color_rgb565_t rgb565_foreground_color);
};

}  // namespace pw::draw
