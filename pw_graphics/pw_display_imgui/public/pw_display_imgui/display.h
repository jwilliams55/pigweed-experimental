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

#include "pw_coordinates/vec_int.h"
#include "pw_display/display.h"
#include "pw_display_driver_imgui/display_driver.h"
#include "pw_framebuffer/rgb565.h"
#include "pw_status/status.h"

namespace pw::display {

// A display that uses ImgUI and supports touch input.
class DisplayImgUI : public Display {
 public:
  DisplayImgUI(pw::framebuffer::FramebufferRgb565 framebuffer,
               pw::display_driver::DisplayDriverImgUI& display_driver);
  ~DisplayImgUI();

  bool TouchscreenAvailable() const override { return true; }
  bool NewTouchEvent() override;
  pw::coordinates::Vec3Int GetTouchPoint() override;

 private:
  pw::display_driver::DisplayDriverImgUI& display_driver_;
};

}  // namespace pw::display
