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
#include "pw_framebuffer/framebuffer.h"
#include "pw_framebuffer_pool/framebuffer_pool.h"
#include "pw_math/size.h"
#include "pw_math/vector3.h"
#include "pw_status/status.h"

namespace pw::display {

// The display is an object that represents a single display (or screen)
// attached to the host. There is a 1:1 correspondence with the screen
// that it manages. It has one or more framebuffers which its clients may
// use for rendering.
class Display {
 public:
  Display(pw::display_driver::DisplayDriver& display_driver,
          pw::math::Size<uint16_t> size);
  virtual ~Display();

  // Return a framebuffer to which the caller may draw. When drawing is complete
  // the framebuffer must be returned using ReleaseFramebuffer(). An invalid
  // framebuffer may be returned, so the caller should verify it is valid
  // before use.
  pw::framebuffer::Framebuffer GetFramebuffer();

  // Release the framebuffer back to the display. The display will
  // send the framebuffer data to the screen. This function will block until
  // the transfer has completed.
  //
  // This function should only be passed a valid framebuffer returned by
  // a paired call to GetFramebuffer.
  //
  // If The pw_display_DISPLAY_RESIZE build variable is set and the display
  // size is different than the framebuffer size then the framebuffer contents
  // will be resized using the nearest-neighbor algorithm.
  Status ReleaseFramebuffer(pw::framebuffer::Framebuffer framebuffer);

  // Return the width (in pixels) of the associated display.
  uint16_t GetWidth() const { return size_.width; }

  // Return the height (in pixels) of the associated display.
  uint16_t GetHeight() const { return size_.height; }

  // Does the associated screen have a touch screen?
  virtual bool TouchscreenAvailable() const { return false; }

  // Is there a new touch event available?
  virtual bool NewTouchEvent() { return false; }

  // Return the new touch point.
  virtual pw::math::Vector3<int> GetTouchPoint() { return {0, 0, 0}; }

 private:
#if DISPLAY_RESIZE
  // Update screen while scaling the framebuffer using nearest
  // neighbor algorithm.
  Status UpdateNearestNeighbor(const pw::framebuffer::Framebuffer& framebuffer);
#endif  // if DISPLAY_RESIZE

  pw::display_driver::DisplayDriver& display_driver_;
  const pw::math::Size<uint16_t> size_;
};

}  // namespace pw::display
