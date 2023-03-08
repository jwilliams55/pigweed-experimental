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
#include "pw_display_imgui/display.h"

namespace pw::display {

DisplayImgUI::DisplayImgUI(
    pw::display_driver::DisplayDriverImgUI& display_driver,
    pw::coordinates::Size<uint16_t> size)
    : Display(display_driver, size), display_driver_(display_driver) {}

DisplayImgUI::~DisplayImgUI() = default;

bool DisplayImgUI::NewTouchEvent() { return display_driver_.NewTouchEvent(); }

pw::coordinates::Vector3<int> DisplayImgUI::GetTouchPoint() {
  return display_driver_.GetTouchPoint();
}

}  // namespace pw::display
