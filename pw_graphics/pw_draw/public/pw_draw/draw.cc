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
#include "pw_draw/sprite_sheet.h"
#include "pw_framebuffer/rgb565.h"

using pw::color::color_rgb565_t;
using pw::framebuffer::FramebufferRgb565;

namespace pw::draw {

void DrawLine(FramebufferRgb565* fb,
              int x1,
              int y1,
              int x2,
              int y2,
              color_rgb565_t pen_color) {
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
      fb->SetPixel(y1, x1, pen_color);
    } else {
      fb->SetPixel(x1, y1, pen_color);
    }
    error_value -= dy;
    if (error_value < 0) {
      y1 += ystep;
      error_value += dx;
    }
  }
}

// Draw a circle at center_x, center_y with given radius and color. Only a
// one-pixel outline is drawn if filled is false.
void DrawCircle(FramebufferRgb565* fb,
                int center_x,
                int center_y,
                int radius,
                color_rgb565_t pen_color,
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
      fb->SetPixel(center_x - i, center_y + y, pen_color);
      // Upper left
      fb->SetPixel(center_x + i, center_y - y, pen_color);
    }
    for (int i = fy; i <= y; i++) {
      // Lower left
      fb->SetPixel(center_x - i, center_y - x, pen_color);
      // Upper right
      fb->SetPixel(center_x + i, center_y + x, pen_color);
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

void DrawHLine(
    FramebufferRgb565* fb, int x1, int x2, int y, color_rgb565_t pen_color) {
  for (int i = x1; i <= x2; i++) {
    fb->SetPixel(i, y, pen_color);
  }
}

void DrawRect(FramebufferRgb565* fb,
              int x1,
              int y1,
              int x2,
              int y2,
              color_rgb565_t pen_color,
              bool filled = false) {
  // Draw top and bottom lines.
  DrawHLine(fb, x1, x2, y1, pen_color);
  DrawHLine(fb, x1, x2, y2, pen_color);
  if (filled) {
    for (int y = y1 + 1; y < y2; y++) {
      DrawHLine(fb, x1, x2, y, pen_color);
    }
  } else {
    for (int y = y1 + 1; y < y2; y++) {
      fb->SetPixel(x1, y, pen_color);
      fb->SetPixel(x2, y, pen_color);
    }
  }
}

void DrawRectWH(FramebufferRgb565* fb,
                int x,
                int y,
                int w,
                int h,
                color_rgb565_t pen_color,
                bool filled = false) {
  DrawRect(fb, x, y, x - 1 + w, y - 1 + h, pen_color, filled);
}

void Fill(FramebufferRgb565* fb, color_rgb565_t pen_color) {
  fb->Fill(pen_color);
}

void DrawSprite(FramebufferRgb565* fb,
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

void DrawTestPattern(FramebufferRgb565* fb) {
  color_rgb565_t color = pw::color::ColorRGBA(0x00, 0xFF, 0xFF).ToRgb565();
  // Create a Test Pattern
  for (int x = 0; x < fb->GetWidth(); x++) {
    for (int y = 0; y < fb->GetHeight(); y++) {
      if (y % 10 != x % 10) {
        fb->SetPixel(x, y, color);
      }
    }
  }
}

}  // namespace pw::draw
