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

#include "app_common/common.h"
#include "pw_digital_io_arduino/digital_io.h"
#include "pw_display_driver_ili9341/display_driver.h"
#include "pw_spi_arduino/chip_selector.h"
#include "pw_spi_arduino/initiator.h"
#include "pw_sync/borrow.h"
#include "pw_sync/mutex.h"

using pw::Status;
using pw::display::Display;
using DisplayDriver = pw::display_driver::DisplayDriverILI9341;
using pw::framebuffer::Framebuffer;
using pw::spi::Device;
using pw::spi::Initiator;
using pw::sync::Borrowable;
using pw::sync::VirtualMutex;
using DigitalOut = pw::digital_io::ArduinoDigitalOut;
using SpiChipSelector = pw::spi::ArduinoChipSelector;
using SpiInitiator = pw::spi::ArduinoInitiator;

namespace {

struct SpiValues {
  SpiValues(pw::spi::Config config,
            pw::spi::ChipSelector& selector,
            pw::sync::VirtualMutex& initiator_mutex);

  pw::spi::ArduinoInitiator initiator;
  pw::sync::Borrowable<pw::spi::Initiator> borrowable_initiator;
  pw::spi::Device device;
};

static_assert(DISPLAY_WIDTH > 0);
static_assert(DISPLAY_HEIGHT > 0);

constexpr uint16_t kFramebufferWidth =
    FRAMEBUFFER_WIDTH >= 0 ? FRAMEBUFFER_WIDTH : DISPLAY_WIDTH;
constexpr uint16_t kFramebufferHeight = DISPLAY_HEIGHT;

constexpr pw::math::Size<uint16_t> kDisplaySize = {DISPLAY_WIDTH,
                                                   DISPLAY_HEIGHT};
constexpr size_t kNumPixels = kFramebufferWidth * kFramebufferHeight;
constexpr uint16_t kDisplayRowBytes = sizeof(uint16_t) * kFramebufferWidth;

constexpr pw::spi::Config kSpiConfig8Bit{
    .polarity = pw::spi::ClockPolarity::kActiveHigh,
    .phase = pw::spi::ClockPhase::kFallingEdge,
    .bits_per_word = pw::spi::BitsPerWord(8),
    .bit_order = pw::spi::BitOrder::kMsbFirst,
};
constexpr pw::spi::Config kSpiConfig16Bit{
    .polarity = pw::spi::ClockPolarity::kActiveHigh,
    .phase = pw::spi::ClockPhase::kFallingEdge,
    .bits_per_word = pw::spi::BitsPerWord(16),
    .bit_order = pw::spi::BitOrder::kMsbFirst,
};

DigitalOut s_display_dc_pin(DISPLAY_DC_GPIO);
#if DISPLAY_RESET_GPIO != -1
DigitalOut s_display_reset_pin(DISPLAY_RESET_GPIO);
#endif
DigitalOut s_display_cs_pin(DISPLAY_CS_GPIO);
SpiChipSelector s_spi_chip_selector(s_display_cs_pin);
SpiInitiator s_spi_initiator;
VirtualMutex s_spi_initiator_mutex;
Borrowable<Initiator> s_borrowable_spi_initiator(s_spi_initiator,
                                                 s_spi_initiator_mutex);
SpiValues s_spi_8_bit(kSpiConfig8Bit,
                      s_spi_chip_selector,
                      s_spi_initiator_mutex);
SpiValues s_spi_16_bit(kSpiConfig16Bit,
                       s_spi_chip_selector,
                       s_spi_initiator_mutex);
uint16_t s_pixel_data[kNumPixels];
constexpr pw::framebuffer::pool::PoolData s_fb_pool_data = {
    .fb_addr =
        {
            s_pixel_data,
            nullptr,
            nullptr,
        },
    .num_fb = 1,
    .size = {kFramebufferWidth, kFramebufferHeight},
    .row_bytes = kDisplayRowBytes,
    .start = {0, 0},
};
DisplayDriver s_display_driver({
  .data_cmd_gpio = s_display_dc_pin,
#if DISPLAY_RESET_GPIO != -1
  .reset_gpio = &s_display_reset_pin,
#else
  .reset_gpio = nullptr,
#endif
  .spi_device_8_bit = s_spi_8_bit.device,
  .spi_device_16_bit = s_spi_16_bit.device, .pool_data = s_fb_pool_data,
});
Display s_display(s_display_driver, kDisplaySize);

SpiValues::SpiValues(pw::spi::Config config,
                     pw::spi::ChipSelector& selector,
                     pw::sync::VirtualMutex& initiator_mutex)
    : borrowable_initiator(initiator, initiator_mutex),
      device(borrowable_initiator, config, selector) {}

}  // namespace

// static
Status Common::Init() {
  s_display_cs_pin.Enable();
  s_display_dc_pin.Enable();
#if DISPLAY_RESET_GPIO != -1
  s_display_reset_pin.Enable();
#endif

  SPI.begin();

  return s_display_driver.Init();
}

// static
pw::display::Display& Common::GetDisplay() { return s_display; }
