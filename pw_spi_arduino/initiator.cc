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

#include "pw_spi_arduino/initiator.h"

#include <algorithm>

#include "pw_assert/check.h"
#include "pw_log/log.h"
#include "pw_status/try.h"

namespace pw::spi {

namespace {

constexpr uint32_t kMaxClockSpeed = 30'000'000;

constexpr uint8_t GetBitOrder(BitOrder bit_order) {
  switch (bit_order) {
    case BitOrder::kLsbFirst:
      return LSBFIRST;
    case BitOrder::kMsbFirst:
      return MSBFIRST;
  }
  PW_UNREACHABLE;
  return LSBFIRST;
}

SPISettings GetSpiSettings(const Config& config) {
  // https://www.e-tinkers.com/2020/03/do-you-know-arduino-spi-and-arduino-spi-library/
  uint8_t mode = 0;
  if (config.polarity == ClockPolarity::kActiveLow) {
    if (config.phase == ClockPhase::kRisingEdge) {
      mode = SPI_MODE0;
    } else {
      mode = SPI_MODE1;
    }
  } else {
    if (config.phase == ClockPhase::kRisingEdge) {
      mode = SPI_MODE2;
    } else {
      mode = SPI_MODE3;
    }
  }
  return SPISettings(kMaxClockSpeed, GetBitOrder(config.bit_order), mode);
}

}  // namespace

ArduinoInitiator::ArduinoInitiator() : bits_per_word_(8) {}

Status ArduinoInitiator::LazyInit() { return OkStatus(); }

Status ArduinoInitiator::Configure(const Config& config) {
  settings_ = GetSpiSettings(config);
  bits_per_word_ = config.bits_per_word;
  return OkStatus();
}

Status ArduinoInitiator::WriteRead(ConstByteSpan write_buffer,
                                   ByteSpan read_buffer) {
  PW_TRY(LazyInit());

  SPI.beginTransaction(settings_);
  size_t transfer_count = std::max(read_buffer.size(), write_buffer.size());
  if (bits_per_word_() == 16) {
    // TODO(cmumford): Look into hardware SPI.
    // Maybe SAMHardwareSPIOutput, or
    // https://www.pjrc.com/teensy/td_libs_SPI.html, or FastLED/fastspi.h.
    const uint16_t* p = reinterpret_cast<const uint16_t*>(write_buffer.data());
    for (size_t i = 0; i < transfer_count; i++) {
      // TODO(cmumford): Implement read value.
      [[maybe_unused]] uint16_t ignore = SPI.transfer16(*p++);
    }
  } else {
    SPI.transfer(write_buffer.data(), read_buffer.data(), transfer_count);
  }
  SPI.endTransaction();

  return OkStatus();
}

}  // namespace pw::spi
