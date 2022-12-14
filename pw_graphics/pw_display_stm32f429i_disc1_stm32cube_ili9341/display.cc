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

#include "pw_display/display_backend.h"

using pw::framebuffer::FramebufferRgb565;

namespace pw::display::backend {

namespace {

// CHIP SELECT PORT AND PIN.
#define LCD_CS_PORT GPIOC
#define LCD_CS_PIN GPIO_PIN_2

// DATA/COMMAND PORT AND PIN.
#define LCD_DC_PORT GPIOD
#define LCD_DC_PIN GPIO_PIN_13

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
    : chip_selector_gpio_(LCD_CS_PORT, LCD_CS_PIN),
      data_cmd_gpio_(LCD_DC_PORT, LCD_DC_PIN),
      spi_chip_selector_(chip_selector_gpio_),
      spi_8_bit_(kSpiConfig8Bit, spi_chip_selector_, spi_initiator_mutex_),
      spi_16_bit_(kSpiConfig16Bit, spi_chip_selector_, spi_initiator_mutex_),
      display_driver_({
          .data_cmd_gpio = data_cmd_gpio_,
          .reset_gpio = nullptr,
          .spi_device_8_bit = spi_8_bit_.device_,
          .spi_device_16_bit = spi_16_bit_.device_,
      }) {}

Display::~Display() = default;

void Display::InitGPIO() {
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();

  chip_selector_gpio_.Enable();
  data_cmd_gpio_.Enable();
}

void Display::InitSPI() {
  __HAL_RCC_SPI5_CLK_ENABLE();

  // SPI5 GPIO Configuration:
  // PF7 SPI5_SCK
  // PF8 SPI5_MISO
  // PF9 SPI5_MOSI
  GPIO_InitTypeDef spi_pin_config = {
      .Pin = GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9,
      .Mode = GPIO_MODE_AF_PP,
      .Pull = GPIO_NOPULL,
      .Speed = GPIO_SPEED_FREQ_VERY_HIGH,
      .Alternate = GPIO_AF5_SPI5,
  };
  HAL_GPIO_Init(GPIOF, &spi_pin_config);
}

Status Display::Init() {
  InitGPIO();
  InitSPI();
  PW_TRY(display_driver_.Init());
  return OkStatus();
}

void Display::Update(FramebufferRgb565& frame_buffer) {
  if (kScaleFactor == 1)
    display_driver_.Update(&frame_buffer);
  else
    display_driver_.UpdatePixelDouble(&frame_buffer);
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
