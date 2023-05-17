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

#include "pw_framebuffer/framebuffer.h"
#include "pw_function/function.h"
#include "pw_status/status.h"

namespace pw::mipi::dsi {

// Interface for to a MIPI Display Serial Interface(1) implementation.
//
// (1) https://www.mipi.org/specifications/dsi
class Device {
 public:
  // Called on the completion of a write operation.
  using WriteCallback = Callback<void(pw::framebuffer::Framebuffer, Status)>;

  virtual ~Device() = default;

  // Retrieve a framebuffer for use. Will block until a framebuffer is
  // available.
  virtual pw::framebuffer::Framebuffer GetFramebuffer() = 0;

  // Begin the process of transporting the |framebuffer| to the display.
  virtual void WriteFramebuffer(pw::framebuffer::Framebuffer framebuffer,
                                WriteCallback write_callback) = 0;

 protected:
  Device() = default;
};

}  // namespace pw::mipi::dsi
