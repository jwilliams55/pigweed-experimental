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

#include "pw_board_led/led.h"

#include <cstdint>

#include "board.h"
#include "fsl_gpio.h"

namespace pw::board_led {

#define LED_GPIO BOARD_LED_BLUE_GPIO
constexpr uint32_t kLEDPort = BOARD_LED_BLUE_GPIO_PORT;
constexpr uint8_t kLEDPin = BOARD_LED_BLUE_GPIO_PIN;

void Init() {
  GPIO_PortInit(LED_GPIO, kLEDPort);

  constexpr gpio_pin_config_t config = {
      .pinDirection = kGPIO_DigitalOutput,
      .outputLogic = 0U,
  };

  GPIO_PinInit(LED_GPIO, kLEDPort, kLEDPin, &config);
}

void TurnOff() { GPIO_PinWrite(LED_GPIO, kLEDPort, kLEDPin, 0); }

void TurnOn() { GPIO_PinWrite(LED_GPIO, kLEDPort, kLEDPin, 1); }

void Toggle() { GPIO_PortToggle(LED_GPIO, kLEDPort, kLEDPin); }

}  // namespace pw::board_led
