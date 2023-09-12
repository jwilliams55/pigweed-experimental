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
#include <cstddef>

#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/pio.h"
#include "hardware/spi.h"
#include "pw_digital_io/digital_io.h"
#include "pw_pixel_pusher/pixel_pusher.h"
#include "pw_sync/binary_semaphore.h"

namespace pw::pixel_pusher {

class PixelPusherRp2040Pio : public PixelPusher {
 public:
  PixelPusherRp2040Pio(
      int dc_pin, int cs_pin, int dout_pin, int sck_pin_, int te_pin, PIO pio);
  ~PixelPusherRp2040Pio();

  // PixelPusher implementation:
  Status Init(
      const pw::framebuffer_pool::FramebufferPool& framebuffer_pool) override;
  void WriteFramebuffer(framebuffer::Framebuffer framebuffer,
                        WriteCallback complete_callback) override;
  bool SupportsResize() const override { return true; }

  // RP2040 PIO Functions
  void SetPixelDouble(bool enabled);
  bool DmaIsBusy();
  void SetupWriteFramebuffer();
  void Clear();
  bool VsyncCallback(gpio_irq_callback_t callback);

 private:
  bool pixel_double_enabled_ = false;
  bool write_mode_ = false;
  PIO pio_;
  uint dma_channel_;
  uint dc_pin_;
  uint cs_pin_;
  uint te_pin_;
  uint dout_pin_;
  uint sck_pin_;
  uint pio_sm_;
  uint pio_offset_;
  uint pio_double_offset_;
};

}  // namespace pw::pixel_pusher
