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

#include "pw_framebuffer_pool/framebuffer_pool.h"
#include "pw_mipi_dsi_mcuxpresso/device.h"
#include "pw_status/status.h"

namespace pw::framebuffer_pool {

// FramebufferPoolMCUXpresso uses the MCUXpressoDevice to manage framebuffers.
// NXP's display controller (dc_fb_t) currently does buffer management. This
// implementation delegates to that system for buffer management.
class FramebufferPoolMCUXpresso : public FramebufferPool {
 public:
  FramebufferPoolMCUXpresso(const Config& config);

  // Initialize the instance. Must be called before using other methods.
  pw::Status Init(pw::mipi::dsi::MCUXpressoDevice* device);

  // pw::framebuffer_pool::FramebufferPool implementation:
  pw::framebuffer::Framebuffer GetFramebuffer() override;
  Status ReleaseFramebuffer(pw::framebuffer::Framebuffer framebuffer) override;

 private:
  pw::mipi::dsi::MCUXpressoDevice* device_;
};

}  // namespace pw::framebuffer_pool
