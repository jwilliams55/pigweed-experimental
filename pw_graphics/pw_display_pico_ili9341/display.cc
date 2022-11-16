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
#define LIB_PICO_STDIO_USB 0
#define LIB_PICO_STDIO_SEMIHOSTING 0

#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include "pw_log/log.h"

using pw::display_driver::DisplayDriverILI9341;

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

constexpr pw::spi::Config kSpiConfig{
    .polarity = pw::spi::ClockPolarity::kActiveHigh,
    .phase = pw::spi::ClockPhase::kFallingEdge,
    .bits_per_word = pw::spi::BitsPerWord(8),
    .bit_order = pw::spi::BitOrder::kMsbFirst,
};

}  // namespace

Display::Display()
    : chip_selector_gpio_(TFT_CS),
      data_cmd_gpio_(TFT_DC),
      reset_gpio_(TFT_RST),
      spi_chip_selector_(chip_selector_gpio_),
      spi_initiator_(SPI_PORT, kBaudRate),
      borrowable_spi_initiator_(spi_initiator_, spi_initiator_mutex_),
      spi_device_(borrowable_spi_initiator_, kSpiConfig, spi_chip_selector_),
      driver_config_{
          .data_cmd_gpio = data_cmd_gpio_,
          .reset_gpio = &reset_gpio_,
          .spi_device = spi_device_,
      },
      display_driver_(driver_config_) {}

Display::~Display() = default;

Status Display::Init() {
  InitGPIO();
  InitSPI();
  InitDisplayDriver();
  return OkStatus();
}

void Display::Update(pw::framebuffer::FramebufferRgb565& frame_buffer) {
  display_driver_.Update(&frame_buffer);
}

void Display::UpdatePixelDouble(
    pw::framebuffer::FramebufferRgb565* frame_buffer) {
  display_driver_.UpdatePixelDouble(frame_buffer);
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

Status Display::InitDisplayDriver() {
  auto s = display_driver_.Init();
  if (!s.ok())
    return s;
  // From hereafter only display pixel updates are made, so switch to 16-bit
  // mode which is expected by DisplayDriver::Update();
  // TODO(b/251033990): Switch to pw_spi way to change word size.
  spi_initiator_.SetOverrideBitsPerWord(pw::spi::BitsPerWord(16));
  return OkStatus();
}

int Display::GetWidth() const { return kDisplayWidth; }

int Display::GetHeight() const { return kDisplayHeight; }

bool Display::TouchscreenAvailable() const { return false; }

bool Display::NewTouchEvent() { return false; }

pw::coordinates::Vec3Int Display::GetTouchPoint() {
  return pw::coordinates::Vec3Int{0, 0, 0};
}

}  // namespace pw::display::backend
