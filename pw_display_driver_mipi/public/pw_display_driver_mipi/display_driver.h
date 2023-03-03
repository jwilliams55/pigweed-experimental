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

#include "pw_coordinates/size.h"
#include "pw_display_driver/display_driver.h"
#include "pw_mipi_dsi/device.h"

namespace pw::display_driver {

// A display driver that communicates with a display controller over the
// MIPI Display Serial Interface (MIPI DSI).
class DisplayDriverMipiDsi : public DisplayDriver {
 public:
  DisplayDriverMipiDsi(pw::mipi::dsi::Device& device,
                       pw::coordinates::Size<int> display_size);
  ~DisplayDriverMipiDsi() override;

  // pw::display_driver::DisplayDriver implementation:
  Status Init() override;
  pw::framebuffer::Framebuffer GetFramebuffer(void) override;
  Status ReleaseFramebuffer(pw::framebuffer::Framebuffer framebuffer) override;
  Status WriteRow(span<uint16_t> row_pixels, int row_idx, int col_idx) override;
  int GetWidth() const override;
  int GetHeight() const override;

 private:
  pw::mipi::dsi::Device& device_;
  const pw::coordinates::Size<int> display_size_;
};

}  // namespace pw::display_driver
