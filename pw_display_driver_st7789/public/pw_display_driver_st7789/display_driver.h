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

namespace pw::display_driver {

class DisplayDriverST7789 : public DisplayDriver {
 public:
  // DisplayDriverST7789 configuration parameters.
  struct Config {
    // The GPIO line to use when specifying data/command mode for the display
    // controller.
    pw::digital_io::DigitalOut& data_cmd_gpio;
    // GPIO line to reset the display controller.
    pw::digital_io::DigitalOut* reset_gpio;
    pw::digital_io::DigitalIn* tear_effect_gpio;
    // The SPI device to which the display controller is connected for 8-bit
    // data.
    pw::spi::Device& spi_device_8_bit;
    // The SPI device to which the display controller is connected for 16-bit
    // data.
    pw::spi::Device& spi_device_16_bit;
    uint16_t screen_width = 320;
    uint16_t screen_height = 240;
  };

  DisplayDriverST7789(const Config& config);

  // DisplayDriver implementation:
  Status Init() override;
  void WriteFramebuffer(pw::framebuffer::Framebuffer framebuffer,
                        WriteCallback write_callback) override;
  Status WriteRow(span<uint16_t> row_pixels,
                  uint16_t row_idx,
                  uint16_t col_idx) override;
  uint16_t GetWidth() const override { return config_.screen_width; }
  uint16_t GetHeight() const override { return config_.screen_height; }

 private:
  enum class Mode {
    kData,
    kCommand,
  };

  // A command and optional data to write to the ST7789.
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

  Config config_;
};

}  // namespace pw::display_driver
