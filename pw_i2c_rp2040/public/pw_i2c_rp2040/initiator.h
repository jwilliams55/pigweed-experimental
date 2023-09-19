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

#include "hardware/i2c.h"
#include "pw_i2c/initiator.h"
#include "pw_sync/interrupt_spin_lock.h"
#include "pw_sync/lock_annotations.h"
#include "pw_sync/mutex.h"

namespace pw::i2c {

// Initiator interface implementation based on I2C driver in Raspberry Pi Pico
// SDK.  Currently supports only devices with 7 bit adresses.
class PicoInitiator final : public Initiator {
 public:
  struct Config {
    uint32_t i2c_block;  // 0 or 1
    uint32_t baud_rate_bps;
    uint8_t sda_pin;
    uint8_t scl_pin;
  };

  PicoInitiator(const Config& config) : config_(config) {}

  // Should be called before attempting any transfers.
  void Enable() PW_LOCKS_EXCLUDED(mutex_);
  void Disable() PW_LOCKS_EXCLUDED(mutex_);

  ~PicoInitiator() final;

 private:
  Status DoWriteReadFor(Address device_address,
                        ConstByteSpan tx_buffer,
                        ByteSpan rx_buffer,
                        chrono::SystemClock::duration timeout) override
      PW_LOCKS_EXCLUDED(mutex_);

 private:
  sync::Mutex mutex_;
  Config const config_;
  i2c_inst_t* base_;
  bool enabled_ PW_GUARDED_BY(mutex_);
};

}  // namespace pw::i2c
