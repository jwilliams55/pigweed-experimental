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

using pw::framebuffer::Framebuffer;

namespace pw::display_driver {

pw::mipi::dsi::Device::WriteCallback s_write_callback;

DisplayDriverMipiDsi::DisplayDriverMipiDsi(
    pw::mipi::dsi::Device& device, pw::math::Size<uint16_t> display_size)
    : device_(device), display_size_(display_size) {}

DisplayDriverMipiDsi::~DisplayDriverMipiDsi() = default;

Status DisplayDriverMipiDsi::Init() { return OkStatus(); }

void DisplayDriverMipiDsi::WriteFramebuffer(Framebuffer framebuffer,
                                            WriteCallback write_callback) {
  // PW_ASSERT(!s_write_callback);
  s_write_callback = std::move(write_callback);
  device_.WriteFramebuffer(std::move(framebuffer),
                           [](Framebuffer fb, Status status) {
                             // PW_ASSERT(s_write_callback);
                             s_write_callback(std::move(fb), status);
                           });
};

Status DisplayDriverMipiDsi::WriteRow(span<uint16_t> row_pixels,
                                      uint16_t row_idx,
                                      uint16_t col_idx) {
  // TODO(cmumford): Implement for development purposes if needed.
  return Status::Unimplemented();
}

uint16_t DisplayDriverMipiDsi::GetWidth() const { return display_size_.width; };

uint16_t DisplayDriverMipiDsi::GetHeight() const {
  return display_size_.height;
}

}  // namespace pw::display_driver
