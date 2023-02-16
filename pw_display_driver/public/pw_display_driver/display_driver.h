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

#include "pw_framebuffer/framebuffer.h"
#include "pw_span/span.h"
#include "pw_status/status.h"

namespace pw::display_driver {

// This interface defines a software display driver. This is the software
// component responsible for all communications with a display controller.
// The display controller is the hardware component of a display that
// controls pixel values and other physical display properties.
class DisplayDriver {
 public:
  virtual ~DisplayDriver() = default;

  // Initialize the display controller.
  virtual Status Init() = 0;

  // Return a framebuffer to which the caller may draw. When drawing is complete
  // the framebuffer must be returned using ReleaseFramebuffer().
  virtual pw::framebuffer::Framebuffer GetFramebuffer() = 0;

  // Send all pixels in the supplied |framebuffer| to the display controller
  // for display.
  virtual Status ReleaseFramebuffer(
      pw::framebuffer::Framebuffer framebuffer) = 0;

  // Send a row of pixels to the display. The number of pixels must be <=
  // display width.
  virtual Status WriteRow(span<uint16_t> row_pixels,
                          int row_idx,
                          int col_idx) = 0;

  virtual int GetWidth() const = 0;

  virtual int GetHeight() const = 0;
};

}  // namespace pw::display_driver
