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

#include "pw_display_driver_st7789/display_driver.h"

#include <array>
#include <cstddef>

#include "pw_digital_io/digital_io.h"
#include "pw_framebuffer/framebuffer.h"
#include "pw_spin_delay/delay.h"

using pw::digital_io::State;
using pw::framebuffer::Framebuffer;
using pw::framebuffer::PixelFormat;
using pw::spi::ChipSelectBehavior;
using pw::spi::Device;
using std::array;
using std::byte;

namespace pw::display_driver {

namespace {

constexpr array<byte, 0> kEmptyArray;

// ST7789 Display Registers
// clang-format off
#define ST7789_SWRESET  0x01
#define ST7789_TEOFF    0x34
#define ST7789_TEON     0x35
#define ST7789_MADCTL   0x36
#define ST7789_COLMOD   0x3A
#define ST7789_GCTRL    0xB7
#define ST7789_VCOMS    0xBB
#define ST7789_LCMCTRL  0xC0
#define ST7789_VDVVRHEN 0xC2
#define ST7789_VRHS     0xC3
#define ST7789_VDVS     0xC4
#define ST7789_FRCTRL2  0xC6
#define ST7789_PWCTRL1  0xD0
#define ST7789_PORCTRL  0xB2
#define ST7789_GMCTRP1  0xE0
#define ST7789_GMCTRN1  0xE1
#define ST7789_INVOFF   0x20
#define ST7789_SLPOUT   0x11
#define ST7789_DISPON   0x29
#define ST7789_GAMSET   0x26
#define ST7789_DISPOFF  0x28
#define ST7789_RAMWR    0x2C
#define ST7789_INVON    0x21
#define ST7789_CASET    0x2A
#define ST7789_RASET    0x2B

// MADCTL Bits (See page 215: MADCTL (36h): Memory Data Access Control)
#define ST7789_MADCTL_ROW_ORDER   0b10000000
#define ST7789_MADCTL_COL_ORDER   0b01000000
#define ST7789_MADCTL_SWAP_XY     0b00100000
#define ST7789_MADCTL_SCAN_ORDER  0b00010000
#define ST7789_MADCTL_RGB_BGR     0b00001000
#define ST7789_MADCTL_HORIZ_ORDER 0b00000100
// clang-format on

constexpr uint8_t HighByte(uint16_t val) { return val >> 8; }

constexpr uint8_t LowByte(uint16_t val) { return val & 0xff; }

}  // namespace

DisplayDriverST7789::DisplayDriverST7789(const Config& config)
    : config_(config) {}

void DisplayDriverST7789::SetMode(Mode mode) {
  // Set the D/CX pin to indicate data or command values.
  if (mode == Mode::kData) {
    // Set data command pin to 1
    config_.data_cmd_gpio.SetState(State::kActive);
  } else {
    // Set data command pin to 0
    config_.data_cmd_gpio.SetState(State::kInactive);
  }
}

Status DisplayDriverST7789::WriteCommand(Device::Transaction& transaction,
                                         const Command& command) {
  SetMode(Mode::kCommand);
  byte buff[1]{static_cast<byte>(command.command)};
  auto s = transaction.Write(buff);
  if (!s.ok())
    return s;

  SetMode(Mode::kData);
  if (command.command_data.empty()) {
    return OkStatus();
  }
  return transaction.Write(command.command_data);
}

Status DisplayDriverST7789::Init() {
  auto transaction = config_.spi_device_8_bit.StartTransaction(
      ChipSelectBehavior::kPerWriteRead);

  WriteCommand(transaction, {ST7789_SWRESET, kEmptyArray});  // Software reset
  pw::spin_delay::WaitMillis(150);

  WriteCommand(transaction, {ST7789_TEON, kEmptyArray});
  WriteCommand(transaction, {ST7789_COLMOD, array<byte, 1>{byte{0x05}}});

  WriteCommand(transaction,
               {ST7789_PORCTRL,
                array<byte, 5>{
                    byte{0x0c},
                    byte{0x0c},
                    byte{0x00},
                    byte{0x33},
                    byte{0x33},
                }});
  WriteCommand(transaction, {ST7789_LCMCTRL, array<byte, 1>{byte{0x2c}}});
  WriteCommand(transaction, {ST7789_VDVVRHEN, array<byte, 1>{byte{0x01}}});
  WriteCommand(transaction, {ST7789_VRHS, array<byte, 1>{byte{0x12}}});
  WriteCommand(transaction, {ST7789_VDVS, array<byte, 1>{byte{0x20}}});
  WriteCommand(transaction,
               {ST7789_PWCTRL1,
                array<byte, 2>{
                    byte{0xa4},
                    byte{0xa1},
                }});
  WriteCommand(transaction, {ST7789_FRCTRL2, array<byte, 1>{byte{0x0f}}});

  WriteCommand(transaction, {ST7789_INVON, kEmptyArray});
  WriteCommand(transaction, {ST7789_SLPOUT, kEmptyArray});
  WriteCommand(transaction, {ST7789_DISPON, kEmptyArray});

  // Landscape drawing Column Address Set
  const uint16_t kMaxColumn = config_.screen_width - 1;
  WriteCommand(transaction,
               {ST7789_CASET,
                array<byte, 4>{
                    byte{0x0},
                    byte{0x0},
                    byte{static_cast<uint8_t>(kMaxColumn >> 8)},
                    byte{static_cast<uint8_t>(kMaxColumn & 0xff)},
                }});

  // Page Address Set
  const uint16_t kMaxRow = config_.screen_height - 1;
  WriteCommand(transaction,
               {ST7789_RASET,
                array<byte, 4>{
                    byte{0x0},
                    byte{0x0},
                    byte{static_cast<uint8_t>(kMaxRow >> 8)},
                    byte{static_cast<uint8_t>(kMaxRow & 0xff)},
                }});

  uint8_t madctl = 0;
  bool rotate_180 = false;

  if (config_.screen_width == 240 && config_.screen_height == 240) {
    // TODO: Figure out 240x240 square display MADCTL values for rotation.
    madctl = ST7789_MADCTL_HORIZ_ORDER;
  } else if (config_.screen_width == 320 && config_.screen_height == 240) {
    madctl = ST7789_MADCTL_COL_ORDER;
    if (rotate_180)
      madctl = ST7789_MADCTL_ROW_ORDER;

    madctl |= ST7789_MADCTL_SWAP_XY | ST7789_MADCTL_SCAN_ORDER;
  }

  WriteCommand(transaction, {ST7789_MADCTL, array<byte, 1>{byte{madctl}}});

  pw::spin_delay::WaitMillis(50);

  return OkStatus();
}

void DisplayDriverST7789::WriteFramebuffer(Framebuffer frame_buffer,
                                           WriteCallback write_callback) {
  PW_ASSERT(frame_buffer.pixel_format() == PixelFormat::RGB565);
  if (config_.pixel_pusher) {
    // Write the pixel data.
    config_.pixel_pusher->WriteFramebuffer(std::move(frame_buffer),
                                           std::move(write_callback));
    return;
  }

  // Write the frame_buffer using pw_spi.
  // Let controller know a write is coming.
  Status s;
  {
    auto transaction = config_.spi_device_8_bit.StartTransaction(
        ChipSelectBehavior::kPerWriteRead);
    s = WriteCommand(transaction, {ST7789_RAMWR, kEmptyArray});
    if (!s.ok()) {
      write_callback(std::move(frame_buffer), s);
      return;
    }
  }

  // Write the pixel data.
  auto transaction = config_.spi_device_16_bit.StartTransaction(
      ChipSelectBehavior::kPerWriteRead);
  const uint16_t* fb_data = static_cast<const uint16_t*>(frame_buffer.data());
  const int num_pixels = frame_buffer.size().width * frame_buffer.size().height;
  s = transaction.Write(
      ConstByteSpan(reinterpret_cast<const byte*>(fb_data), num_pixels));
  write_callback(std::move(frame_buffer), s);
}

Status DisplayDriverST7789::WriteRow(span<uint16_t> row_pixels,
                                     uint16_t row_idx,
                                     uint16_t col_idx) {
  {
    // Let controller know a write is coming.
    auto transaction = config_.spi_device_8_bit.StartTransaction(
        ChipSelectBehavior::kPerWriteRead);
    // Landscape drawing Column Address Set
    const uint16_t max_col_idx =
        std::max(config_.screen_width - 1,
                 col_idx + static_cast<int>(row_pixels.size()));
    WriteCommand(transaction,
                 {ST7789_CASET,
                  std::array<std::byte, 4>{
                      std::byte{HighByte(col_idx)},
                      std::byte{LowByte(col_idx)},
                      std::byte{HighByte(max_col_idx)},
                      std::byte{LowByte(max_col_idx)},
                  }});

    // Page Address Set
    uint16_t max_row_idx = row_idx;
    WriteCommand(transaction,
                 {ST7789_RASET,
                  std::array<std::byte, 4>{
                      std::byte{HighByte(row_idx)},
                      std::byte{LowByte(row_idx)},
                      std::byte{HighByte(max_row_idx)},
                      std::byte{LowByte(max_row_idx)},
                  }});
    PW_TRY(WriteCommand(transaction, {ST7789_RAMWR, kEmptyArray}));
  }

  auto transaction = config_.spi_device_16_bit.StartTransaction(
      ChipSelectBehavior::kPerTransaction);
  return transaction.Write(
      ConstByteSpan(reinterpret_cast<const std::byte*>(row_pixels.data()),
                    row_pixels.size()));
}

Status DisplayDriverST7789::Reset() {
  if (!config_.reset_gpio)
    return Status::Unavailable();
  auto s = config_.reset_gpio->SetStateActive();
  if (!s.ok())
    return s;
  pw::spin_delay::WaitMillis(100);
  s = config_.reset_gpio->SetStateInactive();
  pw::spin_delay::WaitMillis(100);
  return s;
}

bool DisplayDriverST7789::SupportsResize() const {
  return config_.pixel_pusher != nullptr &&
         config_.pixel_pusher->SupportsResize();
}

}  // namespace pw::display_driver
