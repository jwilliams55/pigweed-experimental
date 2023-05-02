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

#include "pw_framebuffer_pool/framebuffer_pool.h"

static_assert(FRAMEBUFFER_COUNT > 0, "Must have at least one framebuffer");

using pw::framebuffer::Framebuffer;

namespace pw::framebuffer_pool {

FramebufferPool::FramebufferPool(const Config& config)
    : buffer_addresses_(config.fb_addr),
      buffer_dimensions_(config.dimensions),
      row_bytes_(config.row_bytes),
      pixel_format_(config.pixel_format),
      next_fb_idx_(0) {
  framebuffer_semaphore_.release(config.fb_addr.size());
}

FramebufferPool::~FramebufferPool() = default;

Framebuffer FramebufferPool::GetFramebuffer() {
  framebuffer_semaphore_.acquire();
  size_t idx = next_fb_idx_++;
  if (next_fb_idx_ == buffer_addresses_.size())
    next_fb_idx_ = 0;

  return Framebuffer(
      buffer_addresses_[idx], pixel_format_, buffer_dimensions_, row_bytes_);
}

Status FramebufferPool::ReleaseFramebuffer(Framebuffer) {
  framebuffer_semaphore_.release();
  return OkStatus();
}

}  // namespace pw::framebuffer_pool
