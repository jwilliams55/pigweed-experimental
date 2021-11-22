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
#include <cstdint>
#include <forward_list>

// #define PW_LOG_LEVEL PW_LOG_LEVEL_INFO

#include "pw_board_led/led.h"
#include "pw_color/color.h"
#include "pw_color/colors_pico8.h"
#include "pw_coordinates/vec2.h"
#include "pw_coordinates/vec_int.h"
#include "pw_display/display.h"
#include "pw_draw/draw.h"
#include "pw_draw/font6x8.h"
#include "pw_draw/pigweed_farm.h"
#include "pw_framebuffer/rgb565.h"
#include "pw_log/log.h"
#include "pw_spin_delay/delay.h"
#include "pw_string/string_builder.h"
#include "pw_touchscreen/touchscreen.h"

using pw::color::colors_pico8_rgb565;

namespace {

typedef bool (*bool_function_pointer)();
typedef pw::coordinates::Vec3Int (*vec3int_function_pointer)();
pw::framebuffer::FramebufferRgb565 frame_buffer = FramebufferRgb565();

const uint8_t banner[] = {
    127, 128, 128, 128, 128, 128, 129, 32,  32,  128, 127, 32,  32,  129, 128,
    128, 128, 127, 32,  32,  127, 128, 32,  32,  32,  32,  127, 128, 32,  127,
    127, 128, 128, 128, 128, 127, 32,  127, 127, 128, 128, 128, 128, 127, 32,
    127, 127, 128, 128, 128, 128, 129, 10,  127, 128, 127, 32,  32,  128, 127,
    32,  127, 128, 127, 32,  128, 128, 127, 32,  132, 128, 127, 32,  127, 128,
    127, 32,  128, 32,  127, 128, 32,  32,  127, 128, 32,  32,  32,  132, 32,
    32,  127, 128, 32,  32,  32,  132, 32,  32,  127, 128, 32,  32,  132, 128,
    130, 10,  127, 128, 129, 129, 129, 128, 127, 32,  127, 128, 127, 32,  128,
    127, 127, 32,  129, 129, 127, 32,  127, 128, 127, 32,  128, 32,  127, 128,
    32,  32,  127, 128, 128, 128, 32,  32,  32,  32,  127, 128, 128, 128, 32,
    32,  32,  32,  127, 128, 32,  32,  32,  128, 130, 10,  127, 128, 132, 32,
    32,  32,  32,  32,  127, 128, 127, 32,  127, 128, 32,  32,  32,  128, 127,
    32,  127, 128, 127, 32,  128, 32,  127, 128, 32,  32,  127, 128, 32,  32,
    32,  129, 32,  32,  127, 128, 32,  32,  32,  129, 32,  32,  127, 128, 32,
    32,  129, 128, 130, 10,  127, 128, 32,  32,  32,  32,  32,  32,  127, 128,
    127, 32,  127, 127, 128, 128, 128, 132, 32,  32,  32,  127, 128, 127, 132,
    127, 128, 127, 32,  127, 127, 128, 128, 128, 128, 127, 32,  127, 127, 128,
    128, 128, 128, 127, 32,  127, 127, 128, 128, 128, 128, 132, 10,  0,
};

}  // namespace

int main() {
  uint32_t frame_start_millis = pw::spin_delay::Millis();
  uint32_t frames = 0;
  int frames_per_second = 0;

  uint32_t time_start_delta = 0;
  uint32_t delta_time = 30000;  // Initial guess
  float delta_seconds = delta_time * 0.000001;

  uint32_t time_start_button_check = pw::spin_delay::Micros();
  uint32_t delta_button_check = pw::spin_delay::Micros();

  uint32_t time_start_game_logic = 0;
  uint32_t delta_game_logic = 0;

  uint32_t time_start_draw_screen = 0;
  uint32_t delta_screen_draw = 0;

  uint32_t time_start_screen_spi_update = 0;
  uint32_t delta_screen_spi_update = 0;

  pw::StringBuffer<64> string_buffer;

  pw::board_led::Init();
  pw::board_led::TurnOn();
  pw::spin_delay::WaitMillis(500);
  pw::board_led::TurnOff();

  PW_LOG_INFO("pw::display::Init()");
  uint16_t* ifb = pw::display::GetInternalFramebuffer();
  if (ifb != NULL) {
    frame_buffer.SetFramebufferData(ifb, 320, 240);
  }
  frame_buffer.SetPenColor(0x0726);
  pw::draw::Fill(&frame_buffer);

  pw::display::Init();
  pw::touchscreen::Init();

  // Touchscreen Functions
  // Default to using pw::touchscreen
  bool_function_pointer touch_screen_available_func =
      &pw::touchscreen::Available;
  vec3int_function_pointer get_touch_screen_point_func =
      &pw::touchscreen::GetTouchPoint;
  bool_function_pointer new_touch_event_func = &pw::touchscreen::NewTouchEvent;
  // Check if pw::display implements touchscreen functions
  if (pw::display::TouchscreenAvailable()) {
    touch_screen_available_func = &pw::display::TouchscreenAvailable;
    get_touch_screen_point_func = &pw::display::GetTouchPoint;
    new_touch_event_func = &pw::display::NewTouchEvent;
  }

  bool touch_screen_exists = (*touch_screen_available_func)();

  Vec2 screen_center =
      Vec2(pw::display::GetWidth() / 2, pw::display::GetHeight() / 2);
  Vec2 touch_location;
  Vec2 touch_location_from_origin;
  float touch_location_angle;
  float touch_location_length;

  while (1) {
    time_start_delta = pw::spin_delay::Micros();

    // Input Update Phase
    bool new_touch_event = false;

    time_start_button_check = pw::spin_delay::Micros();

    if (touch_screen_exists && (*new_touch_event_func)()) {
      pw::coordinates::Vec3Int point = (*get_touch_screen_point_func)();
      if (point.z > 0) {
        new_touch_event = true;
        touch_location = Vec2(point.x, point.y);
        touch_location_from_origin = touch_location - screen_center;
        touch_location_angle = touch_location_from_origin.angle();
        touch_location_length = touch_location_from_origin.length();

        // touch_location_angle will fall within the range [-pi, pi]
        // Map it to [0, 2*pi] instead.
        if (touch_location_angle < 0)
          touch_location_angle = kTwoPi + touch_location_angle;
        // Map to ship angle.
        touch_location_angle = kTwoPi - touch_location_angle;

        PW_LOG_DEBUG("Touch: x:%d, y:%d, z:%d, angle:%f, degrees:%f length:%f",
                     point.x,
                     point.y,
                     point.z,
                     touch_location_angle,
                     degrees(touch_location_angle),
                     touch_location_length);
      }
    }

    delta_button_check = pw::spin_delay::Micros() - time_start_button_check;

    // Game Logic Phase
    time_start_game_logic = pw::spin_delay::Micros();

    delta_game_logic = pw::spin_delay::Micros() - time_start_game_logic;

    // Draw Phase
    time_start_draw_screen = pw::spin_delay::Micros();

    frame_buffer.SetPenColor(0);
    pw::draw::Fill(&frame_buffer);

    pw::draw::TextArea text_area(&frame_buffer, &font6x8);

    text_area.DrawCharacter('\n', 0, 1, colors_pico8_rgb565[COLOR_DARK_BLUE]);

    // Draw the Pigweed "ASCII" banner.
    int i = 0;
    while (banner[i] > 0) {
      text_area.DrawCharacter(banner[i], colors_pico8_rgb565[COLOR_PINK]);
      i++;
    }

    for (int c = font6x8.starting_character; c <= font6x8.ending_character;
         c++) {
      if (c % 16 == 0) {
        text_area.DrawCharacter('\n');
      }
      frame_buffer.SetPenColor(colors_pico8_rgb565[c % 16]);
      text_area.DrawCharacter(c);
    }

    text_area.DrawCharacter('\n');
    text_area.DrawCharacter('\n');
    text_area.DrawTestFontSheet(
        32, text_area.cursor_x, text_area.cursor_y, 0xFFFF);

    int sprite_pos_x = 10;
    int sprite_pos_y = 180;
    int sprite_scale = 4;
    int border_size = 8;

    // Draw the dark blue border
    pw::draw::DrawRectWH(
        &frame_buffer,
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
        &frame_buffer,
        sprite_pos_x - border_size,
        sprite_pos_y - border_size,
        pigweed_farm_sprite_sheet.width * sprite_scale + (border_size * 2),
        pigweed_farm_sprite_sheet.height * sprite_scale + (border_size * 2),
        colors_pico8_rgb565[COLOR_BLUE],
        true);

    // Draw the Sun
    frame_buffer.SetPenColor(colors_pico8_rgb565[COLOR_ORANGE]);
    pw::draw::DrawCircle(
        &frame_buffer,
        sprite_pos_x + (pigweed_farm_sprite_sheet.width * sprite_scale) - 32,
        sprite_pos_y,
        20,
        true);
    frame_buffer.SetPenColor(colors_pico8_rgb565[COLOR_YELLOW]);
    pw::draw::DrawCircle(
        &frame_buffer,
        sprite_pos_x + (pigweed_farm_sprite_sheet.width * sprite_scale) - 32,
        sprite_pos_y,
        18,
        true);

    // Draw the farm sprite's shadow
    pigweed_farm_sprite_sheet.current_index = 1;
    pw::draw::DrawSprite(&frame_buffer,
                         sprite_pos_x + 2,
                         sprite_pos_y + 2,
                         &pigweed_farm_sprite_sheet,
                         4);

    // Draw the farm sprite
    pigweed_farm_sprite_sheet.current_index = 0;
    pw::draw::DrawSprite(&frame_buffer,
                         sprite_pos_x,
                         sprite_pos_y,
                         &pigweed_farm_sprite_sheet,
                         4);

    // Print FPS Stats
    int current_line = 210;
    int font_height = 9;
    string_buffer.clear();
    string_buffer.Format("%.2f (%d) FPS",
                         1.0 / (((float)delta_time) / 1000 / 1000),
                         frames_per_second);
    // pw::display::DrawText(0, current_line, COLOR_WHITE,
    // string_buffer.data());

    current_line += font_height;
    string_buffer.clear();
    string_buffer.Format("Loop Time: %u (microseconds) = ", delta_time);
    // pw::display::DrawText(0, current_line, COLOR_WHITE,
    // string_buffer.data());

    current_line += font_height;
    string_buffer.clear();
    string_buffer.Format("  input:%u + logic:%u + draw:%u + SPI:%u",
                         delta_button_check,
                         delta_game_logic,
                         delta_screen_draw,
                         delta_screen_spi_update);
    // pw::display::DrawText(0, current_line, COLOR_WHITE,
    // string_buffer.data());

    delta_screen_draw = pw::spin_delay::Micros() - time_start_draw_screen;

    // SPI Send Phase
    time_start_screen_spi_update = pw::spin_delay::Micros();

    pw::display::Update(&frame_buffer);

    delta_screen_spi_update =
        pw::spin_delay::Micros() - time_start_screen_spi_update;

    // FPS Count Update
    delta_time = pw::spin_delay::Micros() - time_start_delta;
    delta_seconds = delta_time * 0.000001;

    frames++;
    if (pw::spin_delay::Millis() > frame_start_millis + 1000) {
      PW_LOG_INFO("%.2f (%d) FPS",
                  1.0 / (((float)delta_time) / 1000 / 1000),
                  frames_per_second);
      PW_LOG_INFO("  input:%u + logic:%u + draw:%u + SPI:%u",
                  delta_button_check,
                  delta_game_logic,
                  delta_screen_draw,
                  delta_screen_spi_update);

      frames_per_second = frames;
      frames = 0;

      frame_start_millis = pw::spin_delay::Millis();
    }
  }
}
