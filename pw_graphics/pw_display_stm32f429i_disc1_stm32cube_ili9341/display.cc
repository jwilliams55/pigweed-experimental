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

#include "pw_display/display.h"

#include <cstdint>

#include "pw_color/color.h"
#include "pw_digital_io_stm32cube/digital_io.h"
#include "pw_display_driver_ili9341/display_driver.h"
#include "pw_framebuffer/rgb565.h"
#include "pw_spi_stm32f429i_disc1_stm32cube/chip_selector.h"
#include "pw_spi_stm32f429i_disc1_stm32cube/initiator.h"
#include "pw_sync/borrow.h"
#include "pw_sync/mutex.h"
#include "stm32cube/stm32cube.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_spi.h"

using pw::display_driver::DisplayDriverILI9341;

namespace pw::display {
namespace {

// CHIP SELECT PORT AND PIN.
#define LCD_CS_PORT GPIOC
#define LCD_CS_PIN GPIO_PIN_2

// DATA/COMMAND PORT AND PIN.
#define LCD_DC_PORT GPIOD
#define LCD_DC_PIN GPIO_PIN_13

constexpr int kDisplayWidth = 320;
constexpr int kDisplayHeight = 240;
constexpr int kNumDisplayPixels = kDisplayWidth * kDisplayHeight;

constexpr pw::spi::Config kSpiConfig{
    .polarity = pw::spi::ClockPolarity::kActiveHigh,
    .phase = pw::spi::ClockPhase::kFallingEdge,
    .bits_per_word = pw::spi::BitsPerWord(8),
    .bit_order = pw::spi::BitOrder::kMsbFirst,
};

class InstanceData {
 public:
  InstanceData()
      : chip_selector_gpio_(LCD_CS_PORT, LCD_CS_PIN),
        data_cmd_gpio_(LCD_DC_PORT, LCD_DC_PIN),
        spi_chip_selector_(chip_selector_gpio_),
        borrowable_spi_initiator_(spi_initiator_, spi_initiator_mutex_),
        spi_device_(borrowable_spi_initiator_, kSpiConfig, spi_chip_selector_),
        driver_config_{
            .data_cmd_gpio = data_cmd_gpio_,
            .reset_gpio = nullptr,
            .spi_device = spi_device_,
        },
        display_driver_(driver_config_) {}

  void Init() {
    InitGPIO();
    InitSPI();
    InitDisplayDriver();
  }

  void Update(pw::framebuffer::FramebufferRgb565* frame_buffer) {
    display_driver_.Update(frame_buffer);
  }

  void UpdatePixelDouble(pw::framebuffer::FramebufferRgb565* frame_buffer) {
    display_driver_.UpdatePixelDouble(frame_buffer);
  }

 private:
  void InitGPIO() {
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

  void InitSPI() {
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

  void InitDisplayDriver() {
    display_driver_.Init();
    // From hereafter only display pixel updates are made, so switch to 16-bit
    // mode which is expected by DisplayDriver::Update();
    // TODO(b/251033990): Switch to pw_spi way to change word size.
    spi_initiator_.SetOverrideBitsPerWord(pw::spi::BitsPerWord(16));
  }

  pw::digital_io::Stm32CubeDigitalOut chip_selector_gpio_;
  pw::digital_io::Stm32CubeDigitalOut data_cmd_gpio_;
  pw::spi::Stm32CubeChipSelector spi_chip_selector_;
  pw::spi::Stm32CubeInitiator spi_initiator_;
  pw::sync::VirtualMutex spi_initiator_mutex_;
  pw::sync::Borrowable<pw::spi::Initiator> borrowable_spi_initiator_;
  pw::spi::Device spi_device_;
  DisplayDriverILI9341::Config driver_config_;
  DisplayDriverILI9341 display_driver_;
};  // namespace

InstanceData s_instance_data;

}  // namespace

void Init() { s_instance_data.Init(); }

int GetWidth() { return kDisplayWidth; }

int GetHeight() { return kDisplayHeight; }

void Update(pw::framebuffer::FramebufferRgb565* frame_buffer) {
  s_instance_data.Update(frame_buffer);
}

void UpdatePixelDouble(pw::framebuffer::FramebufferRgb565* frame_buffer) {
  s_instance_data.UpdatePixelDouble(frame_buffer);
}

bool TouchscreenAvailable() { return false; }

bool NewTouchEvent() { return false; }

pw::coordinates::Vec3Int GetTouchPoint() {
  return pw::coordinates::Vec3Int{0, 0, 0};
}

}  // namespace pw::display
