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
#include "public/pw_framebuffer/writer.h"
#include "pw_assert/assert.h"

using pw::color::color_rgb565_t;

namespace pw::framebuffer {

FramebufferReader::FramebufferReader(const Framebuffer& framebuffer)
    : framebuffer_(framebuffer) {
  PW_ASSERT(framebuffer_.pixel_format() == PixelFormat::RGB565);
  PW_ASSERT(framebuffer_.IsValid());
}

Result<color_rgb565_t> FramebufferReader::GetPixel(uint16_t x,
                                                   uint16_t y) const {
  if (x >= framebuffer_.size().width || y >= framebuffer_.size().height) {
    return Status::OutOfRange();
  }
  const color_rgb565_t* data =
      static_cast<const color_rgb565_t*>(framebuffer_.GetFramebufferData());
  return data[y * framebuffer_.size().width + x];
}

}  // namespace pw::framebuffer
