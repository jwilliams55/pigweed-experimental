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
#include "pw_display_driver_ili9341/display_driver.h"
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
  constexpr static int kDisplayWidth = 320;
  constexpr static int kDisplayHeight = 240;
  constexpr static int kNumDisplayPixels = kDisplayWidth * kDisplayHeight;

  void UpdatePixelDouble(pw::framebuffer::FramebufferRgb565* frame_buffer);
  void InitGPIO();
  void InitSPI();
  Status InitDisplayDriver();

  pw::digital_io::PicoDigitalOut chip_selector_gpio_;
  pw::digital_io::PicoDigitalOut data_cmd_gpio_;
  pw::digital_io::PicoDigitalOut reset_gpio_;
  pw::spi::PicoChipSelector spi_chip_selector_;
  pw::spi::PicoInitiator spi_initiator_;
  pw::sync::VirtualMutex spi_initiator_mutex_;
  pw::sync::Borrowable<pw::spi::Initiator> borrowable_spi_initiator_;
  pw::spi::Device spi_device_;
  pw::display_driver::DisplayDriverILI9341::Config driver_config_;
  pw::display_driver::DisplayDriverILI9341 display_driver_;
  uint16_t framebuffer_data_[kNumDisplayPixels];
};

}  // namespace pw::display::backend
