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

#include <Arduino.h>

#include <cstddef>

namespace pw::spin_delay {

void WaitMillis(size_t delay_ms) { delay(delay_ms); }
uint32_t Millis() { return millis(); }
uint32_t Micros() { return micros(); }

}  // namespace pw::spin_delay
