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

#include <algorithm>

#include "pw_digital_io/digital_io.h"
#include "pw_framebuffer/framebuffer.h"
#include "pw_spin_delay/delay.h"

using pw::digital_io::State;
using pw::framebuffer::Framebuffer;
using pw::framebuffer::PixelFormat;
using pw::spi::ChipSelectBehavior;
using pw::spi::Device;

namespace pw::display_driver {

namespace {

constexpr std::array<std::byte, 0> kEmptyByteArray = {};

// clang-format off
// Level 1 commands:
constexpr uint8_t CMD_SWRESET            = 0x01;   // Software Reset.
constexpr uint8_t CMD_READ_DISPLAY_ID    = 0x04;   // Read display identification information.
constexpr uint8_t CMD_RDDST              = 0x09;   // Read Display Status.
constexpr uint8_t CMD_RDDPM              = 0x0A;   // Read Display Power Mode.
constexpr uint8_t CMD_RDDMADCTL          = 0x0B;   // Read Display MADCTL.
constexpr uint8_t CMD_RDDCOLMOD          = 0x0C;   // Read Display Pixel Format.
constexpr uint8_t CMD_RDDIM              = 0x0D;   // Read Display Image Format.
constexpr uint8_t CMD_RDDSM              = 0x0E;   // Read Display Signal Mode.
constexpr uint8_t CMD_RDDSDR             = 0x0F;   // Read Display Self-Diagnostic Result.
constexpr uint8_t CMD_SPLIN              = 0x10;   // Enter Sleep Mode.
constexpr uint8_t CMD_SLEEP_OUT          = 0x11;   // Sleep Out.
constexpr uint8_t CMD_PTLON              = 0x12;   // Partial Mode ON.
constexpr uint8_t CMD_NORMAL_MODE_ON     = 0x13;   // Normal Display Mode ON.
constexpr uint8_t CMD_DINVOFF            = 0x20;   // Display Inversion OFF.
constexpr uint8_t CMD_DINVON             = 0x21;   // Display Inversion ON.
constexpr uint8_t CMD_GAMMA              = 0x26;   // Gamma Set.
constexpr uint8_t CMD_DISPLAY_OFF        = 0x28;   // Display OFF.
constexpr uint8_t CMD_DISPLAY_ON         = 0x29;   // Display ON.
constexpr uint8_t CMD_COLUMN_ADDR        = 0x2A;   // Column Address Set.
constexpr uint8_t CMD_PAGE_ADDR          = 0x2B;   // Page Address Set.
constexpr uint8_t CMD_GRAM               = 0x2C;   // Memory Write.
constexpr uint8_t CMD_RGBSET             = 0x2D;   // Color Set.
constexpr uint8_t CMD_RAMRD              = 0x2E;   // Memory Read.
constexpr uint8_t CMD_PLTAR              = 0x30;   // Partial Area.
constexpr uint8_t CMD_VSCRDEF            = 0x33;   // Vertical Scrolling Definition.
constexpr uint8_t CMD_TEOFF              = 0x34;   // Tearing Effect Line OFF.
constexpr uint8_t CMD_TEON               = 0x35;   // Tearing Effect Line ON.
constexpr uint8_t CMD_MADCTL             = 0x36;   // Memory Access Control.
constexpr uint8_t CMD_VSCRSADD           = 0x37;   // Vertical Scrolling Start Address.
constexpr uint8_t CMD_IDMOFF             = 0x38;   // Idle Mode OFF.
constexpr uint8_t CMD_IDMON              = 0x39;   // Idle Mode ON.
constexpr uint8_t CMD_PIXEL_FORMAT       = 0x3A;   // COLMOD: Pixel Format Set.
constexpr uint8_t CMD_WRITE_MEM_CONTINUE = 0x3C;   // Write_Memory_Continue.
constexpr uint8_t CMD_READ_MEM_CONTINUE  = 0x3E;   // Read_Memory_Continue.
constexpr uint8_t CMD_SET_TEAR_SCANLINE  = 0x44;   // Set_Tear_Scanline.
constexpr uint8_t CMD_GET_SCANLINE       = 0x45;   // Get_Scanline.
constexpr uint8_t CMD_WDB                = 0x51;   // Write Display Brightness.
constexpr uint8_t CMD_RDDISBV            = 0x52;   // Read Display Brightness.
constexpr uint8_t CMD_WCD                = 0x53;   // Write CTRL Display.
constexpr uint8_t CMD_RDCTRLD            = 0x54;   // Read CTRL Display.
constexpr uint8_t CMD_WRCABC             = 0x55;   // Write Content Adaptive Brightness Control.
constexpr uint8_t CMD_RDCABC             = 0x56;   // Read Content Adaptive Brightness Control.
constexpr uint8_t CMD_WRITE_CABC         = 0x5E;   // Write CABC Minimum Brightness.
constexpr uint8_t CMD_READ_CABC          = 0x5F;   // Read CABC Minimum Brightness.
constexpr uint8_t CMD_READ_ID1           = 0xDA;   // Read ID1.
constexpr uint8_t CMD_READ_ID2           = 0xDB;   // Read ID2.
constexpr uint8_t CMD_READ_ID3           = 0xDC;   // Read ID3.

// Level 2 commands:
constexpr uint8_t CMD_RGB_INTERFACE      = 0xB0;   // RGB Interface Signal Control.
constexpr uint8_t CMD_FRMCTR1            = 0xB1;   // Frame Rate Control (In Normal Mode/Full Colors.
constexpr uint8_t CMD_FRMCTR2            = 0xB2;   // Frame Rate Control (In Idle Mode/8 colors).
constexpr uint8_t CMD_FRMCTR3            = 0xB3;   // Frame Rate control (In Partial Mode/Full Colors).
constexpr uint8_t CMD_INVTR              = 0xB4;   // Display Inversion Control.
constexpr uint8_t CMD_BPC                = 0xB5;   // Blanking Porch Control.
constexpr uint8_t CMD_DFC                = 0xB6;   // Display Function Control.
constexpr uint8_t CMD_ETMOD              = 0xB7;   // Entry Mode Set.
constexpr uint8_t CMD_BACKLIGHT1         = 0xB8;   // Backlight Control 1.
constexpr uint8_t CMD_BACKLIGHT2         = 0xB9;   // Backlight Control 2.
constexpr uint8_t CMD_BACKLIGHT3         = 0xBA;   // Backlight Control 3.
constexpr uint8_t CMD_BACKLIGHT4         = 0xBB;   // Backlight Control 4.
constexpr uint8_t CMD_BACKLIGHT5         = 0xBC;   // Backlight Control 5.
constexpr uint8_t CMD_BACKLIGHT7         = 0xBE;   // Backlight Control 7.
constexpr uint8_t CMD_BACKLIGHT8         = 0xBF;   // Backlight Control 8.
constexpr uint8_t CMD_POWER1             = 0xC0;   // Power Control 1.
constexpr uint8_t CMD_POWER2             = 0xC1;   // Power Control 2.
constexpr uint8_t CMD_VCOM1              = 0xC5;   // VCOM Control 1.
constexpr uint8_t CMD_VCOM2              = 0xC7;   // VCOM Control 2.
constexpr uint8_t CMD_NVMWR              = 0xD0;   // NV Memory Write.
constexpr uint8_t CMD_NVMPKEY            = 0xD1;   // NV Memory Protection Key.
constexpr uint8_t CMD_RDNVM              = 0xD2;   // NV Memory Status Read.
constexpr uint8_t CMD_READ_ID4           = 0xD3;   // Read ID4.
constexpr uint8_t CMD_PGAMMA             = 0xE0;   // Positive Gamma Correction.
constexpr uint8_t CMD_NGAMMA             = 0xE1;   // Negative Gamma Correction.
constexpr uint8_t CMD_DGAMCTRL1          = 0xE2;   // Digital Gamma Control 1.
constexpr uint8_t CMD_DGAMCTRL2          = 0xE3;   // Digital Gamma Control 2.
constexpr uint8_t CMD_INTERFACE          = 0xF6;   // Interface Control.

// Extended register commands:
constexpr uint8_t CMD_POWERA             = 0xCB;   // Power control A.
constexpr uint8_t CMD_POWERB             = 0xCF;   // Power control B.
constexpr uint8_t CMD_DTCA               = 0xE8;   // Driver timing control A.
constexpr uint8_t CMD_DTCA_2             = 0xE9;   // Driver timing control A.
constexpr uint8_t CMD_DTCB               = 0xEA;   // Driver timing control B.
constexpr uint8_t CMD_POWER_SEQ          = 0xED;   // Power on sequence control.
constexpr uint8_t CMD_3GAMMA_EN          = 0xF2;   // Enable 3G.
constexpr uint8_t CMD_PRC                = 0xF7;   // Pump ratio control .
// clang-format on

// The ILI9341 is hard-coded at 320x240;
constexpr int kDisplayWidth = 320;
constexpr int kDisplayHeight = 240;
constexpr int kDisplayNumPixels = kDisplayWidth * kDisplayHeight;

// clang-format off
constexpr std::byte MADCTL_MY  = std::byte{0b10000000}; // Row address order.
constexpr std::byte MADCTL_MX  = std::byte{0b01000000}; // Column address order.
constexpr std::byte MADCTL_MV  = std::byte{0b00100000}; // Row/column exchange.
constexpr std::byte MADCTL_ML  = std::byte{0b00010000}; // Vertical refresh order.
constexpr std::byte MADCTL_BGR = std::byte{0b00001000}; // BGR/RGB order.
constexpr std::byte MADCTL_MH  = std::byte{0b00000100}; // Horizontal refresh order.

// Take value specified in target.
constexpr std::byte kMADMode = std::byte{ILI9341_MADCTL};

constexpr uint8_t kDTC_PTG_MASK          = 0b00001100;
constexpr uint8_t kDTC_PTG_NORMAL_SCAN   = 0b00000000;
constexpr uint8_t kDTC_PTG_PROHIBITED1   = 0b00000100;
constexpr uint8_t kDTC_PTG_INTERVAL_SCAN = 0b00001000;
constexpr uint8_t kDTC_PTG_PROHIBITED2   = 0b00001100;

// Mask values for CMD_RGB_INTERFACE:
constexpr uint8_t kIFMODE_MASK_EPL    = 0b00000001;
constexpr uint8_t kIFMODE_MASK_DPL    = 0b00000010;
constexpr uint8_t kIFMODE_MASK_HSPL   = 0b00000100;
constexpr uint8_t kIFMODE_MASK_VSPL   = 0b00001000;
constexpr uint8_t kIFMODE_MASK_UNUSED = 0b00010000;
constexpr uint8_t kIFMODE_MASK_RCM    = 0b01100000;
constexpr uint8_t kIFMODE_MASK_BYPASS = 0b10000000;
// clang-format on

// Bypass=memory, RGB IF="VSYNC, HSYNC, DOTCLK, DE, D", DPL=falling.
constexpr uint8_t kRGBWithDE = 0xC2;
// Bypass=memory, RGB IF="VSYNC, HSYNC, DOTCLK, D", DPL=falling.
constexpr uint8_t kRGBWithoutDE = 0xE2;

// Frame Control (Normal Mode)
constexpr std::byte kFrameRate61 = std::byte{0x1F};
constexpr std::byte kFrameRate70 = std::byte{0x1B};
constexpr std::byte kFrameRate79 = std::byte{0x18};
constexpr std::byte kFrameRate119 = std::byte{0x10};

constexpr std::byte kPixelFormat16bits = std::byte(0x55);
constexpr std::byte kPixelFormat18bits = std::byte(0x36);

constexpr uint8_t HighByte(uint16_t val) { return val >> 8; }

constexpr uint8_t LowByte(uint16_t val) { return val & 0xff; }

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

  SetMode(Mode::kData);
  if (command.command_data.empty())
    return OkStatus();
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

  WriteCommand(transaction,
               {CMD_POWERB,
                std::array<std::byte, 3>{
                    std::byte{0x00},
                    std::byte{0xC1},
                    std::byte{0x30},
                }});

  WriteCommand(transaction,
               {CMD_POWER_SEQ,
                std::array<std::byte, 4>{
                    std::byte{0x64},
                    std::byte{0x03},
                    std::byte{0x12},
                    std::byte{0x81},
                }});

  WriteCommand(transaction,
               {CMD_DTCA,
                std::array<std::byte, 3>{
                    std::byte{0x85},
                    std::byte{0x00},
                    std::byte{0x78},
                }});

  WriteCommand(transaction,
               {CMD_POWERA,
                std::array<std::byte, 5>{
                    std::byte{0x39},
                    std::byte{0x2C},
                    std::byte{0x00},
                    std::byte{0x34},
                    std::byte{0x02},
                }});

  WriteCommand(transaction,
               {CMD_PRC, std::array<std::byte, 1>{std::byte{0x20}}});

  WriteCommand(transaction,
               {CMD_DTCB,
                std::array<std::byte, 2>{
                    std::byte{0x00},
                    std::byte{0x00},
                }});

  WriteCommand(transaction,
               {CMD_FRMCTR1,
                std::array<std::byte, 2>{
                    std::byte{0x00},  // division ratio = fosc.
                    kFrameRate70,
                }});

  // Display Function Control
  WriteCommand(transaction,
               {CMD_DFC,
                std::array<std::byte, 2>{
                    std::byte{0x0A},
                    std::byte{0xA2},
                }});

  // Power control. GVDD = 0x10 = 3.65V.
  WriteCommand(transaction,
               {CMD_POWER1, std::array<std::byte, 1>{std::byte{0x10}}});

  // Power control
  WriteCommand(transaction,
               {CMD_POWER2, std::array<std::byte, 1>{std::byte{0x10}}});

  // VCM control
  WriteCommand(transaction,
               {CMD_VCOM1,
                std::array<std::byte, 2>{
                    std::byte{0x3e},
                    std::byte{0x28},
                }});

  // VCM control
  WriteCommand(transaction,
               {CMD_VCOM2, std::array<std::byte, 1>{std::byte{0x86}}});

  // Memory Access Control.
  WriteCommand(transaction, {CMD_MADCTL, std::array<std::byte, 1>{kMADMode}});

  WriteCommand(
      transaction,
      {CMD_PIXEL_FORMAT, std::array<std::byte, 1>{kPixelFormat16bits}});

  // Gamma Function Disable?
  WriteCommand(transaction,
               {CMD_3GAMMA_EN, std::array<std::byte, 1>{std::byte{0x00}}});

  switch (config_.interface) {
    case InterfaceType::SPI:
      break;
    case InterfaceType::WithDE:
      WriteCommand(
          transaction,
          {CMD_RGB_INTERFACE, std::array<std::byte, 1>{std::byte{kRGBWithDE}}});
      break;
    case InterfaceType::WithoutDE:
      WriteCommand(transaction,
                   {CMD_RGB_INTERFACE,
                    std::array<std::byte, 1>{std::byte{kRGBWithoutDE}}});
      break;
  }

  // Display Function Control
  WriteCommand(transaction,
               {CMD_DFC,
                std::array<std::byte, 4>{
                    std::byte{0x0A},
                    std::byte{0xA7},
                    std::byte{0x27},
                    std::byte{0x04},
                }});

  // Max pixel coordinates in portrait mode.
  constexpr uint16_t kMinX = 0;
  constexpr uint16_t kMaxX = 240 - 1;
  constexpr uint16_t kMinY = 0;
  constexpr uint16_t kMaxY = 320 - 1;

  // Landscape drawing Column Address Set
  const uint16_t kMinColumn = config_.swap_row_col ? kMinX : kMinY;
  const uint16_t kMaxColumn = config_.swap_row_col ? kMaxX : kMaxY;
  WriteCommand(transaction,
               {CMD_COLUMN_ADDR,
                std::array<std::byte, 4>{
                    std::byte{HighByte(kMinColumn)},
                    std::byte{LowByte(kMinColumn)},
                    std::byte{HighByte(kMaxColumn)},
                    std::byte{LowByte(kMaxColumn)},
                }});

  // Page Address Set
  const uint16_t kMinRow = config_.swap_row_col ? kMinY : kMinX;
  const uint16_t kMaxRow = config_.swap_row_col ? kMaxY : kMaxX;
  WriteCommand(transaction,
               {CMD_PAGE_ADDR,
                std::array<std::byte, 4>{
                    std::byte{HighByte(kMinRow)},
                    std::byte{LowByte(kMinRow)},
                    std::byte{HighByte(kMaxRow)},
                    std::byte{LowByte(kMaxRow)},
                }});

  if (config_.interface != InterfaceType::SPI) {
    WriteCommand(transaction,
                 {CMD_INTERFACE,
                  std::array<std::byte, 3>{
                      std::byte{0x00},
                      std::byte{0x01},
                      std::byte{0x06},
                  }});
  }

  WriteCommand(transaction, {CMD_GRAM, kEmptyByteArray});
  pw::spin_delay::WaitMillis(200);

  // Gamma Set
  WriteCommand(transaction,
               {CMD_GAMMA, std::array<std::byte, 1>{std::byte{0x01}}});

  // Positive Gamma Correction
  WriteCommand(transaction,
               {CMD_PGAMMA,
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
               {CMD_NGAMMA,
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

  WriteCommand(transaction, {CMD_SLEEP_OUT, kEmptyByteArray});
  pw::spin_delay::WaitMillis(200);

  WriteCommand(transaction, {CMD_DISPLAY_ON, kEmptyByteArray});

  WriteCommand(transaction, {CMD_NORMAL_MODE_ON, kEmptyByteArray});

  WriteCommand(transaction, {CMD_GRAM, kEmptyByteArray});

  return OkStatus();
}

void DisplayDriverILI9341::WriteFramebuffer(Framebuffer frame_buffer,
                                            WriteCallback write_callback) {
  PW_ASSERT(frame_buffer.is_valid());
  PW_ASSERT(frame_buffer.pixel_format() == PixelFormat::RGB565);
  if (config_.pixel_pusher) {
    config_.pixel_pusher->WriteFramebuffer(std::move(frame_buffer),
                                           std::move(write_callback));
    return;
  }
  auto transaction = config_.spi_device_16_bit.StartTransaction(
      ChipSelectBehavior::kPerTransaction);
  const uint16_t* fb_data = static_cast<const uint16_t*>(frame_buffer.data());
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
  write_callback(std::move(frame_buffer), s);
}

Status DisplayDriverILI9341::WriteRow(span<uint16_t> row_pixels,
                                      uint16_t row_idx,
                                      uint16_t col_idx) {
  {
    // Let controller know a write is coming.
    auto transaction = config_.spi_device_8_bit.StartTransaction(
        ChipSelectBehavior::kPerWriteRead);
    // Landscape drawing Column Address Set
    const uint16_t max_col_idx = std::max(
        kDisplayWidth - 1, col_idx + static_cast<int>(row_pixels.size()));
    WriteCommand(transaction,
                 {CMD_COLUMN_ADDR,
                  std::array<std::byte, 4>{
                      std::byte{HighByte(col_idx)},
                      std::byte{LowByte(col_idx)},
                      std::byte{HighByte(max_col_idx)},
                      std::byte{LowByte(max_col_idx)},
                  }});

    // Page Address Set
    uint16_t max_row_idx = row_idx;
    WriteCommand(transaction,
                 {CMD_PAGE_ADDR,
                  std::array<std::byte, 4>{
                      std::byte{HighByte(row_idx)},
                      std::byte{LowByte(row_idx)},
                      std::byte{HighByte(max_row_idx)},
                      std::byte{LowByte(max_row_idx)},
                  }});
    PW_TRY(WriteCommand(transaction, {CMD_GRAM, kEmptyByteArray}));
  }

  auto transaction = config_.spi_device_16_bit.StartTransaction(
      ChipSelectBehavior::kPerTransaction);
  return transaction.Write(
      ConstByteSpan(reinterpret_cast<const std::byte*>(row_pixels.data()),
                    row_pixels.size()));
}

uint16_t DisplayDriverILI9341::GetWidth() const { return kDisplayWidth; }

uint16_t DisplayDriverILI9341::GetHeight() const { return kDisplayHeight; }

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
