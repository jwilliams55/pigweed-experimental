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

#include <array>

#include "pw_board_led/led.h"
#include "pw_hex_dump/hex_dump.h"
#include "pw_log/log.h"
#include "pw_span/span.h"
#include "pw_spin_delay/delay.h"
#include "pw_string/format.h"
#include "pw_string/string_builder.h"

int main() {
  pw::board_led::Init();

  const char my_data[] = "Super Simple Status Logging";
  std::array<char, 80> hex_dump_buffer;
  pw::dump::FormattedHexDumper hex_dumper(hex_dump_buffer);
  hex_dumper.BeginDump(pw::as_bytes(pw::span(my_data)));
  while (hex_dumper.DumpLine().ok()) {
    PW_LOG_INFO("%s", hex_dump_buffer.data());
  }

  unsigned seconds = 0;
  unsigned update_count = 0;

  char buffer[64];
  while (true) {
    pw::string::Format(buffer,
                       "[%d-%02d-%02d %02d:%02d:%02d] Message number: %u",
                       2020,
                       11,
                       8,
                       14,
                       15,
                       seconds,
                       update_count);

    PW_LOG_INFO("%s", buffer);

    pw::board_led::TurnOn();
    pw::spin_delay::WaitMillis(1000);

    seconds = (seconds + 1) % 60;
    update_count = (update_count + 1) % 65535;
  }

  return 0;
}
