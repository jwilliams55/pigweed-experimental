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
#pragma once

#include "pw_display_driver/display_driver.h"
#include "pw_math/size.h"
#include "pw_mipi_dsi/device.h"

namespace pw::display_driver {

// A display driver that communicates with a display controller over the
// MIPI Display Serial Interface (MIPI DSI).
class DisplayDriverMipiDsi : public DisplayDriver {
 public:
  DisplayDriverMipiDsi(pw::mipi::dsi::Device& device,
                       pw::math::Size<uint16_t> display_size);
  ~DisplayDriverMipiDsi() override;

  // pw::display_driver::DisplayDriver implementation:
  Status Init() override;
  void WriteFramebuffer(pw::framebuffer::Framebuffer framebuffer,
                        WriteCallback write_callback) override;
  Status WriteRow(span<uint16_t> row_pixels,
                  uint16_t row_idx,
                  uint16_t col_idx) override;
  uint16_t GetWidth() const override;
  uint16_t GetHeight() const override;

 private:
  pw::mipi::dsi::Device& device_;
  const pw::math::Size<uint16_t> display_size_;
};

}  // namespace pw::display_driver
