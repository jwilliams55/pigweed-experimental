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

#include <string_view>

#include "pw_color/color.h"
#include "pw_draw/font_set.h"
#include "pw_draw/sprite_sheet.h"
#include "pw_framebuffer/framebuffer.h"
#include "pw_math/size.h"
#include "pw_math/vector2.h"

namespace pw::draw {

void DrawLine(pw::framebuffer::Framebuffer& fb,
              int x1,
              int y1,
              int x2,
              int y2,
              pw::color::color_rgb565_t pen_color);

// Draw a circle at center_x, center_y with given radius and color. Only a
// one-pixel outline is drawn if filled is false.
void DrawCircle(pw::framebuffer::Framebuffer& fb,
                int center_x,
                int center_y,
                int radius,
                pw::color::color_rgb565_t pen_color,
                bool filled);

void DrawHLine(pw::framebuffer::Framebuffer& fb,
               int x1,
               int x2,
               int y,
               pw::color::color_rgb565_t pen_color);

void DrawRect(pw::framebuffer::Framebuffer& fb,
              int x1,
              int y1,
              int x2,
              int y2,
              pw::color::color_rgb565_t pen_color,
              bool filled);

void DrawRectWH(pw::framebuffer::Framebuffer& fb,
                int x,
                int y,
                int w,
                int h,
                pw::color::color_rgb565_t pen_color,
                bool filled);

void Fill(pw::framebuffer::Framebuffer& fb,
          pw::color::color_rgb565_t pen_color);

void DrawSprite(pw::framebuffer::Framebuffer& fb,
                int x,
                int y,
                pw::draw::SpriteSheet* sprite_sheet,
                int integer_scale);

void DrawTestPattern();

pw::math::Size<int> DrawCharacter(int ch,
                                  pw::math::Vector2<int> pos,
                                  pw::color::color_rgb565_t fg_color,
                                  pw::color::color_rgb565_t bg_color,
                                  const FontSet& font,
                                  pw::framebuffer::Framebuffer& framebuffer);

pw::math::Size<int> DrawString(std::wstring_view str,
                               pw::math::Vector2<int> pos,
                               pw::color::color_rgb565_t fg_color,
                               pw::color::color_rgb565_t bg_color,
                               const FontSet& font,
                               pw::framebuffer::Framebuffer& framebuffer);

}  // namespace pw::draw
