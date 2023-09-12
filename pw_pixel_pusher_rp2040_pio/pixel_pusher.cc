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

#include "pw_pixel_pusher_rp2040_pio/pixel_pusher.h"

#include <array>
#include <cstddef>

#include "hardware/dma.h"
#include "hardware/pio.h"
#include "hardware/spi.h"
#include "pw_digital_io/digital_io.h"
#include "pw_pixel_pusher_rp2040_pio/st7789.pio.h"

using pw::Status;
using pw::framebuffer::Framebuffer;
using pw::framebuffer_pool::FramebufferPool;

namespace pw::pixel_pusher {

namespace {

Framebuffer s_framebuffer;
Callback<void(Framebuffer, Status)> s_draw_callback;
static uint dma_channel;
static volatile int current_scanline = 240;
static volatile int irq_fire_count;

static void __isr irq_handler() {
  irq_fire_count++;
  // Write was active, just finished
  if (dma_channel_get_irq0_status(dma_channel)) {
    dma_channel_acknowledge_irq0(dma_channel);

    // * 2 for pixel doubling
    uint16_t fb_width = s_framebuffer.size().width * 2;
    uint16_t fb_height = s_framebuffer.size().height * 2;

    if (++current_scanline > fb_height / 2) {
      // All scanlines written. This frame is done.
      if (s_draw_callback != nullptr) {
        s_draw_callback(std::move(s_framebuffer), pw::OkStatus());
        s_draw_callback = nullptr;
      }
      return;
    }

    auto count =
        current_scanline == (fb_height + 1) / 2 ? fb_width / 4 : fb_width / 2;

    dma_channel_set_trans_count(dma_channel,
                                /* trans_count= */ count,
                                /* trigger= */ false);
    dma_channel_set_read_addr(
        dma_channel,
        /* read_addr= */ static_cast<const uint16_t*>(s_framebuffer.data()) +
            (current_scanline - 1) * (fb_width / 2),
        /* trigger= */ true);
  }
}

// PIO helpers
static void pio_put_byte(PIO pio, uint sm, uint8_t b) {
  while (pio_sm_is_tx_fifo_full(pio, sm))
    ;
  *(volatile uint8_t*)&pio->txf[sm] = b;
}

static void pio_wait(PIO pio, uint sm) {
  uint32_t stall_mask = 1u << (PIO_FDEBUG_TXSTALL_LSB + sm);
  pio->fdebug |= stall_mask;
  while (!(pio->fdebug & stall_mask))
    ;
}

}  // namespace

PixelPusherRp2040Pio::PixelPusherRp2040Pio(
    int dc_pin, int cs_pin, int dout_pin, int sck_pin, int te_pin, PIO pio)
    : dc_pin_(dc_pin),
      cs_pin_(cs_pin),
      dout_pin_(dout_pin),
      sck_pin_(sck_pin),
      te_pin_(te_pin),
      pio_(pio) {}
PixelPusherRp2040Pio::~PixelPusherRp2040Pio() = default;

Status PixelPusherRp2040Pio::Init(const FramebufferPool& framebuffer_pool) {
  const FramebufferPool::BufferArray& buffers =
      framebuffer_pool.GetBuffersForInit();
  if (buffers.empty()) {
    return Status::Internal();
  }

  // PIO Setup
  pio_offset_ = pio_add_program(pio_, /* program= */ &st7789_raw_program);
  pio_double_offset_ =
      pio_add_program(pio_, /* program= */ &st7789_pixel_double_program);

  pio_sm_ = pio_claim_unused_sm(pio_, true);

  pio_sm_config pio_config = st7789_raw_program_get_default_config(pio_offset_);

#if OVERCLOCK_250
  sm_config_set_clkdiv(&pio_config, 2);  // Clock divide by 2 for 62.5MHz
#endif

  sm_config_set_out_shift(&pio_config,
                          /* shift_right= */ false,
                          /* autopull= */ true,
                          /* pull_threshold= */ 8);
  sm_config_set_out_pins(
      &pio_config, /* out_base= */ dout_pin_, /* out_count= */ 1);
  sm_config_set_fifo_join(&pio_config, /* pio_fifo_join= */ PIO_FIFO_JOIN_TX);
  sm_config_set_sideset_pins(&pio_config, /* sideset_base= */ sck_pin_);

  pio_gpio_init(pio_, /* pin= */ dout_pin_);
  pio_gpio_init(pio_, /* pin= */ sck_pin_);
  pio_sm_set_consecutive_pindirs(pio_,
                                 pio_sm_,
                                 /* pin_base= */ dout_pin_,
                                 /* pin_count= */ 1,
                                 /* is_out= */ true);
  pio_sm_set_consecutive_pindirs(pio_,
                                 pio_sm_,
                                 /* pin_base= */ sck_pin_,
                                 /* pin_count= */ 1,
                                 /* is_out= */ true);

  pio_sm_init(pio_, pio_sm_, /* initial_pc= */ pio_offset_, &pio_config);
  pio_sm_set_enabled(pio_, pio_sm_, /* enabled= */ true);

  // DMA Setup
  dma_channel_ = dma_claim_unused_channel(true);
  dma_channel_config config = dma_channel_get_default_config(dma_channel_);
  channel_config_set_transfer_data_size(
      &config, /* dma_channel_transfer_size= */ DMA_SIZE_16);
  // DMA byte swapping: off
  channel_config_set_bswap(&config, /* bswap= */ false);
  // Set tranfer request signal to a dreq (data request).
  channel_config_set_dreq(
      &config,
      /* dreq= */ pio_get_dreq(pio_, pio_sm_, /* is_tx= */ true));
  dma_channel_configure(dma_channel_,
                        &config,
                        /* write_addr= */ &pio_->txf[pio_sm_],
                        /* read_addr= */ NULL,    // framebuffer
                        /* transfer_count= */ 0,  // width * height
                        /* trigger= */ false);

  irq_add_shared_handler(
      /* num= */ DMA_IRQ_0,
      /* handler= */ irq_handler,
      /* order_priority= */ PICO_SHARED_IRQ_HANDLER_DEFAULT_ORDER_PRIORITY);
  irq_set_enabled(DMA_IRQ_0, /* enabled= */ true);

  return OkStatus();
}

void PixelPusherRp2040Pio::WriteFramebuffer(
    framebuffer::Framebuffer framebuffer,
    Callback<void(framebuffer::Framebuffer, Status)> complete_callback) {
  // If this should be non-blocking
  // if (dma_channel_is_busy(dma_channel)) {
  //   return;
  // }

  // If not vsync
  while (DmaIsBusy()) {
  }

  dma_channel_wait_for_finish_blocking(dma_channel);

  if (!write_mode_) {
    SetupWriteFramebuffer();
  }

  PW_ASSERT(s_draw_callback == nullptr);
  s_draw_callback = std::move(complete_callback);
  s_framebuffer = std::move(framebuffer);
  dma_channel = dma_channel_;

  const uint16_t* fb_data = static_cast<const uint16_t*>(s_framebuffer.data());
  int fb_width = s_framebuffer.size().width;
  int fb_height = s_framebuffer.size().height;

  if (pixel_double_enabled_) {
    fb_width *= 2;
    fb_height *= 2;
    current_scanline = 0;
    irq_fire_count = 0;
    dma_channel_set_trans_count(dma_channel_, fb_width / 4, false);
  } else {
    dma_channel_set_trans_count(dma_channel_, fb_width * fb_height, false);
  }

  dma_channel_set_read_addr(dma_channel_, fb_data, true);
}

void PixelPusherRp2040Pio::SetPixelDouble(bool enabled) {
  pixel_double_enabled_ = enabled;

  if (pixel_double_enabled_) {
    dma_channel_acknowledge_irq0(dma_channel);
    dma_channel_set_irq0_enabled(dma_channel, true);
  } else {
    dma_channel_set_irq0_enabled(dma_channel, false);
  }
}

bool PixelPusherRp2040Pio::DmaIsBusy() {
  if (pixel_double_enabled_ &&
      current_scanline <= (s_framebuffer.size().height * 2) / 2) {
    return true;
  }
  return dma_channel_is_busy(dma_channel);
}

void PixelPusherRp2040Pio::Clear() {
  if (!write_mode_)
    SetupWriteFramebuffer();

  int fb_width = s_framebuffer.size().width;
  int fb_height = s_framebuffer.size().height;
  if (pixel_double_enabled_) {
    fb_width *= 2;
    fb_height *= 2;
  }

  for (int i = 0; i < fb_width * fb_height; i++)
    pio_sm_put_blocking(pio_, pio_sm_, 0);
}

bool PixelPusherRp2040Pio::VsyncCallback(gpio_irq_callback_t callback) {
#if DISPLAY_TE_GPIO != -1
  gpio_set_irq_enabled_with_callback(
      te_pin_, GPIO_IRQ_EDGE_RISE, true, callback);
  return true;
#else
  return false;
#endif
}

void PixelPusherRp2040Pio::SetupWriteFramebuffer() {
  pio_wait(pio_, pio_sm_);

  gpio_put(cs_pin_, 0);

  // Enter command mode.
  gpio_put(dc_pin_, 0);
  // Tell the display a framebuffer is coming next.
  pio_put_byte(pio_, pio_sm_, 0x2C);  // ST7789_RAMWR
  pio_wait(pio_, pio_sm_);

  // Enter data mode.
  gpio_put(dc_pin_, 1);

  pio_sm_set_enabled(pio_, pio_sm_, false);
  pio_sm_restart(pio_, pio_sm_);

  if (pixel_double_enabled_) {
    // Switch PIO to the pixel double program.
    pio_sm_set_wrap(pio_,
                    pio_sm_,
                    pio_double_offset_ + st7789_pixel_double_wrap_target,
                    pio_double_offset_ + st7789_pixel_double_wrap);

    pio_->sm[pio_sm_].shiftctrl &=
        ~(PIO_SM0_SHIFTCTRL_PULL_THRESH_BITS | PIO_SM0_SHIFTCTRL_AUTOPULL_BITS);

    pio_sm_exec(pio_, pio_sm_, pio_encode_jmp(pio_double_offset_));

    dma_channel_hw_addr(dma_channel_)->al1_ctrl &=
        ~DMA_CH0_CTRL_TRIG_DATA_SIZE_BITS;
    dma_channel_hw_addr(dma_channel_)->al1_ctrl |=
        DMA_SIZE_32 << DMA_CH0_CTRL_TRIG_DATA_SIZE_LSB;
  } else {
    pio_->sm[pio_sm_].shiftctrl &= ~PIO_SM0_SHIFTCTRL_PULL_THRESH_BITS;
    pio_->sm[pio_sm_].shiftctrl |= (16 << PIO_SM0_SHIFTCTRL_PULL_THRESH_LSB) |
                                   PIO_SM0_SHIFTCTRL_AUTOPULL_BITS;

    dma_channel_hw_addr(dma_channel_)->al1_ctrl &=
        ~DMA_CH0_CTRL_TRIG_DATA_SIZE_BITS;
    dma_channel_hw_addr(dma_channel_)->al1_ctrl |=
        DMA_SIZE_16 << DMA_CH0_CTRL_TRIG_DATA_SIZE_LSB;
  }

  pio_sm_set_enabled(pio_, pio_sm_, true);

  write_mode_ = true;
}

}  // namespace pw::pixel_pusher
