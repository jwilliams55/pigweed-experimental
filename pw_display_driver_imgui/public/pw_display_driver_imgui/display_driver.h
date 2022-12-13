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
#include "pw_display_driver/display_driver.h"

namespace pw::display_driver {

class DisplayDriverImgUI : public DisplayDriver {
 public:
  DisplayDriverImgUI();

  bool NewTouchEvent();
  pw::coordinates::Vec3Int GetTouchPoint();

  // pw::display_driver::DisplayDriver implementation:
  Status Init() override;
  Status Update(pw::framebuffer::FramebufferRgb565* framebuffer) override;

 private:
  void RecreateLcdTexture();
  void Render();
};

}  // namespace pw::display_driver
