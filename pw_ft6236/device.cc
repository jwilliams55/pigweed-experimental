// Copyright 2023 The Pigweed Authors
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

#include "pw_ft6236/device.h"

#include <chrono>
#include <cstddef>
#include <cstdint>

#include "pw_bytes/bit.h"
#include "pw_i2c/address.h"
#include "pw_i2c/register_device.h"
#include "pw_log/log.h"
#include "pw_status/status.h"

using ::pw::Status;
using namespace std::chrono_literals;

namespace pw::ft6236 {

namespace {

#define FT62XX_REG_THRESHHOLD 0x80
#define FT62XX_REG_POINTRATE 0x88
#define FT62XX_REG_CHIPID 0xA3
#define FT62XX_REG_FIRMVERS 0xA6
#define FT62XX_REG_VENDID 0xA8

constexpr pw::i2c::Address kAddress = pw::i2c::Address::SevenBit<0x38>();

}  // namespace

Device::Device(pw::i2c::Initiator& initiator)
    : initiator_(initiator),
      device_(initiator,
              kAddress,
              endian::little,
              pw::i2c::RegisterAddressSize::k1Byte) {
  delta_micros_ = 0;
  this_update_micros_ = 0;
  last_update_micros_ = 0;
}
Device::~Device() = default;

Status Device::Enable() {
  Result<std::byte> vendor_id_result = device_.ReadRegister(
      FT62XX_REG_VENDID, pw::chrono::SystemClock::for_at_least(10ms));
  if (vendor_id_result.value_or(std::byte{0}) != std::byte{0x11}) {
    return Status::NotFound();
  }

  SetThreshhold(128);

  return OkStatus();
}

Status Device::SetThreshhold(uint8_t threshhold) {
  PW_TRY(device_.WriteRegister(FT62XX_REG_THRESHHOLD,
                               std::byte{threshhold},
                               pw::chrono::SystemClock::for_at_least(10ms)));
  return OkStatus();
}

Status Device::Probe() {
  pw::Status probe_result(initiator_.ProbeDeviceFor(
      kAddress, pw::chrono::SystemClock::for_at_least(10ms)));

  if (probe_result != pw::OkStatus()) {
    PW_LOG_DEBUG("FT6236 Probe Failed");
  }
  PW_LOG_DEBUG("FT6236 Probe Ok");
  return probe_result;
}

void Device::LogControllerInfo() {
  Result<std::byte> result = device_.ReadRegister(
      FT62XX_REG_VENDID, pw::chrono::SystemClock::for_at_least(10ms));
  PW_LOG_DEBUG("Vend ID: 0x%x", (uint8_t)result.value_or(std::byte{0}));

  result = device_.ReadRegister(FT62XX_REG_CHIPID,
                                pw::chrono::SystemClock::for_at_least(10ms));
  PW_LOG_DEBUG("Chip ID: 0x%x (0x36==FT6236)",
               (uint8_t)result.value_or(std::byte{0}));

  result = device_.ReadRegister(FT62XX_REG_FIRMVERS,
                                pw::chrono::SystemClock::for_at_least(10ms));
  PW_LOG_DEBUG("Firmware Version: %u", (uint8_t)result.value_or(std::byte{0}));

  result = device_.ReadRegister(FT62XX_REG_POINTRATE,
                                pw::chrono::SystemClock::for_at_least(10ms));
  PW_LOG_DEBUG("Point Rate Hz: %u", (uint8_t)result.value_or(std::byte{0}));

  result = device_.ReadRegister(FT62XX_REG_THRESHHOLD,
                                pw::chrono::SystemClock::for_at_least(10ms));
  PW_LOG_DEBUG("Threshhold: %u", (uint8_t)result.value_or(std::byte{0}));
}

void Device::LogTouchInfo() {
  if (touch_count_ == 0) {
    return;
  }

  PW_LOG_DEBUG("Touches: %d", touch_count_);

  for (int i = 0; i < touch_count_; i++) {
    PW_LOG_DEBUG("(x,y)=(%d, %d) weight=%d area=%d",
                 touches_[i].x,
                 touches_[i].y,
                 touches_[i].weight,
                 touches_[i].area);
  }
}

bool Device::ReadData() {
  // Read 16 registers starting from 0x00.
  std::array<std::byte, 16> rx_buffer;
  device_.ReadRegisters(
      0, rx_buffer, pw::chrono::SystemClock::for_at_least(10ms));

  // Number of touches (0, 1 or 2) is at 0x02.
  touch_count_ = (uint8_t)rx_buffer[0x02];

  // Return false if no new touches are present.
  if (touch_count_ == 0) {
    return false;
  }

  // Read Touch #1 X coordinate high and low registers.
  touches_[0].x =
      (((uint8_t)rx_buffer[0x03] & 0x0F) << 8) | (uint8_t)rx_buffer[0x04];
  // Read Touch #1 Y coordinate high and low registers.
  touches_[0].y =
      (((uint8_t)rx_buffer[0x05] & 0x0F) << 8) | (uint8_t)rx_buffer[0x06];
  // Read Touch #1 Misc data
  touches_[0].weight = (uint8_t)rx_buffer[0x07];
  touches_[0].area = (uint8_t)rx_buffer[0x08] & 0x0F;

  // Read Touch #2 X coordinate high and low registers.
  touches_[1].x =
      (((uint8_t)rx_buffer[0x09] & 0x0F) << 8) | (uint8_t)rx_buffer[0x0A];
  // Read Touch #2 Y coordinate high and low registers.
  touches_[1].y =
      (((uint8_t)rx_buffer[0x0B] & 0x0F) << 8) | (uint8_t)rx_buffer[0x0C];
  // Read Touch #2 Misc data
  touches_[0].weight = (uint8_t)rx_buffer[0x0D];
  touches_[0].area = (uint8_t)rx_buffer[0x0E] & 0x0F;

  return true;
}

}  // namespace pw::ft6236
