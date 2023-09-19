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

#include "pw_digital_io_rp2040/digital_io.h"

#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include "pw_status/status.h"

namespace pw::digital_io {

PicoDigitalIn::PicoDigitalIn(uint32_t pin) : pin_(pin) {}

// pw::digital_io::DigitalIn implementation:
Status PicoDigitalIn::DoEnable(bool enable) {
  if (!enable) {
    // Added later: https://github.com/raspberrypi/pico-sdk/issues/792
    // which first appeared in Pico SDK 1.3.0.
    // Cannot find a way to check Pico SDK version at compile time.
    // gpio_deinit(pin_);
    return Status::Unavailable();
  }

  gpio_init(pin_);
  gpio_set_dir(pin_, GPIO_IN);
  return OkStatus();
}

Result<State> PicoDigitalIn::DoGetState() {
  const pw::Result<State> result(State(gpio_get(pin_)));
  return result;
}

// pw::digital_io::DigitalOut implementation:
PicoDigitalOut::PicoDigitalOut(uint32_t pin) : pin_(pin) {}

Status PicoDigitalOut::DoEnable(bool enable) {
  if (!enable) {
    // Added later: https://github.com/raspberrypi/pico-sdk/issues/792
    // which first appeared in Pico SDK 1.3.0.
    // Cannot find a way to check Pico SDK version at compile time.
    // gpio_deinit(pin_);
    return Status::Unavailable();
  }

  gpio_init(pin_);
  gpio_set_dir(pin_, GPIO_OUT);
  return OkStatus();
}

Status PicoDigitalOut::DoSetState(State level) {
  gpio_put(pin_, level == State::kActive);
  return OkStatus();
}

}  // namespace pw::digital_io
