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

#include <netdb.h>
#include <sys/epoll.h>

#include <array>
#include <cstdint>
#include <optional>

#include "pw_allocator/allocator.h"
#include "pw_bytes/span.h"
#include "pw_data_link/data_link.h"
#include "pw_multibuf/multibuf.h"
#include "pw_status/status.h"
#include "pw_string/to_string.h"
#include "pw_sync/lock_annotations.h"
#include "pw_sync/mutex.h"

namespace pw::data_link {

class SocketDataLink : public DataLink {
 public:
  SocketDataLink(const char* host, uint16_t port);
  SocketDataLink(int connection_fd,
                 EventHandlerCallback&& event_handler,
                 allocator::Allocator& write_buffer_allocator);

  ~SocketDataLink() override PW_LOCKS_EXCLUDED(lock_);

  // Waits for link state changes or events.
  void WaitAndConsumeEvents() PW_LOCKS_EXCLUDED(lock_);

  void Open(EventHandlerCallback&& event_handler,
            allocator::Allocator& write_buffer_allocator) override
      PW_LOCKS_EXCLUDED(lock_);
  void Close() override PW_LOCKS_EXCLUDED(lock_);

  std::optional<multibuf::MultiBuf> GetWriteBuffer(size_t size) override;
  Status Write(multibuf::MultiBuf&& buffer) override;
  Status Read(ByteSpan buffer) override;

 private:
  static constexpr size_t kMtu = 1024;
  static constexpr int kInvalidFd = -1;

  enum class LinkState {
    kOpen,
    kOpenRequest,
    kWaitingForOpen,
    kClosed,
  } link_state_ PW_GUARDED_BY(lock_) = LinkState::kClosed;

  enum class ReadState {
    kIdle,
    kReadRequested,
    kClosed,
  } read_state_ PW_GUARDED_BY(read_lock_) = ReadState::kClosed;

  enum class WriteState {
    kIdle,
    kPending,  // Write operation will occur.
    kClosed,
  } write_state_ PW_GUARDED_BY(write_lock_) = WriteState::kClosed;

  void set_link_state(LinkState new_state) PW_EXCLUSIVE_LOCKS_REQUIRED(lock_);

  void HandleOpenFailure(addrinfo* info) PW_UNLOCK_FUNCTION(lock_);
  bool ConfigureEpoll() PW_EXCLUSIVE_LOCKS_REQUIRED(lock_);

  void DoOpen() PW_UNLOCK_FUNCTION(lock_)
      PW_LOCKS_EXCLUDED(read_lock_, write_lock_);
  void DoClose(bool notify_closed) PW_UNLOCK_FUNCTION(lock_)
      PW_LOCKS_EXCLUDED(read_lock_, write_lock_);
  void DoRead() PW_LOCKS_EXCLUDED(lock_) PW_UNLOCK_FUNCTION(read_lock_);
  void DoWrite() PW_LOCKS_EXCLUDED(lock_) PW_UNLOCK_FUNCTION(write_lock_);

  const char* host_;
  char port_[6];
  int connection_fd_ PW_GUARDED_BY(lock_) = kInvalidFd;
  int epoll_fd_ PW_GUARDED_BY(lock_) = kInvalidFd;
  epoll_event epoll_event_;

  allocator::Allocator* write_buffer_allocator_;
  multibuf::MultiBuf tx_multibuf_ PW_GUARDED_BY(write_lock_);
  size_t num_bytes_to_send_ = 0;
  size_t num_bytes_sent_ = 0;
  ByteSpan rx_buffer_ PW_GUARDED_BY(read_lock_);
  EventHandlerCallback event_handler_;

  // These internal locks must not be acquired when the event handler is called.
  // The main lock cannot be held when acquiring either the read or write locks.
  // However, the main lock can be acquired when one of the read or write locks
  // is held.
  // The read and write locks are held independently and should not overlap.
  sync::Mutex lock_;  // Main lock.
  sync::Mutex read_lock_;
  sync::Mutex write_lock_;
};

}  // namespace pw::data_link
