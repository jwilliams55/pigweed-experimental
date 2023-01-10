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
using pw::framebuffer::FramebufferRgb565;

namespace {

constexpr int kDisplayScaleFactor = 1;
constexpr int kFramebufferWidth = LCD_WIDTH / kDisplayScaleFactor;
constexpr int kFramebufferHeight = LCD_HEIGHT / kDisplayScaleFactor;
constexpr int kNumPixels = kFramebufferWidth * kFramebufferHeight;
constexpr int FramebufferRowBytes = sizeof(uint16_t) * kFramebufferWidth;

uint16_t s_pixel_data[kNumPixels];
pw::display_driver::DisplayDriverImgUI s_display_driver;
pw::display::DisplayImgUI s_display(FramebufferRgb565(s_pixel_data,
                                                      kFramebufferWidth,
                                                      kFramebufferHeight,
                                                      FramebufferRowBytes),
                                    s_display_driver);

}  // namespace

// static
Status Common::Init() {
  PW_TRY(s_display_driver.Init());
  return s_display.Init();
}

// static
pw::display::Display& Common::GetDisplay() { return s_display; }
