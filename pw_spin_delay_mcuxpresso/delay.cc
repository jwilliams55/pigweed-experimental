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

#include "pw_spin_delay/delay.h"

#include <cstddef>
#include <cstdint>

#include "clock_config.h"
#include "fsl_common.h"
#include "fsl_utick.h"

namespace pw::spin_delay {

namespace {

constexpr uint32_t kTickMicros = 1000;  // Every 1 ms.
bool s_initialized = false;
volatile uint32_t s_msec_count = 0;

void UTickCallback(void) {
  // clang-format off
  s_msec_count++;
  // clang-format on
}

void InitTickTimer() {
  s_initialized = true;
  UTICK_Init(UTICK0);
  UTICK_SetTick(UTICK0, kUTICK_Repeat, kTickMicros, UTickCallback);
}

}  // namespace

void WaitMillis(size_t delay_ms) {
  SDK_DelayAtLeastUs(delay_ms * 1000U, BOARD_BOOTCLOCKRUN_CORE_CLOCK);
}

uint32_t Millis() {
  if (!s_initialized) {
    InitTickTimer();
  }
  return s_msec_count;
}

uint32_t Micros() {
  if (!s_initialized) {
    InitTickTimer();
  }
  return s_msec_count * 1000;
}

}  // namespace pw::spin_delay
