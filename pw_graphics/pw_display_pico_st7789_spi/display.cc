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

#include "hardware/pwm.h"
#include "hardware/spi.h"
#include "pico/stdlib.h"
#include "pw_color/color.h"
#include "pw_framebuffer/rgb565.h"

namespace pw::display {

namespace {

// ST7789 Display Registers
#define ST7789_SWRESET 0x01
#define ST7789_TEOFF 0x34
#define ST7789_TEON 0x35
#define ST7789_MADCTL 0x36
#define ST7789_COLMOD 0x3A
#define ST7789_GCTRL 0xB7
#define ST7789_VCOMS 0xBB
#define ST7789_LCMCTRL 0xC0
#define ST7789_VDVVRHEN 0xC2
#define ST7789_VRHS 0xC3
#define ST7789_VDVS 0xC4
#define ST7789_FRCTRL2 0xC6
#define ST7789_PWCTRL1 0xD0
#define ST7789_PORCTRL 0xB2
#define ST7789_GMCTRP1 0xE0
#define ST7789_GMCTRN1 0xE1
#define ST7789_INVOFF 0x20
#define ST7789_SLPOUT 0x11
#define ST7789_DISPON 0x29
#define ST7789_GAMSET 0x26
#define ST7789_DISPOFF 0x28
#define ST7789_RAMWR 0x2C
#define ST7789_INVON 0x21
#define ST7789_CASET 0x2A
#define ST7789_RASET 0x2B

// MADCTL Bits (See page 215: MADCTL (36h): Memory Data Access Control)
#define ST7789_MADCTL_ROW_ORDER 0b10000000
#define ST7789_MADCTL_COL_ORDER 0b01000000
#define ST7789_MADCTL_SWAP_XY 0b00100000
#define ST7789_MADCTL_SCAN_ORDER 0b00010000
#define ST7789_MADCTL_RGB_BGR 0b00001000
#define ST7789_MADCTL_HORIZ_ORDER 0b00000100

// Pico Display Pack 2 Pins
// https://shop.pimoroni.com/products/pico-display-pack-2-0
// --------------------------------------------------------
constexpr int BACKLIGHT_EN = 20;
// spi0 Pins
#define SPI_PORT spi0
constexpr int TFT_SCLK = 18;  // SPI0 SCK
constexpr int TFT_MOSI = 19;  // SPI0 TX
// Unconnected
// const int TFT_MISO = 4;  // SPI0 RX
constexpr int TFT_CS = 17;  // SPI0 CSn
constexpr int TFT_DC = 16;  // GP16
// Reset pin is connected to the Pico reset pin (RUN #30)
// constexpr int TFT_RST = 19;

// Pico Display Pack 2 Size
constexpr int kDisplayWidth = 320;
constexpr int kDisplayHeight = 240;
constexpr int kDisplayDataSize = kDisplayWidth * kDisplayHeight;

// Pico Enviro+ Pack Pins are the same as Display Pack 2
// https://shop.pimoroni.com/products/pico-enviro-pack
// --------------------------------------------------------

// Pico Enviro+ Pack Size
// constexpr int kDisplayWidth = 240;
// constexpr int kDisplayHeight = 240;
// constexpr int kDisplayDataSize = kDisplayWidth * kDisplayHeight;

// PicoSystem
// https://shop.pimoroni.com/products/picosystem
// --------------------------------------------------------
// constexpr int BACKLIGHT_EN = 12;
// #define SPI_PORT spi0
// constexpr int TFT_SCLK = 6;  // SPI0 SCK
// constexpr int TFT_MOSI = 7;  // SPI0 TX
// Unconnected
// const int TFT_MISO = 4;  // SPI0 RX
// constexpr int TFT_CS = 5;  // SPI0 CSn
// constexpr int TFT_DC = 9;  // GP16
// constexpr int TFT_RST = 4;

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

static void inline SPISendCommand(uint8_t command,
                                  size_t data_length,
                                  const char* data) {
  // set data/command to command mode (low).
  DataCommandEnable();
  ChipSelectEnable();

  // send the command to the display.
  spi_write_blocking(SPI_PORT, &command, 1);

  // put the display back into data mode (high).
  DataCommandDisable();
  spi_write_blocking(SPI_PORT, (const uint8_t*)data, data_length);

  ChipSelectDisable();
}

}  // namespace

void Init() {
  stdio_init_all();
  // TODO: This should be a facade
  setup_default_uart();

  uint actual_baudrate = spi_init(SPI_PORT, 62'500'000);
  // NOTE: If the display isn't working try a slower SPI baudrate:
  // uint actual_baudrate = spi_init(SPI_PORT, 31'250'000);

  // Set 8 bit SPI writes.
  spi_set_format(SPI_PORT, 8, (spi_cpol_t)1, (spi_cpha_t)1, SPI_MSB_FIRST);

  // Init backlight PWM
  pwm_config cfg = pwm_get_default_config();
  pwm_set_wrap(pwm_gpio_to_slice_num(BACKLIGHT_EN), 65535);
  pwm_init(pwm_gpio_to_slice_num(BACKLIGHT_EN), &cfg, true);
  gpio_set_function(BACKLIGHT_EN, GPIO_FUNC_PWM);
  // Full Brightness
  pwm_set_gpio_level(BACKLIGHT_EN, 65535);

  // Init Pico SPI
  // gpio_set_function(TFT_MISO, GPIO_FUNC_SPI);  // Unused
  gpio_set_function(TFT_SCLK, GPIO_FUNC_SPI);
  gpio_set_function(TFT_MOSI, GPIO_FUNC_SPI);

  gpio_init(TFT_CS);
  gpio_init(TFT_DC);
  // gpio_init(TFT_RST); // Unused

  gpio_set_dir(TFT_CS, GPIO_OUT);
  gpio_set_dir(TFT_DC, GPIO_OUT);
  // gpio_set_dir(TFT_RST, GPIO_OUT);  // Unused
  gpio_put(TFT_CS, 1);
  gpio_put(TFT_DC, 0);
  // gpio_put(TFT_RST, 0);  // Unused

  // Init Display
  SPISendCommand(ST7789_SWRESET);  // Software reset

  sleep_ms(150);

  SPISendCommand(ST7789_TEON);
  SPISendCommand(ST7789_COLMOD, 1, "\x05");

  SPISendCommand(ST7789_PORCTRL, 5, "\x0c\x0c\x00\x33\x33");
  SPISendCommand(ST7789_LCMCTRL, 1, "\x2c");
  SPISendCommand(ST7789_VDVVRHEN, 1, "\x01");
  SPISendCommand(ST7789_VRHS, 1, "\x12");
  SPISendCommand(ST7789_VDVS, 1, "\x20");
  SPISendCommand(ST7789_PWCTRL1, 2, "\xa4\xa1");
  SPISendCommand(ST7789_FRCTRL2, 1, "\x0f");

  SPISendCommand(ST7789_INVON);
  SPISendCommand(ST7789_SLPOUT);
  SPISendCommand(ST7789_DISPON);

  uint8_t madctl = 0;
  bool rotate_180 = false;

  if (kDisplayWidth == 240 && kDisplayHeight == 240) {
    // Column Address Set
    SPISendCommand(ST7789_CASET);
    SPISendShort(0);
    SPISendShort(kDisplayWidth - 1);
    // Page Address Set
    SPISendCommand(ST7789_RASET);
    SPISendShort(0);
    SPISendShort(kDisplayHeight - 1);
    // TODO: Figure out 240x240 square display MADCTL values for rotation.
    madctl = ST7789_MADCTL_HORIZ_ORDER;
  } else if (kDisplayWidth == 320 && kDisplayHeight == 240) {
    // Landscape drawing
    // Column Address Set
    SPISendCommand(ST7789_CASET);
    SPISendShort(0);
    SPISendShort(kDisplayWidth - 1);
    // Page Address Set
    SPISendCommand(ST7789_RASET);
    SPISendShort(0);
    SPISendShort(kDisplayHeight - 1);

    madctl = ST7789_MADCTL_COL_ORDER;
    if (rotate_180)
      madctl = ST7789_MADCTL_ROW_ORDER;

    madctl |= ST7789_MADCTL_SWAP_XY | ST7789_MADCTL_SCAN_ORDER;
  }

  SPISendCommand(ST7789_MADCTL, 1, (char*)&madctl);

  sleep_ms(50);
}

const int GetWidth() { return kDisplayWidth; }
const int GetHeight() { return kDisplayHeight; }

void SendDisplayWriteCommand() {
  uint8_t command = ST7789_RAMWR;
  // Switch to 8 bit writes.
  spi_set_format(SPI_PORT, 8, (spi_cpol_t)1, (spi_cpha_t)1, SPI_MSB_FIRST);
  DataCommandEnable();
  ChipSelectEnable();
  spi_write_blocking(SPI_PORT, &command, 1);
  DataCommandDisable();
  // Switch to 16 bit writes.
  spi_set_format(SPI_PORT, 16, (spi_cpol_t)1, (spi_cpha_t)1, SPI_MSB_FIRST);
}

void UpdatePixelDouble(pw::framebuffer::FramebufferRgb565* frame_buffer) {
  SendDisplayWriteCommand();

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

  ChipSelectDisable();
}

void Update(pw::framebuffer::FramebufferRgb565* frame_buffer) {
  SendDisplayWriteCommand();

  spi_write16_blocking(
      SPI_PORT, (uint16_t*)frame_buffer->pixel_data, kDisplayDataSize);
  // put the display back into data mode (high).
  ChipSelectDisable();
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
