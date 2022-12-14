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
#include "pw_framebuffer/rgb565.h"
#include "pw_status/status.h"

namespace pw::display {

// The display is an object that represents a single display (or screen)
// attached to the host. There is a 1:1 correspondence with the screen
// that it manages. It has one or more framebuffers which its clients may
// use for rendering.
class Display {
 public:
  Display(pw::framebuffer::FramebufferRgb565 framebuffer,
          pw::display_driver::DisplayDriver& display_driver);
  virtual ~Display();

  // Initialize the display instance.
  Status Init() { return OkStatus(); }

  // Initialize the |framebuffer| for the caller to draw in.
  Status InitFramebuffer(pw::framebuffer::FramebufferRgb565* framebuffer);

  // Return the width (in pixels) of the associated display.
  int GetWidth() const { return framebuffer_.GetWidth(); }

  // Return the height (in pixels) of the associated display.
  int GetHeight() const { return framebuffer_.GetHeight(); }

  // Transport the pixels in the |framebuffer| to the associated screen.
  void Update(pw::framebuffer::FramebufferRgb565& framebuffer);

  // Does the associated screen have a touch screen?
  virtual bool TouchscreenAvailable() const { return false; }

  // Is there a new touch event available?
  virtual bool NewTouchEvent() { return false; }

  // Return the new touch point.
  virtual pw::coordinates::Vec3Int GetTouchPoint() {
    return pw::coordinates::Vec3Int{0, 0, 0};
  }

 private:
  pw::framebuffer::FramebufferRgb565 framebuffer_;
  pw::display_driver::DisplayDriver& display_driver_;
};

}  // namespace pw::display
