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

#include "pw_board_led/led.h"
#include "pw_log/log.h"
#include "pw_spin_delay/delay.h"

int main() {
  pw::board_led::Init();
  int i = 0;

  while (true) {
    PW_LOG_INFO("Blink %d", i++);

    pw::board_led::TurnOn();
    pw::spin_delay::WaitMillis(1000);

    pw::board_led::TurnOff();
    pw::spin_delay::WaitMillis(1000);
  }

  return 0;
}
