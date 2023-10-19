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

#include "pw_data_link/socket_data_link.h"

#include <array>
#include <optional>

#include "gtest/gtest.h"
#include "pw_bytes/span.h"
#include "pw_status/status.h"
#include "pw_status/status_with_size.h"

namespace pw::data_link {
namespace {

struct EventCallbackTracker {
  size_t call_count = 0;
  DataLink::Event last_event;
  pw::StatusWithSize last_event_status;
};

TEST(DataLinkTest, CompileTest) {
  SocketDataLink data_link{"localhost", 123};

  EventCallbackTracker callback_tracker;
  data_link.Open(
      [&callback_tracker](DataLink::Event event, pw::StatusWithSize status) {
        callback_tracker.last_event = event;
        callback_tracker.last_event_status = status;
        ++callback_tracker.call_count;
      });

  data_link.WaitAndConsumeEvents();

  ASSERT_EQ(callback_tracker.call_count, 1u);
  EXPECT_EQ(callback_tracker.last_event, DataLink::Event::kOpen);
  ASSERT_NE(callback_tracker.last_event_status.status(), OkStatus());

  // std::array<std::byte, 2> buffer{};
  // EXPECT_EQ(data_link.Write(buffer), Status::Unknown());
  // EXPECT_EQ(data_link.Read(buffer), Status::Unknown());
}

}  // namespace
}  // namespace pw::data_link