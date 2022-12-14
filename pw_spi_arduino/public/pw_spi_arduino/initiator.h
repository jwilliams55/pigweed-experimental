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

#include <SPI.h>

#include "pw_spi/initiator.h"

namespace pw::spi {

// Arduino userspace implementation of the SPI Initiator
class ArduinoInitiator : public Initiator {
 public:
  ArduinoInitiator();

  // Implements pw::spi::Initiator:
  Status Configure(const Config& config) override;
  Status WriteRead(ConstByteSpan write_buffer, ByteSpan read_buffer) override;

 private:
  Status LazyInit();

  BitsPerWord bits_per_word_;  // Last Configure() bits_per_word.
  Status init_status_;         // The saved LazyInit() status.
  SPISettings settings_;
};

}  // namespace pw::spi
