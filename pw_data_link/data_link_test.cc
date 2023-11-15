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

#include "pw_data_link/data_link.h"

#include <optional>
#include <utility>

#include "gtest/gtest.h"
#include "pw_allocator/allocator.h"
#include "pw_bytes/span.h"
#include "pw_multibuf/multibuf.h"
#include "pw_status/status.h"

namespace pw::data_link {
namespace {

class MockedDataLink : public DataLink {
 public:
  ~MockedDataLink() {}
  constexpr size_t mtu() { return 255; }
  constexpr size_t max_payload_size() { return 255; }
  void Open(EventHandlerCallback&& /*event_handler*/,
            allocator::Allocator& /*write_buffer_allocator*/) override {}
  void Close() override {}
  std::optional<multibuf::MultiBuf> GetWriteBuffer(size_t /*size*/) override {
    return std::nullopt;
  }
  Status Write(multibuf::MultiBuf&& /*buffer*/) override {
    return Status::Unimplemented();
  }
  Status Read(ByteSpan /*buffer*/) override { return Status::Unimplemented(); }
};

TEST(DataLinkTest, CompileTest) {
  MockedDataLink data_link{};
  multibuf::MultiBuf buffer;
  EXPECT_EQ(data_link.Write(std::move(buffer)), Status::Unimplemented());
}

}  // namespace
}  // namespace pw::data_link