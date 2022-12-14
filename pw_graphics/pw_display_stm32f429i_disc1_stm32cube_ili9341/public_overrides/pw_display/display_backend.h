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
#pragma once

#include <cstdint>

#include "pw_digital_io_stm32cube/digital_io.h"
#include "pw_display/display.h"
#include "pw_display_driver_ili9341/display_driver.h"
#include "pw_spi_stm32f429i_disc1_stm32cube/chip_selector.h"
#include "pw_spi_stm32f429i_disc1_stm32cube/initiator.h"
#include "pw_sync/borrow.h"
#include "pw_sync/mutex.h"

namespace pw::display::backend {

constexpr int kILI9341Width = 320;
constexpr int kILI9341Height = 240;

#if 1
constexpr int kDisplayWidth = kILI9341Width;
constexpr int kDisplayHeight = kILI9341Height;
constexpr int kScaleFactor = 1;
#else
constexpr int kDisplayWidth = kILI9341Width / 2;
constexpr int kDisplayHeight = kILI9341Height / 2;
constexpr int kScaleFactor = 2;
#endif
constexpr int kNumDisplayPixels = kDisplayWidth * kDisplayHeight;

class Display : public pw::display::Display {
 public:
  Display();
  virtual ~Display();

  // pw::display::Display implementation:
  Status Init() override;
  Status InitFramebuffer(
      pw::framebuffer::FramebufferRgb565* framebuffer) override;
  int GetWidth() const override { return kDisplayWidth; }
  int GetHeight() const override { return kDisplayHeight; }
  void Update(pw::framebuffer::FramebufferRgb565& framebuffer) override;
  bool TouchscreenAvailable() const override;
  bool NewTouchEvent() override;
  pw::coordinates::Vec3Int GetTouchPoint() override;

 private:
  struct SpiValues {
    SpiValues(pw::spi::Config config,
              pw::spi::ChipSelector& selector,
              pw::sync::VirtualMutex& initiator_mutex);

    pw::spi::Stm32CubeInitiator initiator_;
    pw::sync::Borrowable<pw::spi::Initiator> borrowable_initiator_;
    pw::spi::Device device_;
  };

  void InitGPIO();
  void InitSPI();

  pw::digital_io::Stm32CubeDigitalOut chip_selector_gpio_;
  pw::digital_io::Stm32CubeDigitalOut data_cmd_gpio_;
  pw::spi::Stm32CubeChipSelector spi_chip_selector_;
  pw::sync::VirtualMutex spi_initiator_mutex_;
  SpiValues spi_8_bit_;
  SpiValues spi_16_bit_;
  pw::display_driver::DisplayDriverILI9341 display_driver_;
  uint16_t framebuffer_data_[kNumDisplayPixels];
};

}  // namespace pw::display::backend
