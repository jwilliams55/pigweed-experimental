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

#include "pw_framebuffer_pool_mcuxpresso/framebuffer_pool.h"

using pw::framebuffer::Framebuffer;

namespace pw::framebuffer_pool {

FramebufferPoolMCUXpresso::FramebufferPoolMCUXpresso(const Config& config)
    : FramebufferPool(config), device_(nullptr) {}

pw::Status FramebufferPoolMCUXpresso::Init(
    pw::mipi::dsi::MCUXpressoDevice* device) {
  PW_ASSERT(device_ == nullptr);
  device_ = device;
  return OkStatus();
}

Framebuffer FramebufferPoolMCUXpresso::GetFramebuffer() {
  PW_ASSERT(device_ != nullptr);
  return device_->GetFramebuffer();
}

Status FramebufferPoolMCUXpresso::ReleaseFramebuffer(Framebuffer framebuffer) {
  // framebuffers are implicitly released to the NXP device during the
  // rendering process. The |device_| has a drawing callback that informs it
  // when the update is complete whereby it returns the framebuffer to the
  // available pool list.
  return OkStatus();
}

}  // namespace pw::framebuffer_pool
