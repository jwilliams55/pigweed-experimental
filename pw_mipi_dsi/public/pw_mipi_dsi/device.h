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

#include "pw_framebuffer/rgb565.h"
#include "pw_status/status.h"

namespace pw::mipi::dsi {

using pw::framebuffer::FramebufferRgb565;

// Interface for to a MIPI Display Serial Interface(1) implementation.
//
// (1) https://www.mipi.org/specifications/dsi
class Device {
 public:
  constexpr static size_t kMaxFramebufferCount = 3;

  virtual ~Device() = default;

  // Retrieve a framebuffer for rendering. An invalid framebuffer will be
  // returned if there are no available framebuffers which can happen if
  // all framebuffers are in the process of being sent to the display.
  virtual pw::framebuffer::FramebufferRgb565 GetFramebuffer(void) = 0;

  // Begin the process of transporting the |framebuffer| to the display.
  virtual Status ReleaseFramebuffer(
      pw::framebuffer::FramebufferRgb565 framebuffer) = 0;

 protected:
  Device() = default;
};

}  // namespace pw::mipi::dsi
