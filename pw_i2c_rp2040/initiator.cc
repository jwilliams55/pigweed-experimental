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
#include "pw_i2c_rp2040/initiator.h"

#include <chrono>
#include <mutex>

#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "pico/error.h"
#include "pw_chrono/system_clock.h"
#include "pw_status/status.h"
#include "pw_status/try.h"

namespace pw::i2c {

namespace {

Status PicoStatusToPwStatus(int status) {
  if (status > 0)
    return OkStatus();

  switch (status) {
    case PICO_ERROR_TIMEOUT:
      return Status::DeadlineExceeded();
    default:
      return Status::Unavailable();
  }
}
}  // namespace

void PicoInitiator::Enable() {
  std::lock_guard lock(mutex_);

  if (config_.i2c_block == 0) {
    base_ = i2c0;
  } else {
    base_ = i2c1;
  }
  i2c_init(base_, config_.baud_rate_bps);
  gpio_set_function(config_.sda_pin, GPIO_FUNC_I2C);
  gpio_set_function(config_.scl_pin, GPIO_FUNC_I2C);

  enabled_ = true;
}

void PicoInitiator::Disable() {
  std::lock_guard lock(mutex_);
  i2c_deinit(base_);
  enabled_ = false;
}

PicoInitiator::~PicoInitiator() { Disable(); }

// Performs non-blocking I2C write, read and read-after-write depending on the
// tx and rx buffer states.
Status PicoInitiator::DoWriteReadFor(Address device_address,
                                     ConstByteSpan tx_buffer,
                                     ByteSpan rx_buffer,
                                     chrono::SystemClock::duration timeout) {
  if (timeout <= chrono::SystemClock::duration::zero()) {
    return Status::DeadlineExceeded();
  }
  const int64_t timeout_us = std::chrono::microseconds(timeout).count();

  if (timeout_us > std::numeric_limits<uint>::max()) {
    return Status::InvalidArgument();
  }

  const uint8_t address = device_address.GetSevenBit();
  std::lock_guard lock(mutex_);

  if (!enabled_) {
    return Status::FailedPrecondition();
  }

  if (!tx_buffer.empty() && rx_buffer.empty()) {
    // Write
    int result = i2c_write_timeout_us(
        base_,
        address,
        /*src=*/reinterpret_cast<const uint8_t*>(tx_buffer.data()),
        /*len=*/tx_buffer.size(),
        /*nostop=*/false,
        static_cast<uint>(timeout_us));

    return PicoStatusToPwStatus(result);

  } else if (tx_buffer.empty() && !rx_buffer.empty()) {
    // Read
    int result = i2c_read_timeout_us(
        base_,
        address,
        /*src=*/reinterpret_cast<uint8_t*>(rx_buffer.data()),
        /*len=*/rx_buffer.size(),
        /*nostop=*/false,
        static_cast<uint>(timeout_us));
    return PicoStatusToPwStatus(result);

  } else if (!tx_buffer.empty() && !rx_buffer.empty()) {
    // Write then Read
    pw::Status write_result(PicoStatusToPwStatus(i2c_write_timeout_us(
        base_,
        address,
        /*src=*/reinterpret_cast<const uint8_t*>(tx_buffer.data()),
        /*len=*/tx_buffer.size(),
        /*nostop=*/true,
        static_cast<uint>(timeout_us))));

    if (write_result != OkStatus()) {
      return write_result;
    }

    int read_result = i2c_read_timeout_us(
        base_,
        address,
        /*src=*/reinterpret_cast<uint8_t*>(rx_buffer.data()),
        /*len=*/rx_buffer.size(),
        /*nostop=*/false,
        static_cast<uint>(timeout_us));

    return PicoStatusToPwStatus(read_result);

  } else {
    return Status::InvalidArgument();
  }
}
}  // namespace pw::i2c
