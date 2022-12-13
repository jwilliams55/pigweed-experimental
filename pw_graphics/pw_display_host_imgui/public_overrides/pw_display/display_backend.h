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

#include "pw_display/display.h"
#include "pw_display_driver_imgui/display_driver.h"

namespace pw::display::backend {

class Display : pw::display::Display {
 public:
  Display();
  virtual ~Display();

  // pw::display::Display implementation:
  Status Init() override;
  Status InitFramebuffer(
      pw::framebuffer::FramebufferRgb565* framebuffer) override;
  int GetWidth() const override;
  int GetHeight() const override;
  void Update(pw::framebuffer::FramebufferRgb565& framebuffer) override;
  bool TouchscreenAvailable() const override;
  bool NewTouchEvent() override;
  pw::coordinates::Vec3Int GetTouchPoint() override;

 private:
  constexpr static int kDisplayWidth = 320;
  constexpr static int kDisplayHeight = 240;
  constexpr static int kNumDisplayPixels = kDisplayWidth * kDisplayHeight;

  pw::display_driver::DisplayDriverImgUI display_driver_;
  uint16_t framebuffer_data_[kNumDisplayPixels];
};

}  // namespace pw::display::backend
