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

#include <cstddef>
#include <cstdint>

namespace pw::spin_delay {

// !!!WARNING!!!: This delay is not truly accurate! It's mostly just a rough
// estimate! Also, it only works in a baremetal context with no interrupts
// getting in the way or threads getting CPU time.
//
// TODO(amontanez): Replace this implementation with a loop checking a
// pw_chrono clock.
void WaitMillis(size_t delay_ms) {
  // Default core clock. This is technically not a constant, but since Pigweed
  // doesn't change the system clock a constant will suffice.
  constexpr uint32_t kSystemCoreClock = 16000000;
  constexpr uint32_t kCyclesPerMs = kSystemCoreClock / 1000;

  // This is not totally accurate, but is close enough.
  for (size_t i = 0; i < delay_ms; i++) {
    // Do a 4 instruction loop enough times to be running for a millisecond.
    // This is set up with assembly rather than a regular loop to make the
    // instruction count predictable (no compiler variation).
    uint32_t cycles = kCyclesPerMs;
    asm volatile(
        " mov r0, %[cycles] \n"
        " mov r1, #0        \n"
        "loop:              \n"
        " cmp r1, r0        \n"
        " itt lt            \n"
        " addlt r1, r1, #4  \n"
        " blt loop          \n"
        // clang-format off
        : /*output=*/
        : /*input=*/[cycles]"r"(cycles)
        : /*clobbers=*/"r0", "r1"
        // clang-format on
    );
  }
}

}  // namespace pw::spin_delay
