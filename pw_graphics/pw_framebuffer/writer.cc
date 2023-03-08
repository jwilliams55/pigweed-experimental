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

#include <sys/signal.h>

#include "pw_assert/assert.h"

using pw::color::color_rgb565_t;

namespace pw::framebuffer {

FramebufferWriter::FramebufferWriter(Framebuffer& framebuffer)
    : FramebufferReader(framebuffer) {}

void FramebufferWriter::SetPixel(uint16_t x,
                                 uint16_t y,
                                 color_rgb565_t pixel_value) {
  color_rgb565_t* data =
      static_cast<color_rgb565_t*>(framebuffer_.GetFramebufferData());
  auto fb_size = framebuffer_.size();
  if (x < fb_size.width && y < fb_size.height) {
    data[y * fb_size.width + x] = pixel_value;
  }
}

void FramebufferWriter::Blit(const Framebuffer& fb, uint16_t x, uint16_t y) {
  auto s = fb.size();
  FramebufferReader reader(fb);
  for (uint16_t current_x = 0; current_x < s.width; current_x++) {
    for (uint16_t current_y = 0; current_y < s.height; current_y++) {
      if (auto pixel_color = reader.GetPixel(current_x, current_y);
          pixel_color.ok()) {
        SetPixel(x + current_x, y + current_y, pixel_color.value());
      }
    }
  }
}

void FramebufferWriter::Fill(color_rgb565_t pixel_value) {
  color_rgb565_t* data =
      static_cast<color_rgb565_t*>(framebuffer_.GetFramebufferData());
  const size_t num_pixels =
      framebuffer_.size().width * framebuffer_.size().height;
  for (size_t i = 0; i < num_pixels; i++) {
    data[i] = pixel_value;
  }
}

}  // namespace pw::framebuffer
