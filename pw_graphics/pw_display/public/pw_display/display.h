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
#include "pw_framebuffer/rgb565.h"
#include "pw_status/status.h"

namespace pw::display {

// Defines an interface that all display backends must implement.
class Display {
 public:
  virtual ~Display() = default;

  // Initialize the display.
  virtual Status Init() = 0;

  // Initialize the supplied |framebuffer| to the appropriate size for the
  // display.
  virtual Status InitFramebuffer(
      pw::framebuffer::FramebufferRgb565* framebuffer) = 0;

  // TODO(tonymd): Add a DPI or physical size value.
  virtual int GetWidth() const = 0;
  virtual int GetHeight() const = 0;

  // TODO(tonymd): Add update functions for new framebuffer types or make this a
  // templated class.
  virtual void Update(pw::framebuffer::FramebufferRgb565& framebuffer) = 0;

  virtual bool TouchscreenAvailable() const = 0;
  virtual bool NewTouchEvent() = 0;
  virtual pw::coordinates::Vec3Int GetTouchPoint() = 0;

 protected:
  Display() = default;
};

}  // namespace pw::display
