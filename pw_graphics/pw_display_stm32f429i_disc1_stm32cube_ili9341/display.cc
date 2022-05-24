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

#include "pw_color/color.h"
#include "pw_framebuffer/rgb565.h"
#include "stm32cube/stm32cube.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_spi.h"

namespace pw::display {

namespace {

SPI_HandleTypeDef hspi5;

#define HSPI_INSTANCE &hspi5

// CHIP SELECT PIN AND PORT
#define LCD_CS_PORT GPIOC
#define LCD_CS_PIN GPIO_PIN_2

// DATA COMMAND PIN AND PORT
#define LCD_DC_PORT GPIOD
#define LCD_DC_PIN GPIO_PIN_13

#define ILI9341_MADCTL 0x36
#define MADCTL_MY 0x80
#define MADCTL_MX 0x40
#define MADCTL_MV 0x20
#define MADCTL_ML 0x10
#define MADCTL_RGB 0x00
#define MADCTL_BGR 0x08
#define MADCTL_MH 0x04

#define ILI9341_PIXEL_FORMAT_SET 0x3A

constexpr int kDisplayWidth = 320;
constexpr int kDisplayHeight = 240;
constexpr int kDisplayDataSize = kDisplayWidth * kDisplayHeight;

uint16_t internal_framebuffer[kDisplayDataSize];

// SPI Functions
// TODO(tonymd): move to pw_spi
static inline void ChipSelectEnable() {
  HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_RESET);
}

static inline void ChipSelectDisable() {
  HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_SET);
}

static inline void DataCommandEnable() {
  HAL_GPIO_WritePin(LCD_DC_PORT, LCD_DC_PIN, GPIO_PIN_RESET);
}

static inline void DataCommandDisable() {
  HAL_GPIO_WritePin(LCD_DC_PORT, LCD_DC_PIN, GPIO_PIN_SET);
}

static void inline SPISendByte(uint8_t data) {
  ChipSelectEnable();
  DataCommandDisable();
  HAL_SPI_Transmit(HSPI_INSTANCE, &data, 1, 1);
  ChipSelectDisable();
}

static void inline SPISendShort(uint16_t data) {
  ChipSelectEnable();
  DataCommandDisable();

  uint8_t shortBuffer[2];

  shortBuffer[0] = (uint8_t)(data >> 8);
  shortBuffer[1] = (uint8_t)data;

  HAL_SPI_Transmit(HSPI_INSTANCE, shortBuffer, 2, 1);

  ChipSelectDisable();
}

static void inline SPISendCommand(uint8_t command) {
  // set data/command to command mode (low).
  DataCommandEnable();

  ChipSelectEnable();

  // send the command to the display.
  HAL_SPI_Transmit(HSPI_INSTANCE, &command, 1, 1);

  // put the display back into data mode (high).
  DataCommandDisable();

  ChipSelectDisable();
}

void MX_GPIO_Init(void) {
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  // GPIO Ports Clock Enable
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_RESET);

  GPIO_InitStruct.Pin = LCD_CS_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LCD_CS_PORT, &GPIO_InitStruct);

  HAL_GPIO_WritePin(LCD_DC_PORT, LCD_DC_PIN, GPIO_PIN_RESET);

  GPIO_InitStruct.Pin = LCD_DC_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LCD_DC_PORT, &GPIO_InitStruct);

  // Reset pin not connected

  __HAL_RCC_SPI5_CLK_ENABLE();

  // SPI5 GPIO Configuration:
  // PF7 SPI5_SCK
  // PF8 SPI5_MISO
  // PF9 SPI5_MOSI
  GPIO_InitStruct.Pin = GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF5_SPI5;
  HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);
}

void MX_SPI5_Init(void) {
  hspi5.Instance = SPI5;
  hspi5.Init.Mode = SPI_MODE_MASTER;
  hspi5.Init.Direction = SPI_DIRECTION_2LINES;
  hspi5.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi5.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi5.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi5.Init.NSS = SPI_NSS_SOFT;
  hspi5.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  hspi5.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi5.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi5.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi5.Init.CRCPolynomial = 7;
  HAL_SPI_Init(&hspi5);
}

}  // namespace

void Init() {
  MX_GPIO_Init();
  MX_SPI5_Init();
  // CS OFF
  HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_RESET);

  // Init Display
  ChipSelectEnable();

  // ILI9341 Init Sequence:

  // ?
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

  HAL_Delay(100);

  // Display On
  SPISendCommand(0x29);

  HAL_Delay(100);

  // Normal display mode on
  SPISendCommand(0x13);

  // Setup drawing full framebuffers

  // Landscape drawing
  // Column Address Set
  SPISendCommand(0x2A);
  SPISendShort(0);
  SPISendShort(319);
  // Page Address Set
  SPISendCommand(0x2B);
  SPISendShort(0);
  SPISendShort(239);
  SPISendCommand(0x2C);

  ChipSelectEnable();
  DataCommandDisable();

  // SPI writes from here out use 16 data bits (for drawing the framebuffer).
  hspi5.Instance = SPI5;
  hspi5.Init.Mode = SPI_MODE_MASTER;
  hspi5.Init.Direction = SPI_DIRECTION_2LINES;
  hspi5.Init.DataSize = SPI_DATASIZE_16BIT;
  hspi5.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi5.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi5.Init.NSS = SPI_NSS_SOFT;
  hspi5.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  hspi5.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi5.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi5.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi5.Init.CRCPolynomial = 7;
  HAL_SPI_Init(&hspi5);
}

int GetWidth() { return kDisplayWidth; }
int GetHeight() { return kDisplayHeight; }

uint16_t* GetInternalFramebuffer() { return internal_framebuffer; }

void Update(pw::framebuffer::FramebufferRgb565* frame_buffer) {
  // Send 10 rows at a time
  for (int i = 0; i < 24; i++) {
    HAL_SPI_Transmit(HSPI_INSTANCE,
                     (uint8_t*)&internal_framebuffer[320 * (10 * i)],
                     320 * 10,
                     // TODO(tonymd): Figure out what the timeout should be.
                     // 100 here works for 10 rows of 320 pixels.
                     100);
  }
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
