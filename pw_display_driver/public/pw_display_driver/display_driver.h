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

#include <cstddef>
#include <cstdint>

#include "pw_framebuffer/framebuffer.h"
#include "pw_function/function.h"
#include "pw_span/span.h"
#include "pw_status/status.h"

namespace pw::display_driver {

// This interface defines a software display driver. This is the software
// component responsible for all communications with a display controller.
// The display controller is the hardware component of a display that
// controls pixel values and other physical display properties.
class DisplayDriver {
 public:
  // Called on the completion of a write operation.
  using WriteCallback = Callback<void(framebuffer::Framebuffer, Status)>;

  virtual ~DisplayDriver() = default;

  // Initialize the display controller.
  virtual Status Init() = 0;

  // Send all pixels in the supplied |framebuffer| to the display controller
  // for display.
  virtual void WriteFramebuffer(pw::framebuffer::Framebuffer framebuffer,
                                WriteCallback write_callback) = 0;

  // Send a row of pixels to the display. The number of pixels must be <=
  // display width.
  virtual Status WriteRow(span<uint16_t> row_pixels,
                          uint16_t row_idx,
                          uint16_t col_idx) = 0;

  virtual uint16_t GetWidth() const = 0;

  virtual uint16_t GetHeight() const = 0;

  // Display driver supports resizing during write.
  virtual bool SupportsResize() const { return false; }
};

}  // namespace pw::display_driver
