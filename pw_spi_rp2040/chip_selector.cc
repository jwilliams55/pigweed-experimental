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

#include "pw_spi_rp2040/chip_selector.h"

using pw::digital_io::State;

namespace pw::spi {

PicoChipSelector::PicoChipSelector(pw::digital_io::DigitalOut& cs_pin)
    : cs_pin_(cs_pin) {}

Status PicoChipSelector::SetActive(bool active) {
  return cs_pin_.SetState(active ? State::kInactive : State::kActive);
}

}  // namespace pw::spi
