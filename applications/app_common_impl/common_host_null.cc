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
#include "pw_display/display.h"
#include "pw_display_driver_null/display_driver.h"
#include "pw_status/try.h"

using pw::Status;
using pw::framebuffer_pool::FramebufferPool;

namespace {

constexpr pw::math::Size<uint16_t> kDisplaySize = {DISPLAY_WIDTH,
                                                   DISPLAY_HEIGHT};

pw::display_driver::DisplayDriverNULL s_display_driver;
pw::framebuffer_pool::FramebufferPool s_fb_pool({
    .fb_addr = {nullptr},
    .dimensions = {0, 0},
    .row_bytes = 0,
});
pw::display::Display s_display(s_display_driver, kDisplaySize, s_fb_pool);

}  // namespace

// static
Status Common::Init() { return s_display_driver.Init(); }

// static
pw::display::Display& Common::GetDisplay() { return s_display; }
