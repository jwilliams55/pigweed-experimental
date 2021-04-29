// Copyright 2021 The Pigweed Authors
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

#include "pw_board_led/led.h"

#include <cinttypes>

#include "stm32cube/stm32cube.h"

namespace pw::board_led {

#define _CAT(A, B, C) A##B##C
#define CAT(A, B, C) _CAT(A, B, C)

#define LED_PORT CAT(GPIO, LED_PORT_CHAR, )
#define LED_PIN CAT(GPIO_PIN_, LED_PIN_NUM, )

// This is hacky, but works. It is needed because there is no function to
// initialize an arbitrary GPIO port.
#define LED_PORT_ENABLE CAT(__HAL_RCC_GPIO, LED_PORT_CHAR, _CLK_ENABLE)

void Init() {
  GPIO_InitTypeDef GPIO_InitStruct = {};

  LED_PORT_ENABLE();

  HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET);

  GPIO_InitStruct.Pin = LED_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED_PORT, &GPIO_InitStruct);
}

void TurnOff() { HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET); }

void TurnOn() { HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET); }

void Toggle() { HAL_GPIO_TogglePin(LED_PORT, LED_PIN); }

}  // namespace pw::board_led
