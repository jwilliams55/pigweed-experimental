// Copyright 2023 The Pigweed Authors
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

#include "pw_display_driver_st7735/display_driver.h"

#include <array>
#include <cstddef>

#include "pw_digital_io/digital_io.h"
#include "pw_framebuffer/rgb565.h"
#include "pw_spin_delay/delay.h"

using pw::color::color_rgb565_t;
using pw::digital_io::State;
using pw::spi::ChipSelectBehavior;
using pw::spi::Device;
using std::array;
using std::byte;

namespace pw::display_driver {

namespace {

constexpr array<byte, 0> kEmptyArray;

// ST7735 Display Registers
// clang-format off
#define ST7735_SWRESET  0x01
#define ST7735_RDDID    0x04
#define ST7735_RDDST    0x09
#define ST7735_SLPIN    0x10
#define ST7735_SLPOUT   0x11
#define ST7735_PTLON    0x12
#define ST7735_NORON    0x13
#define ST7735_INVOFF   0x20
#define ST7735_INVON    0x21
#define ST7735_GAMSET   0x26
#define ST7735_DISPOFF  0x28
#define ST7735_DISPON   0x29
#define ST7735_RAMWR    0x2C
#define ST7735_CASET    0x2A
#define ST7735_RASET    0x2B
#define ST7735_RAMWR    0x2C
#define ST7735_RAMRD    0x2E
#define ST7735_PTLAR    0x30
#define ST7735_TEOFF    0x34
#define ST7735_TEON     0x35
#define ST7735_MADCTL   0x36
#define ST7735_COLMOD   0x3A
#define ST7735_FRMCTR1  0xB1 // Frame Rate Control (normal mode/ Full colors)
#define ST7735_FRMCTR2  0xB2 // Frame Rate Control (Idle mode/ 8-colors)
#define ST7735_FRMCTR3  0xB3 // Frame Rate Control (Partial mode/ full colors)
#define ST7735_INVCTR   0xB4
#define ST7735_DISSET5  0xB6
#define ST7735_GCTRL    0xB7
#define ST7735_VCOMS    0xBB
#define ST7735_PWCTR1   0xC0
#define ST7735_PWCTR2   0xC1
#define ST7735_PWCTR3   0xC2
#define ST7735_VRHS     0xC3
#define ST7735_VDVS     0xC4
#define ST7735_VMCTR1   0xC5
#define ST7735_FRCTRL2  0xC6
#define ST7735_PWCTRL1  0xD0
#define ST7735_RDID1    0xDA
#define ST7735_RDID2    0xDB
#define ST7735_RDID3    0xDC
#define ST7735_RDID4    0xDD
#define ST7735_PORCTRL  0xB2
#define ST7735_GMCTRP1  0xE0
#define ST7735_GMCTRN1  0xE1
#define ST7735_PWCTR6   0xFC

// MADCTL Bits (See page 215: MADCTL (36h): Memory Data Access Control)
#define ST7735_MADCTL_ROW_ORDER   0b10000000
#define ST7735_MADCTL_COL_ORDER   0b01000000
#define ST7735_MADCTL_SWAP_XY     0b00100000
#define ST7735_MADCTL_SCAN_ORDER  0b00010000
#define ST7735_MADCTL_RGB_BGR     0b00001000
#define ST7735_MADCTL_HORIZ_ORDER 0b00000100

#define ST7735_INVCTR_NLA 0b00000100 // Inversion setting in full Colors normal mode
#define ST7735_INVCTR_NLB 0b00000010 // Inversion setting in Idle mode
#define ST7735_INVCTR_NLC 0b00000001 // Inversion setting in full Colors partial mode

// clang-format on

constexpr uint8_t HighByte(uint16_t val) { return val >> 8; }

constexpr uint8_t LowByte(uint16_t val) { return val & 0xff; }

}  // namespace

// The ST7735 supports a max display size of 162x132. This was developed with
// a ST7735 development board with a 160x128 pixel screen - hence the row/col
// start values. These should be parameterized.
DisplayDriverST7735::DisplayDriverST7735(const Config& config)
    : config_(config), row_start_(2), col_start_(1) {}

void DisplayDriverST7735::SetMode(Mode mode) {
  // Set the D/CX pin to indicate data or command values.
  if (mode == Mode::kData) {
    config_.data_cmd_gpio.SetState(State::kActive);
  } else {
    config_.data_cmd_gpio.SetState(State::kInactive);
  }
}

Status DisplayDriverST7735::WriteCommand(Device::Transaction& transaction,
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

Status DisplayDriverST7735::Init() {
  auto transaction = config_.spi_device_8_bit.StartTransaction(
      ChipSelectBehavior::kPerWriteRead);

  WriteCommand(transaction, {ST7735_SWRESET, kEmptyArray});  // Software reset
  pw::spin_delay::WaitMillis(150);
  WriteCommand(transaction, {ST7735_SLPOUT, kEmptyArray});
  pw::spin_delay::WaitMillis(500);

  WriteCommand(transaction,
               {ST7735_FRMCTR1,
                array<byte, 3>{
                    byte{0x00},
                    byte{0x06},
                    byte{0x03},
                }});
  pw::spin_delay::WaitMillis(10);

  WriteCommand(transaction,
               {ST7735_DISSET5,
                array<byte, 2>{
                    byte{0x15},
                    byte{0x02},
                }});

  constexpr uint8_t inversion_val =
      ST7735_INVCTR_NLA | ST7735_INVCTR_NLB | ST7735_INVCTR_NLC;

  WriteCommand(transaction,
               {ST7735_INVCTR,
                array<byte, 1>{
                    byte{inversion_val},
                }});

  WriteCommand(transaction, {ST7735_TEON, kEmptyArray});
  WriteCommand(transaction, {ST7735_COLMOD, array<byte, 1>{byte{0x05}}});
  pw::spin_delay::WaitMillis(10);

  WriteCommand(transaction,
               {ST7735_PORCTRL,
                array<byte, 5>{
                    byte{0x0c},
                    byte{0x0c},
                    byte{0x00},
                    byte{0x33},
                    byte{0x33},
                }});
  WriteCommand(transaction,
               {ST7735_PWCTR1,
                array<byte, 2>{
                    byte{0x02},  // GVDD = 4.7V.
                    byte{0x70},  // 1.0uA.
                }});
  pw::spin_delay::WaitMillis(10);
  WriteCommand(transaction,
               {ST7735_PWCTR2,
                array<byte, 1>{
                    byte{0x05},
                }});
  WriteCommand(transaction,
               {ST7735_PWCTR3,
                array<byte, 2>{
                    byte{0x01},
                    byte{0x02},
                }});
  WriteCommand(transaction,
               {ST7735_VMCTR1,
                array<byte, 2>{
                    byte{0x3c},
                    byte{0x38},
                }});
  pw::spin_delay::WaitMillis(10);
  WriteCommand(transaction,
               {ST7735_PWCTR6,
                array<byte, 2>{
                    byte{0x11},
                    byte{0x15},
                }});

  WriteCommand(transaction, {ST7735_VRHS, array<byte, 1>{byte{0x12}}});
  WriteCommand(transaction, {ST7735_VDVS, array<byte, 1>{byte{0x20}}});
  WriteCommand(transaction,
               {ST7735_PWCTRL1,
                array<byte, 2>{
                    byte{0xa4},
                    byte{0xa1},
                }});
  WriteCommand(transaction, {ST7735_FRCTRL2, array<byte, 1>{byte{0x0f}}});

  WriteCommand(transaction, {ST7735_INVOFF, kEmptyArray});

  WriteCommand(transaction,
               {ST7735_GMCTRP1,
                array<byte, 16>{
                    byte{0x09},
                    byte{0x16},
                    byte{0x09},
                    byte{0x20},
                    byte{0x21},
                    byte{0x1B},
                    byte{0x13},
                    byte{0x19},
                    byte{0x17},
                    byte{0x15},
                    byte{0x1E},
                    byte{0x2B},
                    byte{0x04},
                    byte{0x05},
                    byte{0x02},
                    byte{0x0E},
                }});
  WriteCommand(transaction,
               {ST7735_GMCTRN1,
                array<byte, 16>{
                    byte{0x0B},
                    byte{0x14},
                    byte{0x08},
                    byte{0x1E},
                    byte{0x22},
                    byte{0x1D},
                    byte{0x18},
                    byte{0x1E},
                    byte{0x1B},
                    byte{0x1A},
                    byte{0x24},
                    byte{0x2B},
                    byte{0x06},
                    byte{0x06},
                    byte{0x02},
                    byte{0x0F},
                }});
  pw::spin_delay::WaitMillis(10);

  // Landscape drawing Column Address Set
  const uint16_t max_column = config_.screen_width + col_start_ - 1;
  WriteCommand(transaction,
               {ST7735_CASET,
                array<byte, 4>{
                    byte{HighByte(col_start_)},
                    byte{LowByte(col_start_)},
                    byte{HighByte(max_column)},
                    byte{LowByte(max_column)},
                }});

  // Page Address Set
  const uint16_t max_row = config_.screen_height + row_start_ - 1;
  WriteCommand(transaction,
               {ST7735_RASET,
                array<byte, 4>{
                    byte{HighByte(row_start_)},
                    byte{LowByte(row_start_)},
                    byte{HighByte(max_row)},
                    byte{LowByte(max_row)},
                }});

  constexpr bool rotate_180 = false;
  uint8_t madctl = ST7735_MADCTL_COL_ORDER;
  if (rotate_180)
    madctl = ST7735_MADCTL_ROW_ORDER;
  madctl |= ST7735_MADCTL_SWAP_XY | ST7735_MADCTL_SCAN_ORDER;
  WriteCommand(transaction, {ST7735_MADCTL, array<byte, 1>{byte{madctl}}});

  WriteCommand(transaction, {ST7735_NORON, kEmptyArray});
  pw::spin_delay::WaitMillis(10);

  WriteCommand(transaction, {ST7735_DISPON, kEmptyArray});
  pw::spin_delay::WaitMillis(500);

  return OkStatus();
}

Status DisplayDriverST7735::Update(
    pw::framebuffer::FramebufferRgb565* framebuffer) {
  PW_ASSERT(framebuffer->IsValid());
  // Let controller know a write is coming.
  {
    auto transaction = config_.spi_device_8_bit.StartTransaction(
        ChipSelectBehavior::kPerWriteRead);
    PW_TRY(WriteCommand(transaction, {ST7735_RAMWR, kEmptyArray}));
  }

  // Write the pixel data.
  auto transaction = config_.spi_device_16_bit.StartTransaction(
      ChipSelectBehavior::kPerWriteRead);
  const uint16_t* fb_data = framebuffer->GetFramebufferData();
  const size_t num_pixels = config_.screen_width * config_.screen_height;
  return transaction.Write(
      ConstByteSpan(reinterpret_cast<const byte*>(fb_data), num_pixels));
}

Status DisplayDriverST7735::Reset() {
  if (!config_.reset_gpio)
    return Status::Unavailable();
  PW_TRY(config_.reset_gpio->SetStateActive());
  pw::spin_delay::WaitMillis(100);
  PW_TRY(config_.reset_gpio->SetStateInactive());
  pw::spin_delay::WaitMillis(100);
  PW_TRY(config_.reset_gpio->SetStateActive());
  pw::spin_delay::WaitMillis(100);
  return OkStatus();
}

}  // namespace pw::display_driver
