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

#include "pw_digital_io_stm32cube/digital_io.h"

#include "pw_assert/assert.h"
#include "pw_status/status.h"

namespace pw::digital_io {

namespace {

void InitGpio(GPIO_TypeDef* port, uint16_t pin) {
  GPIO_InitTypeDef init_data = {
      .Pin = pin,
      .Mode = GPIO_MODE_OUTPUT_PP,
      .Pull = GPIO_NOPULL,
      .Speed = GPIO_SPEED_FREQ_LOW,
  };
  HAL_GPIO_WritePin(port, pin, GPIO_PIN_RESET);
  HAL_GPIO_Init(port, &init_data);
}

}  // namespace

Stm32CubeDigitalOut::Stm32CubeDigitalOut(GPIO_TypeDef* port, uint16_t pin)
    : port_(port), pin_(pin) {}

// pw::digital_io::DigitalOut implementation:
Status Stm32CubeDigitalOut::DoEnable(bool enable) {
  if (!enable) {
    // Doesn't seem to be supported by the SDK.
    return Status::Unavailable();
  }
  InitGpio(port_, pin_);
  return OkStatus();
}

Status Stm32CubeDigitalOut::DoSetState(State level) {
  HAL_GPIO_WritePin(
      port_, pin_, level == State::kActive ? GPIO_PIN_SET : GPIO_PIN_RESET);
  return OkStatus();
}

}  // namespace pw::digital_io
