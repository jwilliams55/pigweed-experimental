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
#include <cwchar>
#include <forward_list>
#include <memory>
#include <string_view>
#include <utility>

#define PW_LOG_LEVEL PW_LOG_LEVEL_DEBUG

#include "ansi.h"
#include "app_common/common.h"
#include "pw_assert/assert.h"
#include "pw_assert/check.h"
#include "pw_board_led/led.h"
#include "pw_color/color.h"
#include "pw_color/colors_endesga32.h"
#include "pw_color/colors_pico8.h"
#include "pw_coordinates/vec2.h"
#include "pw_coordinates/vec_int.h"
#include "pw_draw/draw.h"
#include "pw_draw/font_set.h"
#include "pw_draw/pigweed_farm.h"
#include "pw_framebuffer/rgb565.h"
#include "pw_log/log.h"
#include "pw_ring_buffer/prefixed_entry_ring_buffer.h"
#include "pw_spin_delay/delay.h"
#include "pw_string/string_builder.h"
#include "pw_sys_io/sys_io.h"
#include "pw_touchscreen/touchscreen.h"
#include "text_buffer.h"

#if defined(USE_FREERTOS)
#include "FreeRTOS.h"
#include "task.h"
#endif  // if defined(USE_FREERTOS)

using pw::color::color_rgb565_t;
using pw::color::colors_pico8_rgb565;
using pw::coordinates::Size;
using pw::coordinates::Vector2;
using pw::display::Display;
using pw::draw::FontSet;
using pw::framebuffer::FramebufferRgb565;
using pw::ring_buffer::PrefixedEntryRingBuffer;

// TODO(cmumford): move this code into a pre_init section (i.e. boot.cc) which
//                 is part of the target. Not all targets currently have this.
#if defined(DEFINE_FREERTOS_MEMORY_FUNCTIONS)
std::array<StackType_t, 100 /*configMINIMAL_STACK_SIZE*/> freertos_idle_stack;
StaticTask_t freertos_idle_tcb;

std::array<StackType_t, configTIMER_TASK_STACK_DEPTH> freertos_timer_stack;
StaticTask_t freertos_timer_tcb;

extern "C" {
// Required for configUSE_TIMERS.
void vApplicationGetTimerTaskMemory(StaticTask_t** ppxTimerTaskTCBBuffer,
                                    StackType_t** ppxTimerTaskStackBuffer,
                                    uint32_t* pulTimerTaskStackSize) {
  *ppxTimerTaskTCBBuffer = &freertos_timer_tcb;
  *ppxTimerTaskStackBuffer = freertos_timer_stack.data();
  *pulTimerTaskStackSize = freertos_timer_stack.size();
}

void vApplicationGetIdleTaskMemory(StaticTask_t** ppxIdleTaskTCBBuffer,
                                   StackType_t** ppxIdleTaskStackBuffer,
                                   uint32_t* pulIdleTaskStackSize) {
  *ppxIdleTaskTCBBuffer = &freertos_idle_tcb;
  *ppxIdleTaskStackBuffer = freertos_idle_stack.data();
  *pulIdleTaskStackSize = freertos_idle_stack.size();
}
}  // extern "C"
#endif  // defined(DEFINE_FREERTOS_MEMORY_FUNCTIONS)

namespace {

constexpr color_rgb565_t kBlack = 0U;
constexpr color_rgb565_t kWhite = 0xffff;

class DemoDecoder : public AnsiDecoder {
 public:
  DemoDecoder(TextBuffer& log_text_buffer)
      : log_text_buffer_(log_text_buffer) {}

 protected:
  void SetFgColor(uint8_t r, uint8_t g, uint8_t b) override {
    fg_color_ = pw::color::ColorRGBA(r, g, b).ToRgb565();
  }
  void SetBgColor(uint8_t r, uint8_t g, uint8_t b) override {
    bg_color_ = pw::color::ColorRGBA(r, g, b).ToRgb565();
  }
  void EmitChar(char c) override {
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
#if defined(USE_FREERTOS)
std::array<StackType_t, configMINIMAL_STACK_SIZE> s_freertos_stack;
StaticTask_t s_freertos_tcb;
#endif  // defined(USE_FREERTOS)

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

void DrawFPS(Vector2<int> tl,
             FramebufferRgb565& framebuffer,
             std::wstring_view fps_msg) {
  if (fps_msg.empty())
    return;

  DrawString(fps_msg,
             tl,
             colors_pico8_rgb565[COLOR_PEACH],
             kBlack,
             pw::draw::font6x8,
             framebuffer);
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
int DrawHeader(FramebufferRgb565& framebuffer, std::wstring_view fps_msg) {
  DrawButton(
      g_button, /*bg_color=*/colors_pico8_rgb565[COLOR_BLUE], framebuffer);
  Vector2<int> tl = {0, 0};
  tl.y = DrawPigweedSprite(framebuffer);

  tl.y = DrawPigweedBanner(tl, framebuffer);
  constexpr int kFontSheetMargin = 4;
  tl.y += kFontSheetMargin;

  DrawFPS({1, 2}, framebuffer, fps_msg);

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

void DrawFrame(FramebufferRgb565& framebuffer, std::wstring_view fps_msg) {
  constexpr int kHeaderMargin = 4;
  int header_bottom = DrawHeader(framebuffer, fps_msg);
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

// Given a ring buffer full of uint32_t values, return the average value
// or zero if empty.
uint32_t CalcAverageUint32Value(PrefixedEntryRingBuffer& ring_buffer) {
  if (!ring_buffer.EntryCount())
    return 0;
  uint64_t sum = 0;
  uint32_t count = 0;
  pw::ring_buffer::PrefixedEntryRingBufferMulti::iterator it =
      ring_buffer.begin();
  for (; it != ring_buffer.end(); ++it) {
    PW_ASSERT(it->buffer.size() == sizeof(uint32_t));
    uint32_t val;
    std::memcpy(&val, it->buffer.data(), sizeof(val));
    sum += val;
    count++;
  }
  return sum / count;
}

}  // namespace

void MainTask(void* pvParameters) {
  // Timing variables
  uint32_t frame_start_millis = pw::spin_delay::Millis();
  uint32_t frames = 0;
  int frames_per_second = 0;
  std::array<wchar_t, 40> fps_buffer = {0};
  std::wstring_view fps_view(fps_buffer.data(), 0);
  std::byte draw_buffer[30 * sizeof(uint32_t)];
  std::byte flush_buffer[30 * sizeof(uint32_t)];
  PrefixedEntryRingBuffer draw_times;
  PrefixedEntryRingBuffer flush_times;

  draw_times.SetBuffer(draw_buffer);
  flush_times.SetBuffer(flush_buffer);

  pw::log_basic::SetOutput(LogCallback);

  pw::board_led::Init();
  PW_CHECK_OK(Common::Init());

  Display& display = Common::GetDisplay();
  FramebufferRgb565 framebuffer = display.GetFramebuffer();
  PW_ASSERT(framebuffer.IsValid());

  pw::draw::Fill(&framebuffer, kBlack);

  PW_LOG_INFO("pw::touchscreen::Init()");
  pw::touchscreen::Init();

  pw::coordinates::Vec3Int last_frame_touch_state(0, 0, 0);

  DrawFrame(framebuffer, fps_view);
  // Push the frame buffer to the screen.
  display.ReleaseFramebuffer(std::move(framebuffer));

  // The display loop.
  while (1) {
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

    uint32_t start = pw::spin_delay::Millis();
    framebuffer = display.GetFramebuffer();
    pw::draw::Fill(&framebuffer, kBlack);
    DrawFrame(framebuffer, fps_view);
    uint32_t end = pw::spin_delay::Millis();
    uint32_t time = end - start;
    draw_times.PushBack(pw::as_bytes(pw::span{std::addressof(time), 1}));
    start = end;

    display.ReleaseFramebuffer(std::move(framebuffer));
    time = pw::spin_delay::Millis() - start;
    flush_times.PushBack(pw::as_bytes(pw::span{std::addressof(time), 1}));

    // Every second make a log message.
    frames++;
    if (pw::spin_delay::Millis() > frame_start_millis + 1000) {
      frames_per_second = frames;
      frames = 0;
      PW_LOG_INFO("Time: %lu - FPS: %d",
                  static_cast<unsigned long>(pw::spin_delay::Millis()),
                  frames_per_second);
      int len = std::swprintf(fps_buffer.data(),
                              fps_buffer.size(),
                              L"FPS:%d, Draw:%dms, Flush:%dms",
                              frames_per_second,
                              CalcAverageUint32Value(draw_times),
                              CalcAverageUint32Value(flush_times));
      fps_view = std::wstring_view(fps_buffer.data(), len);

      frame_start_millis = pw::spin_delay::Millis();
    }
  }
}

int main(void) {
#if defined(USE_FREERTOS)
  TaskHandle_t task_handle = xTaskCreateStatic(MainTask,
                                               "main",
                                               s_freertos_stack.size(),
                                               /*pvParameters=*/nullptr,
                                               tskIDLE_PRIORITY,
                                               s_freertos_stack.data(),
                                               &s_freertos_tcb);
  PW_CHECK_NOTNULL(task_handle);  // Ensure it succeeded.
  vTaskStartScheduler();
#else
  MainTask(/*pvParameters=*/nullptr);
#endif
  return 0;
}
