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
#include <cstdint>

#include "app_common/common.h"
#include "pw_digital_io_stm32cube/digital_io.h"
#include "pw_display_driver_ili9341/display_driver.h"
#include "pw_spi_stm32cube/chip_selector.h"
#include "pw_spi_stm32cube/initiator.h"
#include "pw_sync/borrow.h"
#include "pw_sync/mutex.h"

using pw::Status;
using pw::digital_io::Stm32CubeDigitalOut;
using pw::display::Display;
using pw::display_driver::DisplayDriverILI9341;
using pw::framebuffer::PixelFormat;
using pw::framebuffer_pool::FramebufferPool;
using pw::spi::Device;
using pw::spi::Initiator;
using pw::spi::Stm32CubeChipSelector;
using pw::spi::Stm32CubeInitiator;
using pw::sync::Borrowable;
using pw::sync::VirtualMutex;

namespace {

#define _CAT(A, B) A##B
#define CAT(A, B) _CAT(A, B)

#define LCD_CS_PORT CAT(GPIO, LCD_CS_PORT_CHAR)
#define LCD_CS_PIN CAT(GPIO_PIN_, LCD_CS_PIN_NUM)

#define LCD_DC_PORT CAT(GPIO, LCD_DC_PORT_CHAR)
#define LCD_DC_PIN CAT(GPIO_PIN_, LCD_DC_PIN_NUM)

struct SpiValues {
  SpiValues(pw::spi::Config config,
            pw::spi::ChipSelector& selector,
            pw::sync::VirtualMutex& initiator_mutex);

  pw::spi::Stm32CubeInitiator initiator;
  pw::sync::Borrowable<pw::spi::Initiator> borrowable_initiator;
  pw::spi::Device device;
};

static_assert(DISPLAY_WIDTH > 0);
static_assert(DISPLAY_HEIGHT > 0);

constexpr uint16_t kFramebufferWidth =
    FRAMEBUFFER_WIDTH >= 0 ? FRAMEBUFFER_WIDTH : DISPLAY_WIDTH;
constexpr uint16_t kFramebufferHeight = DISPLAY_HEIGHT;

constexpr size_t kNumPixels = kFramebufferWidth * kFramebufferHeight;
constexpr uint16_t kDisplayRowBytes = sizeof(uint16_t) * kFramebufferWidth;
constexpr pw::math::Size<uint16_t> kDisplaySize = {DISPLAY_WIDTH,
                                                   DISPLAY_HEIGHT};
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

Stm32CubeDigitalOut s_display_dc_pin(LCD_DC_PORT, LCD_DC_PIN);
Stm32CubeDigitalOut s_display_cs_pin(LCD_CS_PORT, LCD_CS_PIN);
Stm32CubeChipSelector s_spi_chip_selector(s_display_cs_pin);
Stm32CubeInitiator s_spi_initiator;
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
pw::framebuffer_pool::FramebufferPool s_fb_pool({
    .fb_addr = {s_pixel_data},
    .dimensions = {kFramebufferWidth, kFramebufferHeight},
    .row_bytes = kDisplayRowBytes,
    .pixel_format = PixelFormat::RGB565,
});
DisplayDriverILI9341 s_display_driver({
    .data_cmd_gpio = s_display_dc_pin,
    .reset_gpio = nullptr,
    .spi_device_8_bit = s_spi_8_bit.device,
    .spi_device_16_bit = s_spi_16_bit.device,
});
Display s_display(s_display_driver, kDisplaySize, s_fb_pool);

SpiValues::SpiValues(pw::spi::Config config,
                     pw::spi::ChipSelector& selector,
                     pw::sync::VirtualMutex& initiator_mutex)
    : borrowable_initiator(initiator, initiator_mutex),
      device(borrowable_initiator, config, selector) {}

void InitSPIPins() {
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

}  // namespace

// static
Status Common::Init() {
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();

  __HAL_RCC_SPI5_CLK_ENABLE();

  s_display_cs_pin.Enable();
  s_display_dc_pin.Enable();

  InitSPIPins();

  return s_display_driver.Init();
}

// static
pw::display::Display& Common::GetDisplay() { return s_display; }
