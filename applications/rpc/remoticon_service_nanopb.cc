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

#include "remoticon/remoticon_service_nanopb.h"

#include <cstring>
#include <span>

#include "pw_status/status.h"

namespace remoticon {

pw::Status SuperloopService::GetStats(
    const remoticon_StatsRequest& /* request */,
    remoticon_StatsResponse& response) {
  // Send back the number of superloop iterations by setting the field in the
  // response proto. In this case, the request proto is unused.
  response.loop_iterations = loop_iterations_;

  // pw_rpc's surrounding code handles serializing the nanopb StatsResponse
  // struct and sending it out.
  //
  // Note: pw_rpc also supports raw methods where you are responsible for
  // serializing and deserializing the request & response.
  return pw::OkStatus();
}

// TODO FOR WORKSHOP: Implement methods for your service here!

}  // namespace remoticon
