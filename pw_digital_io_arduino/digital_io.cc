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

#include "pw_digital_io_arduino/digital_io.h"

#include <Arduino.h>

namespace pw::digital_io {

ArduinoDigitalOut::ArduinoDigitalOut(uint32_t pin) : pin_(pin) {}

Status ArduinoDigitalOut::DoEnable(bool enable) {
  // No way to disable pin in Arduino, but setting disabled pin mode to
  // input should cause failures which should help trigger program bugs.
  pinMode(pin_, enable ? OUTPUT : INPUT);
  return OkStatus();
}

Status ArduinoDigitalOut::DoSetState(State level) {
  digitalWrite(pin_, level == State::kActive ? HIGH : LOW);
  return OkStatus();
}

}  // namespace pw::digital_io
