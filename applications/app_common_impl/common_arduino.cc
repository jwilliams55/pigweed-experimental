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
using pw::framebuffer::FramebufferRgb565;
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

constexpr int kNumPixels = LCD_WIDTH * LCD_HEIGHT;
constexpr int kDisplayRowBytes = sizeof(uint16_t) * LCD_WIDTH;

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

DigitalOut s_display_dc_pin(TFT_DC);
#if TFT_RST != -1
DigitalOut s_display_reset_pin(TFT_RST);
#endif
DigitalOut s_display_cs_pin(TFT_CS);
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
DisplayDriver s_display_driver({
  .data_cmd_gpio = s_display_dc_pin,
#if TFT_RST != -1
  .reset_gpio = &s_display_reset_pin,
#else
  .reset_gpio = nullptr,
#endif
  .spi_device_8_bit = s_spi_8_bit.device,
  .spi_device_16_bit = s_spi_16_bit.device,
});
uint16_t s_pixel_data[kNumPixels];
Display s_display(
    FramebufferRgb565(s_pixel_data, LCD_WIDTH, LCD_HEIGHT, kDisplayRowBytes),
    s_display_driver);

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
#if TFT_RST != -1
  s_display_reset_pin.Enable();
#endif

  SPI.begin();

  PW_TRY(s_display_driver.Init());

  return s_display.Init();
}

// static
pw::display::Display& Common::GetDisplay() { return s_display; }
