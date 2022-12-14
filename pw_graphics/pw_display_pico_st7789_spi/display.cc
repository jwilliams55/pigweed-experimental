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
#include "hardware/pwm.h"
#include "pico/stdlib.h"
#include "pw_log/log.h"
#include "pw_status/try.h"

namespace pw::display::backend {

namespace {

// Pico Display Pack 2 Pins
// https://shop.pimoroni.com/products/pico-display-pack-2-0
// --------------------------------------------------------
constexpr int BACKLIGHT_EN = 20;
// Pico spi0 Pins
#define SPI_PORT spi0
constexpr int TFT_SCLK = 18;  // SPI0 SCK
constexpr int TFT_MOSI = 19;  // SPI0 TX
// Unused
// constexpr int TFT_MISO = 4;   // SPI0 RX
constexpr int TFT_CS = 17;  // SPI0 CSn
constexpr int TFT_DC = 16;  // GP10
// Reset pin is connected to the Pico reset pin (RUN #30)
// constexpr int TFT_RST = 19;

constexpr uint32_t kBaudRate = 62'500'000;

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
      spi_chip_selector_(chip_selector_gpio_),
      spi_8_bit_(kSpiConfig8Bit, spi_chip_selector_, spi_initiator_mutex_),
      spi_16_bit_(kSpiConfig16Bit, spi_chip_selector_, spi_initiator_mutex_),
      display_driver_({
          .data_cmd_gpio = data_cmd_gpio_,
          .reset_gpio = nullptr,
          .spi_device_8_bit = spi_8_bit_.device_,
          .spi_device_16_bit = spi_16_bit_.device_,
          .screen_width = kDisplayWidth,
          .screen_height = kDisplayHeight,
      }) {}

Display::~Display() = default;

Status Display::Init() {
  stdio_init_all();
  // TODO: This should be a facade
  setup_default_uart();

  InitGPIO();
  InitSPI();

  // Init backlight PWM
  pwm_config cfg = pwm_get_default_config();
  pwm_set_wrap(pwm_gpio_to_slice_num(BACKLIGHT_EN), 65535);
  pwm_init(pwm_gpio_to_slice_num(BACKLIGHT_EN), &cfg, true);
  gpio_set_function(BACKLIGHT_EN, GPIO_FUNC_PWM);
  // Full Brightness
  pwm_set_gpio_level(BACKLIGHT_EN, 65535);

  PW_TRY(display_driver_.Init());

  return OkStatus();
}

void Display::Update(pw::framebuffer::FramebufferRgb565& frame_buffer) {
  display_driver_.Update(&frame_buffer);
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
  gpio_init(TFT_CS);
  gpio_init(TFT_DC);
  // gpio_init(TFT_RST); // Unused

  gpio_set_dir(TFT_CS, GPIO_OUT);
  gpio_set_dir(TFT_DC, GPIO_OUT);

  chip_selector_gpio_.Enable();
  data_cmd_gpio_.Enable();
}

void Display::InitSPI() {
  uint actual_baudrate = spi_init(SPI_PORT, kBaudRate);
  PW_LOG_DEBUG("Actual Baudrate: %u", actual_baudrate);

  // Not currently used (not yet reading from display).
  // gpio_set_function(TFT_MISO, GPIO_FUNC_SPI);
  gpio_set_function(TFT_SCLK, GPIO_FUNC_SPI);
  gpio_set_function(TFT_MOSI, GPIO_FUNC_SPI);
}

int Display::GetWidth() const { return kDisplayWidth; }

int Display::GetHeight() const { return kDisplayHeight; }

bool Display::TouchscreenAvailable() const { return false; }

bool Display::NewTouchEvent() { return false; }

pw::coordinates::Vec3Int Display::GetTouchPoint() {
  return pw::coordinates::Vec3Int{0, 0, 0};
}

}  // namespace pw::display::backend
