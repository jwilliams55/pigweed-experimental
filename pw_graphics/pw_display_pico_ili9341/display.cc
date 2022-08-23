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

#include "hardware/spi.h"
#include "pico/stdlib.h"
#include "pw_color/color.h"
#include "pw_framebuffer/rgb565.h"

namespace pw::display {

namespace {

// ILI9341 Display Registers
#define ILI9341_MADCTL 0x36
#define MADCTL_MY 0x80
#define MADCTL_MX 0x40
#define MADCTL_MV 0x20
#define MADCTL_ML 0x10
#define MADCTL_RGB 0x00
#define MADCTL_BGR 0x08
#define MADCTL_MH 0x04
#define ILI9341_PIXEL_FORMAT_SET 0x3A

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
constexpr int kDisplayDataSize = kDisplayWidth * kDisplayHeight;

// SPI Functions
// TODO(tonymd): move to pw_spi
static inline void ChipSelectEnable() {
  asm volatile("nop \n nop \n nop");
  gpio_put(TFT_CS, 0);
  asm volatile("nop \n nop \n nop");
}

static inline void ChipSelectDisable() {
  asm volatile("nop \n nop \n nop");
  gpio_put(TFT_CS, 1);
  asm volatile("nop \n nop \n nop");
}

static inline void DataCommandEnable() {
  asm volatile("nop \n nop \n nop");
  gpio_put(TFT_DC, 0);
  asm volatile("nop \n nop \n nop");
}

static inline void DataCommandDisable() {
  asm volatile("nop \n nop \n nop");
  gpio_put(TFT_DC, 1);
  asm volatile("nop \n nop \n nop");
}

static void inline SPISendByte(uint8_t data) {
  ChipSelectEnable();
  DataCommandDisable();
  spi_write_blocking(SPI_PORT, &data, 1);
  ChipSelectDisable();
}

static void inline SPISendShort(uint16_t data) {
  ChipSelectEnable();
  DataCommandDisable();

  uint8_t shortBuffer[2];

  shortBuffer[0] = (uint8_t)(data >> 8);
  shortBuffer[1] = (uint8_t)data;

  spi_write_blocking(SPI_PORT, shortBuffer, 2);

  ChipSelectDisable();
}

static void inline SPISendCommand(uint8_t command) {
  // set data/command to command mode (low).
  DataCommandEnable();

  ChipSelectEnable();

  // send the command to the display.
  spi_write_blocking(SPI_PORT, &command, 1);

  // put the display back into data mode (high).
  DataCommandDisable();

  ChipSelectDisable();
}

}  // namespace

void Init() {
  stdio_init_all();
  uint actual_baudrate = spi_init(SPI_PORT, 31250000);
  spi_set_format(SPI_PORT, 8, (spi_cpol_t)1, (spi_cpha_t)1, SPI_MSB_FIRST);

  // Init pico SPI
  printf("Actual Baudrate: %i\n", actual_baudrate);
  // TFT_MISO is unused:
  // gpio_set_function(TFT_MISO, GPIO_FUNC_SPI);
  gpio_set_function(TFT_SCLK, GPIO_FUNC_SPI);
  gpio_set_function(TFT_MOSI, GPIO_FUNC_SPI);

  gpio_init(TFT_CS);
  gpio_init(TFT_DC);
  gpio_init(TFT_RST);

  gpio_set_dir(TFT_CS, GPIO_OUT);
  gpio_set_dir(TFT_DC, GPIO_OUT);
  gpio_set_dir(TFT_RST, GPIO_OUT);

  gpio_put(TFT_CS, 1);
  gpio_put(TFT_DC, 0);
  gpio_put(TFT_RST, 0);

  // Init Display
  ChipSelectEnable();

  // Toggle Reset pin
  gpio_put(TFT_RST, 0);
  sleep_ms(100);
  gpio_put(TFT_RST, 1);
  sleep_ms(100);

  // ILI9341 Init Sequence:
  SPISendCommand(0xEF);
  SPISendByte(0x03);
  SPISendByte(0x80);
  SPISendByte(0x02);

  // ?
  SPISendCommand(0xCF);
  SPISendByte(0x00);
  SPISendByte(0xC1);
  SPISendByte(0x30);

  // ?
  SPISendCommand(0xED);
  SPISendByte(0x64);
  SPISendByte(0x03);
  SPISendByte(0x12);
  SPISendByte(0x81);

  // ?
  SPISendCommand(0xE8);
  SPISendByte(0x85);
  SPISendByte(0x00);
  SPISendByte(0x78);

  // ?
  SPISendCommand(0xCB);
  SPISendByte(0x39);
  SPISendByte(0x2C);
  SPISendByte(0x00);
  SPISendByte(0x34);
  SPISendByte(0x02);

  // ?
  SPISendCommand(0xF7);
  SPISendByte(0x20);

  // ?
  SPISendCommand(0xEA);
  SPISendByte(0x00);
  SPISendByte(0x00);

  // Power control
  SPISendCommand(0xC0);
  SPISendByte(0x23);

  // Power control
  SPISendCommand(0xC1);
  SPISendByte(0x10);

  // VCM control
  SPISendCommand(0xC5);
  SPISendByte(0x3e);
  SPISendByte(0x28);

  // VCM control
  SPISendCommand(0xC7);
  SPISendByte(0x86);

  SPISendCommand(ILI9341_MADCTL);
  // Rotation
  // SPISendByte(MADCTL_MX | MADCTL_BGR); // 0
  // SPISendByte(MADCTL_MV | MADCTL_BGR); // 1 landscape
  // SPISendByte(MADCTL_MY | MADCTL_BGR); // 2
  SPISendByte(MADCTL_MX | MADCTL_MY | MADCTL_MV | MADCTL_BGR);  // 3 landscape

  SPISendCommand(ILI9341_PIXEL_FORMAT_SET);
  SPISendByte(0x55);  // 16 bits / pixel
  // SPISendByte(0x36);  // 18 bits / pixel

  // Frame Control (Normal Mode)
  SPISendCommand(0xB1);
  SPISendByte(0x00);  // division ratio
  SPISendByte(0x1F);  // 61 Hz
  // SPISendByte(0x1B);  // 70 Hz - default
  // SPISendByte(0x18);  // 79 Hz
  // SPISendByte(0x10);  // 119 Hz

  // Display Function Control
  SPISendCommand(0xB6);
  SPISendByte(0x08);
  SPISendByte(0x82);
  SPISendByte(0x27);

  // Gamma Function Disable?
  SPISendCommand(0xF2);
  SPISendByte(0x00);

  // Gamma Set
  SPISendCommand(0x26);
  SPISendByte(0x01);

  // Positive Gamma Correction
  SPISendCommand(0xE0);
  SPISendByte(0x0F);
  SPISendByte(0x31);
  SPISendByte(0x2B);
  SPISendByte(0x0C);
  SPISendByte(0x0E);
  SPISendByte(0x08);
  SPISendByte(0x4E);
  SPISendByte(0xF1);
  SPISendByte(0x37);
  SPISendByte(0x07);
  SPISendByte(0x10);
  SPISendByte(0x03);
  SPISendByte(0x0E);
  SPISendByte(0x09);
  SPISendByte(0x00);

  // Negative Gamma Correction
  SPISendCommand(0xE1);
  SPISendByte(0x00);
  SPISendByte(0x0E);
  SPISendByte(0x14);
  SPISendByte(0x03);
  SPISendByte(0x11);
  SPISendByte(0x07);
  SPISendByte(0x31);
  SPISendByte(0xC1);
  SPISendByte(0x48);
  SPISendByte(0x08);
  SPISendByte(0x0F);
  SPISendByte(0x0C);
  SPISendByte(0x31);
  SPISendByte(0x36);
  SPISendByte(0x0F);

  // Exit Sleep
  SPISendCommand(0x11);

  sleep_ms(100);

  // Display On
  SPISendCommand(0x29);

  sleep_ms(100);

  // Normal display mode on
  SPISendCommand(0x13);

  // Setup drawing full framebuffers

  // Landscape drawing
  // Column Address Set
  SPISendCommand(0x2A);
  SPISendShort(0);
  SPISendShort(kDisplayWidth - 1);
  // Page Address Set
  SPISendCommand(0x2B);
  SPISendShort(0);
  SPISendShort(kDisplayHeight - 1);

  sleep_ms(10);

  SPISendCommand(0x2C);

  ChipSelectEnable();
  DataCommandDisable();

  sleep_ms(10);

  // SPI writes from here out use 16 data bits (for drawing the framebuffer).
  spi_set_format(SPI_PORT, 16, (spi_cpol_t)1, (spi_cpha_t)1, SPI_MSB_FIRST);
}

const int GetWidth() { return kDisplayWidth; }
const int GetHeight() { return kDisplayHeight; }

void UpdatePixelDouble(pw::framebuffer::FramebufferRgb565* frame_buffer) {
  uint16_t temp_row[kDisplayWidth];
  for (int y = 0; y < frame_buffer->height; y++) {
    // Populate this row with each pixel repeated twice
    for (int x = 0; x < frame_buffer->width; x++) {
      temp_row[x * 2] = frame_buffer->pixel_data[y * frame_buffer->width + x];
      temp_row[(x * 2) + 1] =
          frame_buffer->pixel_data[y * frame_buffer->width + x];
    }
    // Send this row to the display twice.
    spi_write16_blocking(SPI_PORT, temp_row, kDisplayWidth);
    spi_write16_blocking(SPI_PORT, temp_row, kDisplayWidth);
  }
}

void Update(pw::framebuffer::FramebufferRgb565* frame_buffer) {
  spi_write16_blocking(
      SPI_PORT, (uint16_t*)frame_buffer->pixel_data, kDisplayDataSize);
}

bool TouchscreenAvailable() { return false; }

bool NewTouchEvent() { return false; }

pw::coordinates::Vec3Int GetTouchPoint() {
  pw::coordinates::Vec3Int point;
  point.x = 0;
  point.y = 0;
  point.z = 0;
  return point;
}

}  // namespace pw::display
