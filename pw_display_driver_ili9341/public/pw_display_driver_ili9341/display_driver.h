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

#include <cstddef>

#include "pw_digital_io/digital_io.h"
#include "pw_display_driver/display_driver.h"
#include "pw_spi/device.h"

namespace pw {
namespace display_driver {

class DisplayDriverILI9341 : public DisplayDriver {
 public:
  // DisplayDriverILI9341 configuration parameters.
  struct Config {
    // The GPIO line to use when specifying data/command mode for the display
    // controller.
    pw::digital_io::DigitalOut& data_cmd_gpio;
    // Optional GPIO line to reset the display controller.
    pw::digital_io::DigitalOut* reset_gpio;
    // The SPI device to which the display controller is connected.
    pw::spi::Device& spi_device;
  };

  DisplayDriverILI9341(const Config& config);

  // DisplayDriver implementation:
  // The SPI bus must be in 8-bit mode before calling this function.
  Status Init() override;
  int GetWidth() const override;
  int GetHeight() const override;
  // The SPI bus must be in 16-bit mode before calling this function.
  Status Update(pw::framebuffer::FramebufferRgb565* framebuffer);
  Status UpdatePixelDouble(pw::framebuffer::FramebufferRgb565* frame_buffer);

 private:
  enum class Mode {
    kData,
    kCommand,
  };

  // A command and optional data to write to the ILI9341.
  struct Command {
    uint8_t command;
    ConstByteSpan command_data;
  };

  // Toggle the reset GPIO line to reset the display controller.
  Status Reset();

  // Set the command/data mode of the display controller.
  void SetMode(Mode mode);
  // Write the command to the display controller.
  Status WriteCommand(pw::spi::Device::Transaction& transaction,
                      const Command& command);

  pw::digital_io::DigitalOut& data_cmd_gpio_;  // Pin to specify D/CX mode.
  pw::digital_io::DigitalOut* reset_gpio_;     // Optional to reset controller.
  pw::spi::Device& spi_device_;  // SPI device connected to controller.
};

}  // namespace display_driver
}  // namespace pw
