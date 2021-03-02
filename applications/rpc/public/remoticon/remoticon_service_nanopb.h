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
#pragma once

#include <cstring>
#include <span>

#include "pw_status/status.h"
#include "remoticon_proto/remoticon.rpc.pb.h"

namespace remoticon {

class SuperloopService final : public generated::Superloop<SuperloopService> {
 public:
  SuperloopService(unsigned& loop_iterations)
      : loop_iterations_(loop_iterations) {}

  // RPC method - this is exposed through the RPC server once the service is
  // registered.
  pw::Status GetStats(ServerContext&,
                      const remoticon_StatsRequest& request,
                      remoticon_StatsResponse& response);

  // TODO FOR WORKSHOP: Add blink control.

 private:
  // Points to the super loop iteration count; this is updated externally.
  //
  // Note: In production storing an unsynchronized pointer is not a great
  // pattern (no synchronization, no accessor/encapsulation).
  unsigned& loop_iterations_;
};

}  // namespace remoticon
