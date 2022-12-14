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

#include "pw_digital_io_pico/digital_io.h"
#include "pw_display/display.h"
#include "pw_display_driver_st7789/display_driver.h"
#include "pw_spi_pico/chip_selector.h"
#include "pw_spi_pico/initiator.h"
#include "pw_sync/borrow.h"
#include "pw_sync/mutex.h"

namespace pw::display::backend {

class Display : pw::display::Display {
 public:
  Display();
  virtual ~Display();

  // pw::display::Display implementation:
  Status Init() override;
  Status InitFramebuffer(
      pw::framebuffer::FramebufferRgb565* framebuffer) override;
  int GetWidth() const override;
  int GetHeight() const override;
  void Update(pw::framebuffer::FramebufferRgb565& framebuffer) override;
  bool TouchscreenAvailable() const override;
  bool NewTouchEvent() override;
  pw::coordinates::Vec3Int GetTouchPoint() override;

 private:
  struct SpiValues {
    SpiValues(pw::spi::Config config,
              pw::spi::ChipSelector& selector,
              pw::sync::VirtualMutex& initiator_mutex);

    pw::spi::PicoInitiator initiator_;
    pw::sync::Borrowable<pw::spi::Initiator> borrowable_initiator_;
    pw::spi::Device device_;
  };

  constexpr static int kDisplayWidth = 320;
  constexpr static int kDisplayHeight = 240;
  constexpr static int kNumDisplayPixels = kDisplayWidth * kDisplayHeight;

  void InitGPIO();
  void InitSPI();

  pw::digital_io::PicoDigitalOut chip_selector_gpio_;
  pw::digital_io::PicoDigitalOut data_cmd_gpio_;
  pw::spi::PicoChipSelector spi_chip_selector_;
  pw::sync::VirtualMutex spi_initiator_mutex_;
  SpiValues spi_8_bit_;
  SpiValues spi_16_bit_;
  pw::display_driver::DisplayDriverST7789 display_driver_;
  uint16_t framebuffer_data_[kNumDisplayPixels];
};

}  // namespace pw::display::backend
