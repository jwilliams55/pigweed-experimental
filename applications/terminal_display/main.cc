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
#include "pw_draw/text_area.h"
#include "pw_framebuffer/rgb565.h"
#include "pw_log/log.h"
#include "pw_spin_delay/delay.h"
#include "pw_string/string_builder.h"
#include "pw_sys_io/sys_io.h"
#include "pw_touchscreen/touchscreen.h"

using pw::color::colors_pico8_rgb565;
using pw::display::backend::Display;
using pw::draw::TextArea;
using pw::framebuffer::FramebufferRgb565;

namespace {

typedef bool (*bool_function_pointer)();
typedef pw::coordinates::Vec3Int (*vec3int_function_pointer)();

class DemoDecoder : public AnsiDecoder {
 public:
  DemoDecoder(TextArea& log_text_area) : log_text_area_(log_text_area) {}

 protected:
  virtual void SetFgColor(uint8_t r, uint8_t g, uint8_t b) {
    log_text_area_.SetForegroundColor(pw::color::ColorRGBA(r, g, b).ToRgb565());
  }
  virtual void SetBgColor(uint8_t r, uint8_t g, uint8_t b) {
    log_text_area_.SetBackgroundColor(pw::color::ColorRGBA(r, g, b).ToRgb565());
  }
  virtual void EmitChar(char c) { log_text_area_.DrawCharacter(c); }

 private:
  TextArea& log_text_area_;
};

TextArea* g_log_text_area;
DemoDecoder* g_demo_decoder;

void (*write_log_to_screen)(std::string_view) = [](std::string_view log) {
  static int cursor_x = 0;
  static int cursor_y = g_log_text_area->framebuffer->GetHeight() -
                        g_log_text_area->current_font->height;
  g_log_text_area->SetCursor(cursor_x, cursor_y);

  for (auto c : log) {
    g_demo_decoder->ProcessChar(c);
  }
  g_demo_decoder->ProcessChar('\n');

  cursor_x = g_log_text_area->cursor_x;
  cursor_y = g_log_text_area->cursor_y;

  pw::sys_io::WriteLine(log).IgnoreError();
};

const wchar_t pigweed_banner[] = {
    L"▒█████▄   █▓  ▄███▒  ▒█    ▒█ ░▓████▒ ░▓████▒ ▒▓████▄\n"
    L" ▒█░  █░ ░█▒ ██▒ ▀█▒ ▒█░ █ ▒█  ▒█   ▀  ▒█   ▀  ▒█  ▀█▌\n"
    L" ▒█▄▄▄█░ ░█▒ █▓░ ▄▄░ ▒█░ █ ▒█  ▒███    ▒███    ░█   █▌\n"
    L" ▒█▀     ░█░ ▓█   █▓ ░█░ █ ▒█  ▒█   ▄  ▒█   ▄  ░█  ▄█▌\n"
    L" ▒█      ░█░ ░▓███▀   ▒█▓▀▓█░ ░▓████▒ ░▓████▒ ▒▓████▀\n"};

void draw_sprite_and_text_demo(FramebufferRgb565& frame_buffer,
                               TextArea& log_text_area) {
  int sprite_pos_x = 10;
  int sprite_pos_y = 24;
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
  pw::draw::DrawCircle(
      &frame_buffer,
      sprite_pos_x + (pigweed_farm_sprite_sheet.width * sprite_scale) - 32,
      sprite_pos_y,
      20,
      colors_pico8_rgb565[COLOR_ORANGE],
      true);
  pw::draw::DrawCircle(
      &frame_buffer,
      sprite_pos_x + (pigweed_farm_sprite_sheet.width * sprite_scale) - 32,
      sprite_pos_y,
      18,
      colors_pico8_rgb565[COLOR_YELLOW],
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
  pw::draw::DrawSprite(
      &frame_buffer, sprite_pos_x, sprite_pos_y, &pigweed_farm_sprite_sheet, 4);

  // Draw some text
  pw::draw::TextArea text_area(&frame_buffer, &pw::draw::font6x8_box_chars);

  // Start drawing a x=0, y=72
  text_area.DrawCharacter('\n', 0, 72);

  // Draw the Pigweed "ASCII" banner.
  text_area.SetForegroundColor(colors_pico8_rgb565[COLOR_PINK]);
  text_area.SetCharacterWrap(false);
  text_area.DrawText(pigweed_banner);
  text_area.SetCharacterWrap(true);

  text_area.SetFont(&pw::draw::font6x8);
  for (int c = pw::draw::font6x8.starting_character;
       c <= pw::draw::font6x8.ending_character;
       c++) {
    if (c % 32 == 0) {
      text_area.DrawCharacter('\n');
    }
    text_area.SetForegroundColor(0);
    text_area.SetBackgroundColor(pw::color::colors_endesga32_rgb565[c % 32]);
    text_area.DrawCharacter(c);
  }
  // Reset background to black
  text_area.SetBackgroundColor(0);
  text_area.DrawCharacter('\n');

  text_area.SetForegroundColor(0xFFFF);
  text_area.SetFont(&pw::draw::font6x8);
  text_area.DrawTestFontSheet(32, text_area.cursor_x, text_area.cursor_y);
  text_area.DrawText("\n\nBox Characters:\n");

  text_area.SetFont(&pw::draw::font6x8_box_chars);
  text_area.DrawTestFontSheet(32, text_area.cursor_x, text_area.cursor_y);

  text_area.SetFont(&pw::draw::font6x8);
  text_area.DrawCharacter('\n');
}

void create_demo_log_messages() {
  // Create some demo log messages.
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
  TextArea log_text_area(&frame_buffer, &pw::draw::font6x8);
  DemoDecoder demo_decoder = DemoDecoder(log_text_area);

  // Save global pointers needed for the logging callback.
  // TODO(cmumford): See if we can use a functor or std::bind to eliminate
  // these.
  g_log_text_area = &log_text_area;
  g_demo_decoder = &demo_decoder;

  // Clear the framebuffer to black
  pw::draw::Fill(&frame_buffer, 0);

  // Init the display and touchscreen.
  PW_LOG_INFO("pw::display::Init()");
  display.Init();
  PW_LOG_INFO("pw::touchscreen::Init()");
  pw::touchscreen::Init();

  // Change log output function to write_log_to_screen.
  log_text_area.SetBackgroundColor(0);
  pw::log_basic::SetOutput(*write_log_to_screen);

  // Touch event variables
  Vec2 screen_center =
      Vec2(frame_buffer.GetWidth() / 2.0, frame_buffer.GetHeight() / 2.0);
  Vec2 touch_location;
  Vec2 touch_location_from_origin;
  float touch_location_angle;
  float touch_location_length;

  pw::coordinates::Vec3Int last_frame_touch_state(0, 0, 0);

  draw_sprite_and_text_demo(frame_buffer, log_text_area);
  // Push the frame buffer to the screen.
  display.Update(frame_buffer);

  // Setup the log message button position variables.
  TextArea button_text_area(&frame_buffer, &pw::draw::font6x8);
  int button_width = 19 * button_text_area.current_font->width;
  int button_height = button_text_area.current_font->height;
  int button_pos_x = frame_buffer.GetWidth() - button_width;
  int button_pos_y = 0;
  button_text_area.SetCursor(button_pos_x, button_pos_y);
  button_text_area.SetForegroundColor(colors_pico8_rgb565[COLOR_BLACK]);
  button_text_area.SetBackgroundColor(colors_pico8_rgb565[COLOR_BLUE]);

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
        touch_location = Vec2(point.x, point.y);
        touch_location_from_origin = touch_location - screen_center;
        touch_location_angle = touch_location_from_origin.angle();
        touch_location_length = touch_location_from_origin.length();

        PW_LOG_DEBUG("Touch: x:%d, y:%d, z:%d", point.x, point.y, point.z);

        // Find the angle and length of the touch location from the
        // screen_center.
        //
        // touch_location_angle will fall within the range [-pi, pi]
        // Map it to [0, 2*pi] instead.
        // if (touch_location_angle < 0)
        //   touch_location_angle = kTwoPi + touch_location_angle;
        // touch_location_angle = kTwoPi - touch_location_angle;
        // PW_LOG_DEBUG("       degrees:%f length:%f",
        //              degrees(touch_location_angle),
        //              touch_location_length);

        // If a button was just pressed, call create_demo_log_messages.
        if (button_just_pressed && touch_location.x >= button_pos_x &&
            touch_location.y >= button_pos_y &&
            touch_location.x < button_pos_x + button_width &&
            touch_location.y < button_pos_y + button_height) {
          create_demo_log_messages();
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

    button_text_area.SetCursor(button_pos_x, button_pos_y);
    button_text_area.DrawText(" Click to add logs ");

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
      // Log FPS Stats
      PW_LOG_INFO(
          "Time: %u - FPS: %d", pw::spin_delay::Millis(), frames_per_second);

      frames_per_second = frames;
      frames = 0;

      frame_start_millis = pw::spin_delay::Millis();
    }
  }
}
