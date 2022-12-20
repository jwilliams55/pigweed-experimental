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

#include "pw_digital_io/digital_io.h"
#include "pw_spi/chip_selector.h"

namespace pw::spi {

// Arduino implementation of SPI ChipSelector.
class ArduinoChipSelector : public ChipSelector {
 public:
  ArduinoChipSelector(pw::digital_io::DigitalOut& cs_pin);

  // Implements pw::spi::ChipSelector:
  Status SetActive(bool active) override;

 private:
  pw::digital_io::DigitalOut& cs_pin_;
};

}  // namespace pw::spi