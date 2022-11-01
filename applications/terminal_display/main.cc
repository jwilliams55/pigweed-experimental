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
#include <array>
#include <cstdint>
#include <forward_list>
#include <memory>
#include <string_view>

#define PW_LOG_LEVEL PW_LOG_LEVEL_DEBUG

#include "ansi.h"
#include "pw_board_led/led.h"
#include "pw_color/color.h"
#include "pw_color/colors_endesga32.h"
#include "pw_color/colors_pico8.h"
#include "pw_coordinates/vec2.h"
#include "pw_coordinates/vec_int.h"
#include "pw_display/display_backend.h"
#include "pw_draw/draw.h"
#include "pw_draw/font_set.h"
#include "pw_draw/pigweed_farm.h"
#include "pw_framebuffer/rgb565.h"
#include "pw_log/log.h"
#include "pw_spin_delay/delay.h"
#include "pw_string/string_builder.h"
#include "pw_sys_io/sys_io.h"
#include "pw_touchscreen/touchscreen.h"
#include "text_buffer.h"

using pw::color::color_rgb565_t;
using pw::color::colors_pico8_rgb565;
using pw::coordinates::Size;
using pw::coordinates::Vector2;
using pw::display::backend::Display;
using pw::draw::FontSet;
using pw::framebuffer::FramebufferRgb565;

namespace {

constexpr color_rgb565_t kBlack = 0U;
constexpr color_rgb565_t kWhite = 0xffff;

class DemoDecoder : public AnsiDecoder {
 public:
  DemoDecoder(TextBuffer& log_text_buffer)
      : log_text_buffer_(log_text_buffer) {}

 protected:
  virtual void SetFgColor(uint8_t r, uint8_t g, uint8_t b) {
    fg_color_ = pw::color::ColorRGBA(r, g, b).ToRgb565();
  }
  virtual void SetBgColor(uint8_t r, uint8_t g, uint8_t b) {
    bg_color_ = pw::color::ColorRGBA(r, g, b).ToRgb565();
  }
  virtual void EmitChar(char c) {
    log_text_buffer_.DrawCharacter(TextBuffer::Char{c, fg_color_, bg_color_});
  }

 private:
  color_rgb565_t fg_color_ = kWhite;
  color_rgb565_t bg_color_ = kBlack;
  TextBuffer& log_text_buffer_;
};

// A simple implementation of a UI button.
class Button {
 public:
  // The label ptr must be valid throughout the lifetime of this object.
  Button(const wchar_t* label, const Vector2<int>& tl, const Size<int>& size)
      : label_(label), tl_(tl), size_(size) {}

  bool Contains(Vector2<int> pt) const {
    return pt.x >= tl_.x && pt.x < (tl_.x + size_.width) && pt.y >= tl_.y &&
           pt.y < (tl_.y + size_.height);
  }
  const wchar_t* label_;
  const Vector2<int> tl_;
  const Size<int> size_;
};

constexpr const wchar_t* kButtonLabel = L"Click to add logs";
constexpr int kButtonWidth = 108;
constexpr Vector2<int> kButtonTL = {320 - kButtonWidth, 0};
constexpr Size<int> kButtonSize = {kButtonWidth, 12};

TextBuffer s_log_text_buffer;
DemoDecoder s_demo_decoder(s_log_text_buffer);
Button g_button(kButtonLabel, kButtonTL, kButtonSize);

void DrawButton(const Button& button,
                color_rgb565_t bg_color,
                FramebufferRgb565& framebuffer) {
  pw::draw::DrawRectWH(&framebuffer,
                       button.tl_.x,
                       button.tl_.y,
                       button.size_.width,
                       button.size_.height,
                       bg_color,
                       /*filled=*/true);
  constexpr int kMargin = 2;
  Vector2<int> tl{button.tl_.x + kMargin, button.tl_.y + kMargin};
  DrawString(
      button.label_, tl, kBlack, bg_color, pw::draw::font6x8, framebuffer);
}

// Draw a font sheet starting at the given top-left screen coordinates.
Vector2<int> DrawTestFontSheet(Vector2<int> tl,
                               int num_columns,
                               color_rgb565_t fg_color,
                               color_rgb565_t bg_color,
                               const FontSet& font,
                               FramebufferRgb565& framebuffer) {
  Vector2<int> max_extents = tl;
  const int initial_x = tl.x;
  for (int c = font.starting_character; c <= font.ending_character; c++) {
    int char_idx = c - font.starting_character;
    if (char_idx % num_columns == 0) {
      tl.x = initial_x;
      tl.y += font.height;
    }
    auto char_size =
        DrawCharacter(c, tl, fg_color, bg_color, font, framebuffer);
    tl.x += char_size.width;
    max_extents.x = std::max(tl.x, max_extents.x);
    max_extents.y = std::max(tl.y, max_extents.y);
  }
  max_extents.y += font.height;
  return max_extents;
}

Vector2<int> DrawColorFontSheet(Vector2<int> tl,
                                int num_columns,
                                color_rgb565_t fg_color,
                                const FontSet& font,
                                FramebufferRgb565& framebuffer) {
  constexpr int kNumColors = sizeof(pw::color::colors_endesga32_rgb565) /
                             sizeof(pw::color::colors_endesga32_rgb565[0]);
  const int initial_x = tl.x;
  Vector2<int> max_extents = tl;
  for (int c = font.starting_character; c <= font.ending_character; c++) {
    int char_idx = c - font.starting_character;
    if (char_idx % num_columns == 0) {
      tl.x = initial_x;
      tl.y += font.height;
    }
    auto char_size =
        DrawCharacter(c,
                      tl,
                      fg_color,
                      pw::color::colors_endesga32_rgb565[char_idx % kNumColors],
                      font,
                      framebuffer);
    tl.x += char_size.width;
    max_extents.x = std::max(tl.x, max_extents.x);
    max_extents.y = std::max(tl.y, max_extents.y);
  }
  max_extents.y += font.height;
  return max_extents;
}

// The logging callback used to capture log messages sent to pw_log.
void LogCallback(std::string_view log) {
  for (auto c : log) {
    s_demo_decoder.ProcessChar(c);
  }
  s_demo_decoder.ProcessChar('\n');

  pw::sys_io::WriteLine(log).IgnoreError();
};

// Draw the Pigweed sprite and artwork at the top of the display.
// Returns the bottom Y coordinate drawn.
int DrawPigweedSprite(FramebufferRgb565& framebuffer) {
  int sprite_pos_x = 10;
  int sprite_pos_y = 24;
  int sprite_scale = 4;
  int border_size = 8;

  // Draw the dark blue border
  pw::draw::DrawRectWH(
      &framebuffer,
      sprite_pos_x - border_size,
      sprite_pos_y - border_size,
      pigweed_farm_sprite_sheet.width * sprite_scale + (border_size * 2),
      pigweed_farm_sprite_sheet.height * sprite_scale + (border_size * 2),
      colors_pico8_rgb565[COLOR_DARK_BLUE],
      true);

  // Shrink the border
  border_size = 4;

  // Draw the light blue background
  pw::draw::DrawRectWH(
      &framebuffer,
      sprite_pos_x - border_size,
      sprite_pos_y - border_size,
      pigweed_farm_sprite_sheet.width * sprite_scale + (border_size * 2),
      pigweed_farm_sprite_sheet.height * sprite_scale + (border_size * 2),
      colors_pico8_rgb565[COLOR_BLUE],
      true);

  static Vector2<int> sun_offset;
  static int motion_dir = -1;
  static int frame_num = 0;
  frame_num++;
  if ((frame_num % 5) == 0)
    sun_offset.x += motion_dir;
  if ((frame_num % 15) == 0)
    sun_offset.y -= motion_dir;
  if (sun_offset.x < -60)
    motion_dir = 1;
  else if (sun_offset.x > 10)
    motion_dir = -1;

  // Draw the Sun
  pw::draw::DrawCircle(&framebuffer,
                       sun_offset.x + sprite_pos_x +
                           (pigweed_farm_sprite_sheet.width * sprite_scale) -
                           32,
                       sun_offset.y + sprite_pos_y,
                       20,
                       colors_pico8_rgb565[COLOR_ORANGE],
                       true);
  pw::draw::DrawCircle(&framebuffer,
                       sun_offset.x + sprite_pos_x +
                           (pigweed_farm_sprite_sheet.width * sprite_scale) -
                           32,
                       sun_offset.y + sprite_pos_y,
                       18,
                       colors_pico8_rgb565[COLOR_YELLOW],
                       true);

  // Draw the farm sprite's shadow
  pigweed_farm_sprite_sheet.current_index = 1;
  pw::draw::DrawSprite(&framebuffer,
                       sprite_pos_x + 2,
                       sprite_pos_y + 2,
                       &pigweed_farm_sprite_sheet,
                       4);

  // Draw the farm sprite
  pigweed_farm_sprite_sheet.current_index = 0;
  pw::draw::DrawSprite(
      &framebuffer, sprite_pos_x, sprite_pos_y, &pigweed_farm_sprite_sheet, 4);

  return 76;
}

// Draw the pigweed text banner.
// Returns the bottom Y coordinate of the bottommost pixel set.
int DrawPigweedBanner(Vector2<int> tl, FramebufferRgb565& framebuffer) {
  constexpr std::array<std::wstring_view, 5> pigweed_banner = {
      L"▒█████▄   █▓  ▄███▒  ▒█    ▒█ ░▓████▒ ░▓████▒ ▒▓████▄",
      L" ▒█░  █░ ░█▒ ██▒ ▀█▒ ▒█░ █ ▒█  ▒█   ▀  ▒█   ▀  ▒█  ▀█▌",
      L" ▒█▄▄▄█░ ░█▒ █▓░ ▄▄░ ▒█░ █ ▒█  ▒███    ▒███    ░█   █▌",
      L" ▒█▀     ░█░ ▓█   █▓ ░█░ █ ▒█  ▒█   ▄  ▒█   ▄  ░█  ▄█▌",
      L" ▒█      ░█░ ░▓███▀   ▒█▓▀▓█░ ░▓████▒ ░▓████▒ ▒▓████▀"};

  // Draw the Pigweed "ASCII" banner.
  for (auto text_row : pigweed_banner) {
    Size<int> string_dims = DrawString(text_row,
                                       tl,
                                       colors_pico8_rgb565[COLOR_PINK],
                                       kBlack,
                                       pw::draw::font6x8_box_chars,
                                       framebuffer);
    tl.y += string_dims.height;
  }
  return tl.y - pw::draw::font6x8_box_chars.height;
}

// Draw the font sheets.
// Returns the bottom Y coordinate drawn.
int DrawFontSheets(Vector2<int> tl, FramebufferRgb565& framebuffer) {
  constexpr int kFontSheetVerticalPadding = 4;
  constexpr int kFontSheetNumColumns = 48;

  int initial_x = tl.x;
  tl = DrawColorFontSheet(tl,
                          kFontSheetNumColumns,
                          /*fg_color=*/kBlack,
                          pw::draw::font6x8,
                          framebuffer);

  tl.x = initial_x;
  tl.y -= pw::draw::font6x8.height;
  tl.y += kFontSheetVerticalPadding;

  tl = DrawTestFontSheet(tl,
                         kFontSheetNumColumns,
                         /*fg_color=*/kWhite,
                         /*bg_color=*/kBlack,
                         pw::draw::font6x8,
                         framebuffer);

  tl.x = initial_x;
  tl.y += kFontSheetVerticalPadding;

  Size<int> string_dims = DrawString(L"Box Characters:",
                                     tl,
                                     /*fg_color=*/kWhite,
                                     /*bg_color=*/kBlack,
                                     pw::draw::font6x8,
                                     framebuffer);
  tl.x += string_dims.width + pw::draw::font6x8.width;
  tl.y -= pw::draw::font6x8.height;

  tl = DrawTestFontSheet(tl,
                         /*num_columns=*/32,
                         /*fg_color=*/kWhite,
                         /*bg_color=*/kBlack,
                         pw::draw::font6x8_box_chars,
                         framebuffer);
  return tl.y;
}

// Draw the application header section which is mostly static text/graphics.
// Return the height (in pixels) of the header.
int DrawHeader(FramebufferRgb565& framebuffer) {
  DrawButton(
      g_button, /*bg_color=*/colors_pico8_rgb565[COLOR_BLUE], framebuffer);
  Vector2<int> tl = {0, 0};
  tl.y = DrawPigweedSprite(framebuffer);

  tl.y = DrawPigweedBanner(tl, framebuffer);
  constexpr int kFontSheetMargin = 4;
  tl.y += kFontSheetMargin;

  return DrawFontSheets(tl, framebuffer);
}

void DrawLogTextBuffer(int top,
                       const FontSet& font,
                       FramebufferRgb565& framebuffer) {
  constexpr int kLeft = 0;
  Vector2<int> loc;
  Vector2<int> pos{kLeft, top};
  Size<int> buffer_size = s_log_text_buffer.GetSize();
  for (loc.y = 0; loc.y < buffer_size.height; loc.y++) {
    for (loc.x = 0; loc.x < buffer_size.width; loc.x++) {
      auto ch = s_log_text_buffer.GetChar(loc);
      if (!ch.ok())
        continue;
      Size<int> char_size = DrawCharacter(ch->ch,
                                          pos,
                                          ch->foreground_color,
                                          ch->background_color,
                                          font,
                                          framebuffer);
      pos.x += char_size.width;
    }
    pos.y += font.height;
    pos.x = kLeft;
  }
}

void DrawFrame(FramebufferRgb565& framebuffer) {
  constexpr int kHeaderMargin = 4;
  int header_bottom = DrawHeader(framebuffer);
  DrawLogTextBuffer(
      header_bottom + kHeaderMargin, pw::draw::font6x8, framebuffer);
}

void CreateDemoLogMessages() {
  PW_LOG_CRITICAL("An irrecoverable error has occurred!");
  PW_LOG_ERROR("There was an error on our last operation");
  PW_LOG_WARN("Looks like something is amiss; consider investigating");
  PW_LOG_INFO("The operation went as expected");
  PW_LOG_DEBUG("Debug output");
}

}  // namespace

int main() {
  // Timing variables
  uint32_t frame_start_millis = pw::spin_delay::Millis();
  uint32_t frames = 0;
  int frames_per_second = 0;

  uint32_t time_start_delta = 0;
  uint32_t delta_time = 30000;  // Initial guess

  uint32_t time_start_button_check = pw::spin_delay::Millis();
  uint32_t delta_button_check = pw::spin_delay::Millis();

  uint32_t time_start_game_logic = 0;
  uint32_t delta_game_logic = 0;

  uint32_t time_start_draw_screen = 0;
  uint32_t delta_screen_draw = 0;

  uint32_t time_start_screen_spi_update = 0;
  uint32_t delta_screen_spi_update = 0;

  pw::board_led::Init();

  Display display;
  FramebufferRgb565 frame_buffer;
  display.InitFramebuffer(&frame_buffer).IgnoreError();

  pw::draw::Fill(&frame_buffer, kBlack);

  PW_LOG_INFO("pw::display::Init()");
  display.Init();

  PW_LOG_INFO("pw::touchscreen::Init()");
  pw::touchscreen::Init();

  pw::log_basic::SetOutput(LogCallback);

  pw::coordinates::Vec3Int last_frame_touch_state(0, 0, 0);

  DrawFrame(frame_buffer);
  // Push the frame buffer to the screen.
  display.Update(frame_buffer);

  // The display loop.
  while (1) {
    time_start_delta = pw::spin_delay::Millis();

    // Input Update Phase
    time_start_button_check = pw::spin_delay::Millis();

    pw::coordinates::Vec3Int point = display.GetTouchPoint();
    // Check for touchscreen events.
    if (display.TouchscreenAvailable() && display.NewTouchEvent()) {
      if (point.z > 0) {
        bool button_just_pressed = false;
        if (point.z != last_frame_touch_state.z)
          button_just_pressed = true;
        // New touch event
        Vector2<int> touch_location{point.x, point.y};

        PW_LOG_DEBUG("Touch: x:%d, y:%d, z:%d", point.x, point.y, point.z);

        // If a button was just pressed, call CreateDemoLogMessages.
        if (button_just_pressed && g_button.Contains(touch_location)) {
          CreateDemoLogMessages();
        }
      }
    }
    last_frame_touch_state.x = point.x;
    last_frame_touch_state.y = point.y;
    last_frame_touch_state.z = point.z;

    // End Input Update Phase
    delta_button_check = pw::spin_delay::Millis() - time_start_button_check;

    // Game Logic Phase
    time_start_game_logic = pw::spin_delay::Millis();

    // End Game Logic Phase
    delta_game_logic = pw::spin_delay::Millis() - time_start_game_logic;

    // Draw Phase
    time_start_draw_screen = pw::spin_delay::Millis();

    pw::draw::Fill(&frame_buffer, kBlack);
    DrawFrame(frame_buffer);

    // End Draw Phase
    delta_screen_draw = pw::spin_delay::Millis() - time_start_draw_screen;

    // Display Write Phase
    time_start_screen_spi_update = pw::spin_delay::Millis();

    display.Update(frame_buffer);

    // End Display Write Phase
    delta_screen_spi_update =
        pw::spin_delay::Millis() - time_start_screen_spi_update;

    // FPS Count Update
    delta_time = pw::spin_delay::Millis() - time_start_delta;

    // Every second make a log message.
    frames++;
    if (pw::spin_delay::Millis() > frame_start_millis + 1000) {
      PW_LOG_INFO(
          "Time: %u - FPS: %d", pw::spin_delay::Millis(), frames_per_second);

      frames_per_second = frames;
      frames = 0;

      frame_start_millis = pw::spin_delay::Millis();
    }
  }
}
