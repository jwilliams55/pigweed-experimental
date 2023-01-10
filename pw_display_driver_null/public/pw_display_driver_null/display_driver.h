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

#include "pw_display_driver/display_driver.h"

namespace pw::display_driver {

// A no-op display driver that does nothing. Handy for display-less targets and
// testing.
class DisplayDriverNULL : public DisplayDriver {
 public:
  ~DisplayDriverNULL() override = default;

  // pw::display_driver::DisplayDriver implementation:
  pw::Status Init() override { return pw::OkStatus(); }
  pw::Status Update(const pw::framebuffer::FramebufferRgb565&) override {
    return pw::OkStatus();
  }
  Status WriteRow(span<uint16_t>, int, int) override { return pw::OkStatus(); }
  int GetWidth() const override { return 0; };
  int GetHeight() const override { return 0; };
};

}  // namespace pw::display_driver
