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
#pragma once

#include <array>
#include <cstdint>
#include <optional>

#include "pw_bytes/span.h"
#include "pw_data_link/data_link.h"
#include "pw_status/status.h"
#include "pw_string/to_string.h"
#include "pw_sync/lock_annotations.h"
#include "pw_sync/mutex.h"

namespace pw::data_link {

class SocketDataLink : public DataLink {
 public:
  SocketDataLink(const char* host, uint16_t port);
  SocketDataLink(int connection_fd, EventHandlerCallback&& event_handler);

  ~SocketDataLink() override PW_LOCKS_EXCLUDED(lock_);

  constexpr size_t mtu() { return kMtu; }
  constexpr size_t max_payload_size() { return kMtu; }

  // Waits for link state changes or events.
  void WaitAndConsumeEvents() PW_LOCKS_EXCLUDED(lock_);

  void Open(EventHandlerCallback&& event_handler) override
      PW_LOCKS_EXCLUDED(lock_);
  void Close() override PW_LOCKS_EXCLUDED(lock_);

  std::optional<ByteSpan> GetWriteBuffer() override;
  Status Write(ByteSpan buffer) override;
  Status Read(ByteSpan buffer) override;

 private:
  static constexpr size_t kMtu = 1024;
  static constexpr int kInvalidFd = -1;

  enum class LinkState {
    kOpen,
    kOpenRequest,
    kClosed,
  } link_state_ PW_GUARDED_BY(lock_) = LinkState::kClosed;

  enum class ReadState {
    kIdle,
    kReadRequested,
  } read_state_ PW_GUARDED_BY(lock_) = ReadState::kIdle;

  enum class WriteState {
    kIdle,
    kWaitingForWrite,  // Buffer was provided.
    kPending,          // Write operation will occur.
  } write_state_ PW_GUARDED_BY(lock_) = WriteState::kIdle;

  void set_link_state(LinkState new_state) PW_EXCLUSIVE_LOCKS_REQUIRED(lock_);

  void DoOpen() PW_UNLOCK_FUNCTION(lock_);
  void DoClose() PW_UNLOCK_FUNCTION(lock_);
  void DoRead() PW_UNLOCK_FUNCTION(lock_);
  void DoWrite() PW_UNLOCK_FUNCTION(lock_);

  const char* host_;
  char port_[6];
  int connection_fd_ PW_GUARDED_BY(lock_) = kInvalidFd;

  std::array<std::byte, kMtu> tx_buffer_storage_{};
  pw::ByteSpan tx_buffer_{};
  size_t num_bytes_to_send_ = 0;
  pw::ByteSpan rx_buffer_{};
  EventHandlerCallback event_handler_;
  pw::sync::Mutex lock_;
};

}  // namespace pw::data_link
