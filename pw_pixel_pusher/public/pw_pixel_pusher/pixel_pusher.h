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
#include "pw_framebuffer_pool/framebuffer_pool.h"
#include "pw_function/function.h"
#include "pw_status/status.h"

namespace pw::pixel_pusher {

class PixelPusher {
 public:
  using WriteCallback = Callback<void(framebuffer::Framebuffer, Status)>;

  virtual ~PixelPusher() = default;

  virtual Status Init(
      const pw::framebuffer_pool::FramebufferPool& framebuffer_pool) = 0;

  virtual void WriteFramebuffer(framebuffer::Framebuffer framebuffer,
                                WriteCallback complete_callback) = 0;
};

}  // namespace pw::pixel_pusher
