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

#include "pw_digital_io_null/digital_io.h"

#include "pw_log/log.h"
#include "pw_status/status.h"

namespace pw::digital_io {

NullDigitalOut::NullDigitalOut(uint32_t pin) : pin_(pin) {}

Status NullDigitalOut::DoEnable(bool enable) {
  enabled_ = enable;
  PW_LOG_INFO("PIN(%u)[%c]", static_cast<unsigned>(pin_), enabled_ ? '*' : ' ');
  return OkStatus();
}

Status NullDigitalOut::DoSetState(State level) {
  return DoEnable(level == State::kActive);
}

}  // namespace pw::digital_io
