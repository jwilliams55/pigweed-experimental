// Copyright 2021 The Pigweed Authors
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

#include "pw_spin_delay/delay.h"

#include <chrono>
#include <cstddef>
#include <cstdint>

namespace {
std::chrono::system_clock::time_point kProgramStart =
    std::chrono::system_clock::now();
}

namespace pw::spin_delay {

void WaitMillis(size_t delay_ms) {
  std::chrono::system_clock::time_point start =
      std::chrono::system_clock::now();
  std::chrono::system_clock::time_point now;
  size_t difference;
  do {
    now = std::chrono::system_clock::now();
    difference =
        std::chrono::duration_cast<std::chrono::milliseconds>(now - start)
            .count();
  } while (difference < delay_ms);
}

uint32_t Millis() {
  std::chrono::system_clock::time_point now;
  uint32_t difference;
  now = std::chrono::system_clock::now();
  difference =
      std::chrono::duration_cast<std::chrono::milliseconds>(now - kProgramStart)
          .count();
  return difference;
}

uint32_t Micros() {
  std::chrono::system_clock::time_point now;
  uint32_t difference;
  now = std::chrono::system_clock::now();
  difference =
      std::chrono::duration_cast<std::chrono::microseconds>(now - kProgramStart)
          .count();
  return difference;
}

}  // namespace pw::spin_delay
