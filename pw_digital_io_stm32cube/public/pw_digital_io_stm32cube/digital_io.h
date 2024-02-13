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

#pragma once

#include <cstddef>

#include "pw_digital_io/digital_io.h"
#include "pw_digital_io/polarity.h"
#include "stm32cube/stm32cube.h"
#include "stm32f4xx_hal_gpio.h"

namespace pw::digital_io {

struct Stm32CubeConfig {
  GPIO_TypeDef* port;
  uint16_t pin;
  Polarity polarity;

  bool operator==(const Stm32CubeConfig& rhs) const {
    return polarity == rhs.polarity && pin == rhs.pin && port == rhs.port;
  }
  State PhysicalToLogical(const bool hal_value) const {
    return polarity == Polarity::kActiveHigh ? State(hal_value)
                                             : State(!hal_value);
  }
  bool LogicalToPhysical(const State state) const {
    return polarity == Polarity::kActiveHigh ? (bool)state : !(bool)state;
  }
};

class Stm32CubeDigitalOut : public DigitalOut {
 public:
  Stm32CubeDigitalOut(Stm32CubeConfig config);

  // pw::digital_io::DigitalOut implementation:
  Status DoEnable(bool enable) override;
  Status DoSetState(State level) override;

 private:
  Stm32CubeConfig config_;
};

}  // namespace pw::digital_io
