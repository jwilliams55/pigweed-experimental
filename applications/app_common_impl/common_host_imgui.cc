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
#include "app_common/common.h"
#include "pw_display_driver_imgui/display_driver.h"
#include "pw_display_imgui/display.h"
#include "pw_status/try.h"

using pw::Status;
using pw::color::color_rgb565_t;

namespace {

constexpr int kDisplayScaleFactor = 1;
constexpr int kFramebufferWidth = LCD_WIDTH / kDisplayScaleFactor;
constexpr int kFramebufferHeight = LCD_HEIGHT / kDisplayScaleFactor;
constexpr int kNumPixels = kFramebufferWidth * kFramebufferHeight;
constexpr int kFramebufferRowBytes = sizeof(uint16_t) * kFramebufferWidth;
constexpr pw::coordinates::Size<int> kDisplaySize = {LCD_WIDTH, LCD_HEIGHT};

color_rgb565_t s_pixel_data[kNumPixels];
constexpr pw::framebuffer::pool::PoolData s_fb_pool_data = {
    .fb_addr =
        {
            s_pixel_data,
            nullptr,
            nullptr,
        },
    .num_fb = 1,
    .size = {kFramebufferWidth, kFramebufferHeight},
    .row_bytes = kFramebufferRowBytes,
    .start = {0, 0},
};
pw::display_driver::DisplayDriverImgUI s_display_driver(s_fb_pool_data);
pw::display::DisplayImgUI s_display(s_display_driver, kDisplaySize);

}  // namespace

// static
Status Common::Init() { return s_display_driver.Init(); }

// static
pw::display::Display& Common::GetDisplay() { return s_display; }
