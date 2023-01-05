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

#include "pw_display_driver_ili9341/display_driver.h"

#include "pw_digital_io/digital_io.h"
#include "pw_framebuffer/rgb565.h"
#include "pw_spin_delay/delay.h"

using pw::color::color_rgb565_t;
using pw::digital_io::State;
using pw::framebuffer::FramebufferRgb565;
using pw::spi::ChipSelectBehavior;
using pw::spi::Device;

namespace pw::display_driver {

namespace {

constexpr uint16_t ILI9341_MADCTL = 0x36;
constexpr std::byte MADCTL_MY = std::byte{0x80};
constexpr std::byte MADCTL_MX = std::byte{0x40};
constexpr std::byte MADCTL_MV = std::byte{0x20};
constexpr std::byte MADCTL_ML = std::byte{0x10};
constexpr std::byte MADCTL_RGB = std::byte{0x00};
constexpr std::byte MADCTL_BGR = std::byte{0x08};
constexpr std::byte MADCTL_MH = std::byte{0x04};

constexpr uint16_t ILI9341_PIXEL_FORMAT_SET = 0x3A;

// The ILI9341 is hard-coded at 320x240;
constexpr int kDisplayWidth = 320;
constexpr int kDisplayHeight = 240;
constexpr int kDisplayNumPixels = kDisplayWidth * kDisplayHeight;

}  // namespace

DisplayDriverILI9341::DisplayDriverILI9341(const Config& config)
    : config_(config) {}

void DisplayDriverILI9341::SetMode(Mode mode) {
  // Set the D/CX pin to indicate data or command values.
  if (mode == Mode::kData) {
    config_.data_cmd_gpio.SetState(State::kActive);
  } else {
    config_.data_cmd_gpio.SetState(State::kInactive);
  }
}

Status DisplayDriverILI9341::WriteCommand(Device::Transaction& transaction,
                                          const Command& command) {
  SetMode(Mode::kCommand);
  std::byte buff[1]{static_cast<std::byte>(command.command)};
  auto s = transaction.Write(buff);
  if (!s.ok())
    return s;

  if (command.command_data.empty())
    return OkStatus();
  SetMode(Mode::kData);
  return transaction.Write(command.command_data);
}

Status DisplayDriverILI9341::Init() {
  Reset().IgnoreError();

  // TODO(cmumford): Figure out why kPerTransaction is flakey for this.
  // Seems to be OK on the Pico's display, but not the STM32F429I-DISC1.
  auto transaction = config_.spi_device_8_bit.StartTransaction(
      ChipSelectBehavior::kPerWriteRead);

  // ?
  WriteCommand(transaction,
               {0xEF,
                std::array<std::byte, 3>{
                    std::byte{0x03},
                    std::byte{0x80},
                    std::byte{0x02},
                }});

  // ?
  WriteCommand(transaction,
               {0xCF,
                std::array<std::byte, 3>{
                    std::byte{0x00},
                    std::byte{0xC1},
                    std::byte{0x30},
                }});

  // ?
  WriteCommand(transaction,
               {0xED,
                std::array<std::byte, 4>{
                    std::byte{0x64},
                    std::byte{0x03},
                    std::byte{0x12},
                    std::byte{0x81},
                }});

  // ?
  WriteCommand(transaction,
               {0xE8,
                std::array<std::byte, 3>{
                    std::byte{0x85},
                    std::byte{0x00},
                    std::byte{0x78},
                }});

  // ?
  WriteCommand(transaction,
               {0xCB,
                std::array<std::byte, 5>{
                    std::byte{0x39},
                    std::byte{0x2C},
                    std::byte{0x00},
                    std::byte{0x34},
                    std::byte{0x02},
                }});

  // ?
  WriteCommand(transaction, {0xF7, std::array<std::byte, 1>{std::byte{0x20}}});

  // ?
  WriteCommand(transaction,
               {0xEA,
                std::array<std::byte, 2>{
                    std::byte{0x00},
                    std::byte{0x00},
                }});

  // Power control
  WriteCommand(transaction, {0xC0, std::array<std::byte, 1>{std::byte{0x23}}});

  // Power control
  WriteCommand(transaction, {0xC1, std::array<std::byte, 1>{std::byte{0x10}}});

  // VCM control
  WriteCommand(transaction,
               {0xC5,
                std::array<std::byte, 2>{
                    std::byte{0x3e},
                    std::byte{0x28},
                }});

  // VCM control
  WriteCommand(transaction, {0xC7, std::array<std::byte, 1>{std::byte{0x86}}});

  constexpr std::byte kMode0 = MADCTL_MX | MADCTL_BGR;
  constexpr std::byte kMode1 = MADCTL_MV | MADCTL_BGR;
  constexpr std::byte kMode2 = MADCTL_MY | MADCTL_BGR;
  constexpr std::byte kMode3 = MADCTL_MX | MADCTL_MY | MADCTL_MV | MADCTL_BGR;
  WriteCommand(transaction, {ILI9341_MADCTL, std::array<std::byte, 1>{kMode3}});

  constexpr std::byte kPixelFormat16bits = std::byte(0x55);
  constexpr std::byte kPixelFormat18bits = std::byte(0x36);
  WriteCommand(
      transaction,
      {ILI9341_PIXEL_FORMAT_SET, std::array<std::byte, 1>{kPixelFormat16bits}});

  // Frame Control (Normal Mode)
  constexpr std::byte kFrameRate61 = std::byte{0x1F};
  constexpr std::byte kFrameRate70 = std::byte{0x1B};
  constexpr std::byte kFrameRate79 = std::byte{0x18};
  constexpr std::byte kFrameRate119 = std::byte{0x10};
  WriteCommand(transaction,
               {0xB1,
                std::array<std::byte, 2>{
                    std::byte{0x00},  // division ratio
                    kFrameRate61,
                }});

  // Display Function Control
  WriteCommand(transaction,
               {0xB6,
                std::array<std::byte, 3>{
                    std::byte{0x08},
                    std::byte{0x82},
                    std::byte{0x27},
                }});

  // Gamma Function Disable?
  WriteCommand(transaction, {0xF2, std::array<std::byte, 1>{std::byte{0x00}}});

  // Gamma Set
  WriteCommand(transaction, {0x26, std::array<std::byte, 1>{std::byte{0x01}}});

  // Positive Gamma Correction
  WriteCommand(transaction,
               {0xE0,
                std::array<std::byte, 15>{
                    std::byte{0x0F},
                    std::byte{0x31},
                    std::byte{0x2B},
                    std::byte{0x0C},
                    std::byte{0x0E},
                    std::byte{0x08},
                    std::byte{0x4E},
                    std::byte{0xF1},
                    std::byte{0x37},
                    std::byte{0x07},
                    std::byte{0x10},
                    std::byte{0x03},
                    std::byte{0x0E},
                    std::byte{0x09},
                    std::byte{0x00},
                }});

  // Negative Gamma Correction
  WriteCommand(transaction,
               {0xE1,
                std::array<std::byte, 15>{
                    std::byte{0x00},
                    std::byte{0x0E},
                    std::byte{0x14},
                    std::byte{0x03},
                    std::byte{0x11},
                    std::byte{0x07},
                    std::byte{0x31},
                    std::byte{0xC1},
                    std::byte{0x48},
                    std::byte{0x08},
                    std::byte{0x0F},
                    std::byte{0x0C},
                    std::byte{0x31},
                    std::byte{0x36},
                    std::byte{0x0F},
                }});

  // Exit Sleep
  WriteCommand(transaction, {0x11, std::array<std::byte, 0>{}});
  pw::spin_delay::WaitMillis(100);

  // Display On
  WriteCommand(transaction, {0x29, std::array<std::byte, 0>{}});
  pw::spin_delay::WaitMillis(100);

  // Normal display mode on
  WriteCommand(transaction, {0x13, std::array<std::byte, 0>{}});

  // Setup drawing full framebuffers

  // Landscape drawing Column Address Set
  constexpr uint16_t kMaxColumn = kDisplayWidth - 1;
  WriteCommand(transaction,
               {0x2A,
                std::array<std::byte, 4>{
                    std::byte{0x0},
                    std::byte{0x0},
                    std::byte{kMaxColumn >> 8},    // high byte of short.
                    std::byte{kMaxColumn & 0xff},  // low byte of short.
                }});

  // Page Address Set
  constexpr uint16_t kMaxRow = kDisplayHeight - 1;
  WriteCommand(transaction,
               {0x2B,
                std::array<std::byte, 4>{
                    std::byte{0x0},
                    std::byte{0x0},
                    std::byte{kMaxRow >> 8},    // high byte of short.
                    std::byte{kMaxRow & 0xff},  // low byte of short.
                }});

  pw::spin_delay::WaitMillis(10);
  WriteCommand(transaction, {0x2C, std::array<std::byte, 0>{}});

  SetMode(Mode::kData);
  pw::spin_delay::WaitMillis(100);

  return OkStatus();
}

Status DisplayDriverILI9341::Update(const FramebufferRgb565& frame_buffer) {
  auto transaction = config_.spi_device_16_bit.StartTransaction(
      ChipSelectBehavior::kPerTransaction);
  const uint16_t* fb_data = frame_buffer.GetFramebufferData();
  Status s;
  // TODO(cmumford): Figure out why the STM32F429I cannot send the entire
  // framebuffer in a single write, but another display can.
#if 1
  constexpr int kNumRowsPerSend = 10;
  static_assert(!(kDisplayHeight % kNumRowsPerSend),
                "Cannot send fractional number of rows");
  constexpr int kNumSends = kDisplayHeight / kNumRowsPerSend;
  constexpr size_t kNumPixelsInSend = kDisplayWidth * kNumRowsPerSend;

  for (int i = 0; i < kNumSends && s.ok(); i++) {
    const uint8_t* data = reinterpret_cast<const uint8_t*>(
        &fb_data[kDisplayWidth * (kNumRowsPerSend * i)]);
    // At this point the SPI bus is in 16-bit mode, so we send the number
    // of 16-bit values (i.e. pixels).
    s = transaction.Write(ConstByteSpan(
        reinterpret_cast<const std::byte*>(data), kNumPixelsInSend));
  }
#else
  s = transaction.Write(ConstByteSpan(
      reinterpret_cast<const std::byte*>(fb_data), kDisplayNumPixels));
#endif
  return s;
}

Status DisplayDriverILI9341::UpdatePixelDouble(
    const FramebufferRgb565& frame_buffer) {
  uint16_t temp_row[kDisplayWidth];
  auto transaction = config_.spi_device_16_bit.StartTransaction(
      ChipSelectBehavior::kPerTransaction);
  const color_rgb565_t* const fbdata = frame_buffer.GetFramebufferData();
  for (int y = 0; y < frame_buffer.GetHeight(); y++) {
    // Populate this row with each pixel repeated twice
    for (int x = 0; x < frame_buffer.GetWidth(); x++) {
      temp_row[x * 2] = fbdata[y * frame_buffer.GetWidth() + x];
      temp_row[(x * 2) + 1] = fbdata[y * frame_buffer.GetWidth() + x];
    }
    // Send this row to the display twice.
    auto s = transaction.Write(ConstByteSpan(
        reinterpret_cast<const std::byte*>(temp_row), kDisplayWidth));
    if (!s.ok())
      return s;
    s = transaction.Write(ConstByteSpan(
        reinterpret_cast<const std::byte*>(temp_row), kDisplayWidth));
    if (!s.ok())
      return s;
  }
  return OkStatus();
}

Status DisplayDriverILI9341::Reset() {
  if (!config_.reset_gpio)
    return Status::Unavailable();
  auto s = config_.reset_gpio->SetStateInactive();
  if (!s.ok())
    return s;
  pw::spin_delay::WaitMillis(100);
  s = config_.reset_gpio->SetStateActive();
  pw::spin_delay::WaitMillis(100);
  return s;
}

}  // namespace pw::display_driver
