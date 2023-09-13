// Copyright 2023 The Pigweed Authors
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

#define PW_LOG_LEVEL PW_LOG_LEVEL_DEBUG

#include "app_common/common.h"
#include "graphics/surface.hpp"
#include "pw_assert/assert.h"
#include "pw_assert/check.h"
#include "pw_board_led/led.h"
#include "pw_color/color.h"
#include "pw_color/colors_endesga32.h"
#include "pw_color/colors_pico8.h"
#include "pw_display/display.h"
#include "pw_framebuffer/framebuffer.h"
#include "pw_log/log.h"
#include "pw_ring_buffer/prefixed_entry_ring_buffer.h"
#include "pw_spin_delay/delay.h"
#include "pw_string/string_builder.h"
#include "pw_sys_io/sys_io.h"
#include "random.h"

#if defined(USE_FREERTOS)
#include "FreeRTOS.h"
#include "task.h"
#endif  // if defined(USE_FREERTOS)

using pw::color::color_rgb565_t;
using pw::color::colors_pico8_rgb565;
using pw::display::Display;
using pw::framebuffer::Framebuffer;
using pw::ring_buffer::PrefixedEntryRingBuffer;

// TODO(tonymd): move this code into a pre_init section (i.e. boot.cc) which
//               is part of the target. Not all targets currently have this.
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
}       // extern "C"
#endif  // defined(DEFINE_FREERTOS_MEMORY_FUNCTIONS)

namespace {

#if defined(USE_FREERTOS)
std::array<StackType_t, configMINIMAL_STACK_SIZE> s_freertos_stack;
StaticTask_t s_freertos_tcb;
#endif  // defined(USE_FREERTOS)

struct test_particle {
  blit::Vec2 pos;
  blit::Vec2 vel;
  int age;
  bool generated = false;
};

void rain_generate(test_particle& p, blit::Surface screen) {
  p.pos = blit::Vec2(GetRandomFloat(screen.bounds.w),
                     GetRandomFloat(10) - (screen.bounds.h + 10));
  p.vel = blit::Vec2(0, 150);
  p.age = 0;
  p.generated = true;
};

void rain(blit::Surface screen, uint32_t time_ms, blit::Rect floor_position) {
  static test_particle s[300];
  static int generate_index = 0;
  static uint32_t last_time_ms = time_ms;

  int elapsed_ms = time_ms - last_time_ms;
  float td = (elapsed_ms) / 1000.0f;

  rain_generate(s[generate_index++], screen);
  if (generate_index >= 300)
    generate_index = 0;

  float w = sinf(time_ms / 1000.0f) * 0.05f;

  blit::Vec2 gvec = blit::Vec2(0, 9.8 * 5);
  blit::Vec2 gravity = gvec * td;

  for (auto& p : s) {
    if (p.generated) {
      p.vel += gravity;
      p.pos += p.vel * td;

      int floor = -3;
      if (p.pos.x > floor_position.x &&
          p.pos.x < (floor_position.x + floor_position.w))
        floor = -3 - (screen.bounds.h - floor_position.y);

      if (p.pos.y >= floor) {
        p.pos.y = floor;
        float bounce = (GetRandomFloat(10)) / 80.0f;
        p.vel.y *= -bounce;
        p.vel.x = (GetRandomFloat(30) - 15);
      }
      p.age++;

      int a = p.age / 2;
      int r = 100 - (a / 2);
      int g = 255 - (a / 2);
      int b = 255;  // -(a * 4);

      if (p.vel.length() > 20) {
        screen.pen = blit::Pen(b, g, r, 100);
        screen.pixel(p.pos + blit::Point(0, screen.bounds.h - 1));
        screen.pen = blit::Pen(b, g, r, 160);
        screen.pixel(p.pos + blit::Point(0, screen.bounds.h + 1));
      }
      screen.pen = blit::Pen(b, g, r, 180);
      screen.pixel(p.pos + blit::Point(0, screen.bounds.h + 2));
    }
  }

  last_time_ms = time_ms;
};

// Given a ring buffer full of uint32_t values, return the average value
// or zero if empty (or iteration error).
uint32_t CalcAverageUint32Value(PrefixedEntryRingBuffer& ring_buffer) {
  uint64_t sum = 0;
  uint32_t count = 0;
  for (const auto& entry_info : ring_buffer) {
    PW_ASSERT(entry_info.buffer.size() == sizeof(uint32_t));
    uint32_t val;
    std::memcpy(&val, entry_info.buffer.data(), sizeof(val));
    sum += val;
    count++;
  }
  return count == 0 ? 0 : sum / count;
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

  pw::board_led::Init();
  PW_CHECK_OK(Common::Init());

  Display& display = Common::GetDisplay();
  Framebuffer framebuffer = display.GetFramebuffer();
  PW_ASSERT(framebuffer.is_valid());

  blit::Surface screen = blit::Surface(
      (uint8_t*)framebuffer.data(),
      blit::PixelFormat::RGB565,
      blit::Size(framebuffer.size().width, framebuffer.size().height));
  screen.pen = blit::Pen(0, 0, 0, 255);
  screen.clear();

  display.ReleaseFramebuffer(std::move(framebuffer));

  uint32_t delta_screen_draw = 0;

  // The display loop.
  while (1) {
    uint32_t start = pw::spin_delay::Millis();
    framebuffer = display.GetFramebuffer();
    PW_ASSERT(framebuffer.is_valid());
    screen.data = (uint8_t*)framebuffer.data();

    // Draw Phase
    // Clear the screen
    screen.pen = blit::Pen(0, 0, 0);
    screen.clear();

    // Draw 32blit animation
    std::string text = "Pigweed + 32blit";
    auto text_size = screen.measure_text(text, blit::minimal_font, true);
    blit::Rect text_rect(
        blit::Point((screen.bounds.w / 2) - (text_size.w / 2),
                    (screen.bounds.h * .75) - (text_size.h / 2)),
        text_size);
    rain(screen, start - delta_screen_draw, text_rect);
    screen.pen = blit::Pen(0xFF, 0xFF, 0xFF);
    screen.text(
        text, blit::minimal_font, text_rect, true, blit::TextAlign::top_left);
    delta_screen_draw = pw::spin_delay::Millis() - start;

    // Update timers
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
      PW_LOG_INFO("FPS:%d, Draw:%dms, Flush:%dms",
                  frames_per_second,
                  CalcAverageUint32Value(draw_times),
                  CalcAverageUint32Value(flush_times));
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
