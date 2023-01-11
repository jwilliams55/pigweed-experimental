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

#include "pw_framebuffer/rgb565.h"
#include "pw_framebuffer_pool/framebuffer_pool.h"
#include "pw_status/try.h"

using pw::color::color_rgb565_t;
using pw::framebuffer::FramebufferRgb565;

namespace pw::display {

Display::Display(pw::display_driver::DisplayDriver& display_driver,
                 pw::coordinates::Size<int> size)
    : display_driver_(display_driver), size_(size) {}

Display::~Display() = default;

Status Display::UpdateNearestNeighbor(const FramebufferRgb565& framebuffer) {
  PW_ASSERT(framebuffer.IsValid());
  if (!framebuffer.GetWidth() || !framebuffer.GetHeight())
    return Status::Internal();

  const int fb_last_row_idx = framebuffer.GetHeight() - 1;
  const int fb_last_col_idx = framebuffer.GetWidth() - 1;

  constexpr int kResizeBufferNumPixels = 80;
  color_rgb565_t resize_buffer[kResizeBufferNumPixels];

  const color_rgb565_t* fbdata = framebuffer.GetFramebufferData();
  constexpr int kBytesPerPixel = sizeof(color_rgb565_t);
  const int num_src_row_pixels = framebuffer.GetRowBytes() / kBytesPerPixel;

  const int num_dst_rows = size_.height;
  const int num_dst_cols = size_.width;
  for (int dst_row_idx = 0; dst_row_idx < num_dst_rows; dst_row_idx++) {
    int src_row_idx = dst_row_idx * fb_last_row_idx / (num_dst_rows - 1);
    PW_ASSERT(src_row_idx >= 0);
    PW_ASSERT(src_row_idx < framebuffer.GetHeight());
    int next_buff_idx = 0;
    int dst_col_write_idx = 0;
    for (int dst_col_idx = 0; dst_col_idx < num_dst_cols; dst_col_idx++) {
      int src_col_idx = dst_col_idx * fb_last_col_idx / (num_dst_cols - 1);
      PW_ASSERT(src_col_idx >= 0);
      PW_ASSERT(src_col_idx < framebuffer.GetWidth());
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

FramebufferRgb565 Display::GetFramebuffer() {
  return display_driver_.GetFramebuffer();
}

Status Display::ReleaseFramebuffer(FramebufferRgb565 framebuffer) {
  if (!framebuffer.IsValid())
    return Status::InvalidArgument();
  if (framebuffer.GetWidth() != size_.width ||
      framebuffer.GetHeight() != size_.height) {
    return UpdateNearestNeighbor(framebuffer);
  }
  return display_driver_.ReleaseFramebuffer(std::move(framebuffer));
}

}  // namespace pw::display
