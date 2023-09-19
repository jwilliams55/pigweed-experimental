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
#pragma once

#include <array>
#include <cstdint>

#include "pw_i2c/address.h"
#include "pw_i2c/initiator.h"
#include "pw_i2c/register_device.h"
#include "pw_status/status.h"

namespace pw::ft6236 {

struct Touch {
  uint16_t x;
  uint16_t y;
  uint8_t weight;
  uint8_t area;
};

class Device {
 public:
  Device(pw::i2c::Initiator& initiator);
  ~Device();

  Status Enable();
  Status Probe();
  void LogControllerInfo();
  void LogTouchInfo();
  Status SetThreshhold(uint8_t threshhold);
  bool ReadData();

 private:
  pw::i2c::Initiator& initiator_;
  pw::i2c::RegisterDevice device_;

  std::array<Touch, 2> touches_;
  uint8_t touch_count_;

  uint32_t delta_micros_ = 0;
  uint32_t this_update_micros_ = 0;
  uint32_t last_update_micros_ = 0;
};

}  // namespace pw::ft6236
