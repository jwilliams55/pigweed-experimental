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

#include <cinttypes>
#include <cstdint>

#include "pw_display/display_backend.h"

using pw::framebuffer::FramebufferRgb565;

namespace pw::display::backend {

namespace {

constexpr int TFT_DC = 9;
constexpr int TFT_CS = 32;
constexpr int TFT_RST = 3;

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
    : borrowable_initiator_(initiator_, initiator_mutex),
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

void Display::InitGPIO() {
  chip_selector_gpio_.Enable();
  data_cmd_gpio_.Enable();
  reset_gpio_.Enable();
}

void Display::InitSPI() { SPI.begin(); }

Status Display::Init() {
  InitGPIO();
  InitSPI();
  return display_driver_.Init();
}

void Display::Update(FramebufferRgb565& frame_buffer) {
  display_driver_.Update(&frame_buffer);
}

Status Display::InitFramebuffer(FramebufferRgb565* framebuffer) {
  framebuffer->SetFramebufferData(framebuffer_data_,
                                  kDisplayWidth,
                                  kDisplayHeight,
                                  kDisplayWidth * sizeof(uint16_t));
  return OkStatus();
}

bool Display::TouchscreenAvailable() const { return false; }

bool Display::NewTouchEvent() { return false; }

pw::coordinates::Vec3Int Display::GetTouchPoint() {
  return pw::coordinates::Vec3Int{0, 0, 0};
}

}  // namespace pw::display::backend
