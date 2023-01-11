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

#include "pw_display_driver_mipi/display_driver.h"

using pw::framebuffer::FramebufferRgb565;

namespace pw::display_driver {

DisplayDriverMipiDsi::DisplayDriverMipiDsi(
    pw::mipi::dsi::Device& device, pw::coordinates::Size<int> display_size)
    : device_(device), display_size_(display_size) {}

DisplayDriverMipiDsi::~DisplayDriverMipiDsi() = default;

Status DisplayDriverMipiDsi::Init() { return OkStatus(); }

FramebufferRgb565 DisplayDriverMipiDsi::GetFramebuffer() {
  return device_.GetFramebuffer();
}

Status DisplayDriverMipiDsi::ReleaseFramebuffer(FramebufferRgb565 framebuffer) {
  return device_.ReleaseFramebuffer(std::move(framebuffer));
};

Status DisplayDriverMipiDsi::WriteRow(span<uint16_t> row_pixels,
                                      int row_idx,
                                      int col_idx) {
  // TODO(cmumford): Implement for debugging purposes.
  return Status::Unimplemented();
}

int DisplayDriverMipiDsi::GetWidth() const { return display_size_.width; };

int DisplayDriverMipiDsi::GetHeight() const { return display_size_.height; }

}  // namespace pw::display_driver
