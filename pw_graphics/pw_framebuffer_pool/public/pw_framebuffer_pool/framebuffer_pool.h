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

#include <array>
#include <cstdint>

#include "pw_containers/vector.h"
#include "pw_framebuffer/framebuffer.h"
#include "pw_math/size.h"
#include "pw_status/status.h"
#include "pw_sync/counting_semaphore.h"

namespace pw::framebuffer_pool {

// FramebufferPool manages a collection of (one or more) framebuffers.
// It provides a mechanism to retrieve a buffer from the pool for use, and
// for returning that buffer back to the pool.
class FramebufferPool {
 public:
  using BufferArray = pw::Vector<void*>;

  // Constructor parameters.
  struct Config {
    const BufferArray& fb_addr;  // Address of each buffer in this pool.
    pw::math::Size<uint16_t> dimensions;  // width/height of each buffer.
    uint16_t row_bytes;                   // row bytes of each buffer.
    pw::framebuffer::PixelFormat pixel_format;
  };

  FramebufferPool(const Config& config);
  virtual ~FramebufferPool();

  // Return the framebuffer addresses for initialization purposes only.
  // Some drivers require these during initialization of their subsystems.
  // Do not use this as a means to retrieve the address of a framebuffer.
  // Always use GetFramebuffer if a new buffer is needed.
  const BufferArray& GetBuffersForInit() const { return buffer_addresses_; }

  // Return the row bytes for each framebuffer in this pool.
  uint16_t row_bytes() const { return row_bytes_; }

  // Return the dimensions (width/height) for each framebuffer in this pool.
  pw::math::Size<uint16_t> dimensions() const { return buffer_dimensions_; }

  // Return the pixel format for each framebuffer in this pool.
  pw::framebuffer::PixelFormat pixel_format() const { return pixel_format_; }

  // Return a framebuffer to the caller for use. This call WILL BLOCK until a
  // framebuffer is returned for use. Framebuffers *must* be returned to this
  // pool by a corresponding call to ReleaseFramebuffer. This function will only
  // return a valid framebuffers.
  //
  // This call is thread-safe, but not interrupt safe.
  virtual pw::framebuffer::Framebuffer GetFramebuffer();

  // Return the framebuffer to the pool available for use by the next call to
  // GetFramebuffer.
  //
  // This may be called on another thread or during an interrupt.
  virtual Status ReleaseFramebuffer(pw::framebuffer::Framebuffer framebuffer);

 private:
  pw::sync::CountingSemaphore framebuffer_semaphore_;
  const BufferArray& buffer_addresses_;         // Address of each pixel buffer.
  pw::math::Size<uint16_t> buffer_dimensions_;  // width/height of all buffers
  uint16_t row_bytes_;                          // All row bytes are the same.
  pw::framebuffer::PixelFormat pixel_format_;   // Shared pixel format.
  volatile size_t next_fb_idx_;
};

}  // namespace pw::framebuffer_pool
