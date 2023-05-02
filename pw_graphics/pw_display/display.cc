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
#include "pw_display/display.h"

#include <utility>

#include "pw_assert/assert.h"
#include "pw_color/color.h"
#include "pw_framebuffer/framebuffer.h"
#include "pw_framebuffer_pool/framebuffer_pool.h"
#include "pw_status/try.h"

using pw::color::color_rgb565_t;
using pw::framebuffer::Framebuffer;
using pw::framebuffer::PixelFormat;

namespace pw::display {

Display::Display(pw::display_driver::DisplayDriver& display_driver,
                 pw::math::Size<uint16_t> size,
                 pw::framebuffer_pool::FramebufferPool& framebuffer_pool)
    : display_driver_(display_driver),
      size_(size),
      framebuffer_pool_(framebuffer_pool) {}

Display::~Display() = default;

#if DISPLAY_RESIZE
Status Display::UpdateNearestNeighbor(const Framebuffer& framebuffer) {
  PW_ASSERT(framebuffer.is_valid());
  if (!framebuffer.size().width || !framebuffer.size().height)
    return Status::Internal();

  const int fb_last_row_idx = framebuffer.size().height - 1;
  const int fb_last_col_idx = framebuffer.size().width - 1;

  constexpr int kResizeBufferNumPixels = 80;
  color_rgb565_t resize_buffer[kResizeBufferNumPixels];

  const color_rgb565_t* fbdata =
      static_cast<const color_rgb565_t*>(framebuffer.data());
  constexpr int kBytesPerPixel = sizeof(color_rgb565_t);
  const int num_src_row_pixels = framebuffer.row_bytes() / kBytesPerPixel;

  const int num_dst_rows = size_.height;
  const int num_dst_cols = size_.width;
  for (int dst_row_idx = 0; dst_row_idx < num_dst_rows; dst_row_idx++) {
    int src_row_idx = dst_row_idx * fb_last_row_idx / (num_dst_rows - 1);
    PW_ASSERT(src_row_idx >= 0);
    PW_ASSERT(src_row_idx < framebuffer.size().height);
    int next_buff_idx = 0;
    int dst_col_write_idx = 0;
    for (int dst_col_idx = 0; dst_col_idx < num_dst_cols; dst_col_idx++) {
      int src_col_idx = dst_col_idx * fb_last_col_idx / (num_dst_cols - 1);
      PW_ASSERT(src_col_idx >= 0);
      PW_ASSERT(src_col_idx < framebuffer.size().width);
      int src_pixel_idx = src_row_idx * num_src_row_pixels + src_col_idx;
      resize_buffer[next_buff_idx++] = fbdata[src_pixel_idx];
      if (next_buff_idx == kResizeBufferNumPixels) {
        // Buffer is full, flush it.
        PW_TRY(display_driver_.WriteRow(
            span(resize_buffer, kResizeBufferNumPixels),
            dst_row_idx,
            dst_col_write_idx));
        next_buff_idx = 0;
        dst_col_write_idx += kResizeBufferNumPixels;
      }
    }

    if (next_buff_idx) {
      // Pixels in buffer, flush them.
      PW_TRY(display_driver_.WriteRow(
          span(resize_buffer, next_buff_idx), dst_row_idx, dst_col_write_idx));
    }
  }
  return OkStatus();
}
#endif  // DISPLAY_RESIZE

Framebuffer Display::GetFramebuffer() {
  return framebuffer_pool_.GetFramebuffer();
}

Status Display::ReleaseFramebuffer(Framebuffer framebuffer) {
  pw::framebuffer_pool::FramebufferPool& fb_pool = framebuffer_pool_;
  auto write_cb = [&fb_pool](pw::framebuffer::Framebuffer fb, Status status) {
    PW_ASSERT_OK(status);
    fb_pool.ReleaseFramebuffer(std::move(fb));
  };
  if (!framebuffer.is_valid())
    return Status::InvalidArgument();
  if (framebuffer.size() != size_) {
#if DISPLAY_RESIZE
    return UpdateNearestNeighbor(framebuffer);
#endif
    // Rely on display driver's ability to support size mismatch. It is
    // expected to return an error if it cannot.
  }

  display_driver_.WriteFramebuffer(std::move(framebuffer), write_cb);
  return OkStatus();
}

}  // namespace pw::display
