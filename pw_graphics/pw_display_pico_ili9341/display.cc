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

#include <stdio.h>
#include <stdlib.h>

#include <cinttypes>
#include <cstdint>

#include "pw_display/display_backend.h"

#define LIB_CMSIS_CORE 0
#define LIB_PICO_STDIO_SEMIHOSTING 0

#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include "pw_log/log.h"

namespace pw::display::backend {

namespace {

// Pico spi0 Pins
#define SPI_PORT spi0
constexpr int TFT_SCLK = 18;  // SPI0 SCK
constexpr int TFT_MOSI = 19;  // SPI0 TX
// Unused
// constexpr int TFT_MISO = 4;   // SPI0 RX
constexpr int TFT_CS = 9;    // SPI0 CSn
constexpr int TFT_DC = 10;   // GP10
constexpr int TFT_RST = 11;  // GP11

constexpr uint32_t kBaudRate = 31'250'000;

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

}  // namespace

Display::SpiValues::SpiValues(pw::spi::Config config,
                              pw::spi::ChipSelector& selector,
                              pw::sync::VirtualMutex& initiator_mutex)
    : initiator_(SPI_PORT, kBaudRate),
      borrowable_initiator_(initiator_, initiator_mutex),
      device_(borrowable_initiator_, config, selector) {}

Display::Display()
    : chip_selector_gpio_(TFT_CS),
      data_cmd_gpio_(TFT_DC),
      reset_gpio_(TFT_RST),
      spi_chip_selector_(chip_selector_gpio_),
      spi_8_bit_(kSpiConfig8Bit, spi_chip_selector_, spi_initiator_mutex_),
      spi_16_bit_(kSpiConfig16Bit, spi_chip_selector_, spi_initiator_mutex_),
      display_driver_({
          .data_cmd_gpio = data_cmd_gpio_,
          .reset_gpio = &reset_gpio_,
          .spi_device_8_bit = spi_8_bit_.device_,
          .spi_device_16_bit = spi_16_bit_.device_,
      }) {}

Display::~Display() = default;

Status Display::Init() {
  InitGPIO();
  InitSPI();
  return display_driver_.Init();
}

void Display::Update(pw::framebuffer::FramebufferRgb565& frame_buffer) {
  display_driver_.Update(&frame_buffer).IgnoreError();
}

void Display::UpdatePixelDouble(
    pw::framebuffer::FramebufferRgb565* frame_buffer) {
  display_driver_.UpdatePixelDouble(frame_buffer).IgnoreError();
}

Status Display::InitFramebuffer(
    pw::framebuffer::FramebufferRgb565* framebuffer) {
  framebuffer->SetFramebufferData(framebuffer_data_,
                                  kDisplayWidth,
                                  kDisplayHeight,
                                  kDisplayWidth * sizeof(uint16_t));
  return OkStatus();
}

void Display::InitGPIO() {
  stdio_init_all();
  // TODO: This should be a facade
  setup_default_uart();

  chip_selector_gpio_.Enable();
  data_cmd_gpio_.Enable();
  reset_gpio_.Enable();
}

void Display::InitSPI() {
  uint actual_baudrate = spi_init(SPI_PORT, kBaudRate);
  PW_LOG_DEBUG("Actual Baudrate: %u", actual_baudrate);

  // Not currently used (not yet reading from display).
  // gpio_set_function(TFT_MISO, GPIO_FUNC_SPI);
  gpio_set_function(TFT_SCLK, GPIO_FUNC_SPI);
  gpio_set_function(TFT_MOSI, GPIO_FUNC_SPI);
}

}  // namespace pw::display::backend
