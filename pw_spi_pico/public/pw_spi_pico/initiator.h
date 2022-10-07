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

#include <cstdint>

#include "hardware/spi.h"
#include "pw_spi/initiator.h"

namespace pw::spi {

// Pico SDK userspace implementation of the SPI Initiator
class PicoInitiator : public Initiator {
 public:
  PicoInitiator(spi_inst_t* spi, uint32_t baud_rate);

  void SetOverrideBitsPerWord(BitsPerWord bits_per_word);

  // Implements pw::spi::Initiator:
  Status Configure(const Config& config) override;
  Status WriteRead(ConstByteSpan write_buffer, ByteSpan read_buffer) override;

 private:
  Status LazyInit();

  spi_inst_t* spi_;
  uint32_t baud_rate_;
  Status init_status_;  // The saved LazyInit() status.
  Config config_;
  BitsPerWord desired_bits_per_word_;
  bool override_bits_per_word_ = false;
};

}  // namespace pw::spi
