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

#include <stdio.h>
#include <stdlib.h>

#include <cinttypes>
#include <cstdint>

#define LIB_CMSIS_CORE 0
#define LIB_PICO_STDIO_USB 0
#define LIB_PICO_STDIO_SEMIHOSTING 0

#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include "pw_color/color.h"
#include "pw_digital_io_pico/digital_io.h"
#include "pw_display_driver_ili9341/display_driver.h"
#include "pw_framebuffer/rgb565.h"
#include "pw_log/log.h"
#include "pw_spi_pico/chip_selector.h"
#include "pw_spi_pico/initiator.h"
#include "pw_spin_delay/delay.h"
#include "pw_sync/borrow.h"
#include "pw_sync/mutex.h"

using pw::display_driver::DisplayDriverILI9341;

namespace pw::display {

namespace {

// Pico spi0 Pins
#define SPI_PORT spi0
constexpr int TFT_SCLK = 18;  // SPI0 SCK
constexpr int TFT_MOSI = 19;  // SPI0 TX
// Unused
// const int TFT_MISO = 4;   // SPI0 RX
constexpr int TFT_CS = 9;    // SPI0 CSn
constexpr int TFT_DC = 10;   // GP10
constexpr int TFT_RST = 11;  // GP11

constexpr int kDisplayWidth = 320;
constexpr int kDisplayHeight = 240;
constexpr int kNumDisplayPixels = kDisplayWidth * kDisplayHeight;
constexpr uint32_t kBaudRate = 31'250'000;

constexpr pw::spi::Config kSpiConfig{
    .polarity = pw::spi::ClockPolarity::kActiveHigh,
    .phase = pw::spi::ClockPhase::kFallingEdge,
    .bits_per_word = pw::spi::BitsPerWord(8),
    .bit_order = pw::spi::BitOrder::kMsbFirst,
};

class InstanceData {
 public:
  InstanceData()
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
    stdio_init_all();

    chip_selector_gpio_.Enable();
    data_cmd_gpio_.Enable();
    reset_gpio_.Enable();
  }

  void InitSPI() {
    uint actual_baudrate = spi_init(SPI_PORT, kBaudRate);
    PW_LOG_DEBUG("Actual Baudrate: %u", actual_baudrate);

    // Not currently used (not yet reading from display).
    // gpio_set_function(TFT_MISO, GPIO_FUNC_SPI);
    gpio_set_function(TFT_SCLK, GPIO_FUNC_SPI);
    gpio_set_function(TFT_MOSI, GPIO_FUNC_SPI);
  }

  Status InitDisplayDriver() {
    auto s = display_driver_.Init();
    if (!s.ok())
      return s;
    // From hereafter only display pixel updates are made, so switch to 16-bit
    // mode which is expected by DisplayDriver::Update();
    // TODO(b/251033990): Switch to pw_spi way to change word size.
    spi_initiator_.SetOverrideBitsPerWord(pw::spi::BitsPerWord(16));
    return OkStatus();
  }

  pw::digital_io::PicoDigitalOut chip_selector_gpio_;
  pw::digital_io::PicoDigitalOut data_cmd_gpio_;
  pw::digital_io::PicoDigitalOut reset_gpio_;
  pw::spi::PicoChipSelector spi_chip_selector_;
  pw::spi::PicoInitiator spi_initiator_;
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
