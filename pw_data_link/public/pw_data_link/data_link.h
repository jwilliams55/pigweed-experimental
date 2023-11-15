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

#include <optional>

#include "pw_allocator/allocator.h"
#include "pw_bytes/span.h"
#include "pw_function/function.h"
#include "pw_multibuf/multibuf.h"
#include "pw_status/status.h"
#include "pw_status/status_with_size.h"

namespace pw::data_link {

// Generic Data Link interface.
class DataLink {
 public:
  // Events escalated to upper layers.
  enum class Event {
    kOpen,          // The link is now open.
    kClosed,        // The link is now closed.
    kDataReceived,  // Data has been received. Use Read() to fetch it.
    kDataRead,      // Reading into the provided buffer in Read() is complete.
                    // The buffer is now free.
                    // WriteState: kReadPending -> kReadDataReady.
    kDataSent,      // The data in Write() is in the send process and the buffer
                    // provided is now free. The link is ready to send more
                    // data.
  };

  // Logical states:
  //  WriteState - { kWriteIdle, kWritePending }
  //  ReadState  - { kReadIdle,  kReadPending, kReadDataReady }
  //  LinkState  - { kConnected, kConnectionPending, kDisconnected }

  using EventHandlerCallback = pw::Function<void(Event, pw::StatusWithSize)>;

  // Closes the underlying links. There will not be any events after this.
  virtual ~DataLink() = 0;

  // The MTU byte size of the packet which is the payload size plus header(s).
  //
  // This should only be used to size receive buffers to enable zero copying.
  constexpr size_t mtu() { return 0; }

  // Initializes the link peripherals if necessary, and the working thread or
  // threads depending on the implementation.
  // The event handler callback will be called in the working thread.
  // Wait for a kConnected event and check its status.
  //
  // Precondition: link is closed.
  virtual void Open(EventHandlerCallback&& event_handler,
                    allocator::Allocator& write_buffer_allocator) = 0;

  // Closes the underlying link, cancelling any pending operations.
  //
  // Precondition: link is open.
  virtual void Close() = 0;

  // Gets the location where the outgoing data can be written to if there is no
  // ongoing write process. Otherwise, wait for the next kDataSent event.
  //
  // Precondition:
  //   Link is open.
  virtual std::optional<multibuf::MultiBuf> GetWriteBuffer(size_t size) = 0;

  // Writes the entire MultiBuf. The send operation finishes when the kDataSent
  // event is emitted. The event has the operation status and how many bytes
  // were sent, which must be the size of the provided buffer, since no partial
  // writes are supported.
  //
  // Note: If the caller has a MultiBuf partially filled with data to send, they
  // must remove any unused ``MultiBuf::Chunk``s and truncate any partially
  // filled ones to avoid writing the empty ``MultiBuf::Chunk``s.
  //
  // Precondition:
  //   Link is open.
  //   No write operation is in progress.
  //
  // Returns:
  //   OK: The buffer is successfully in the send process.
  //   FAILED_PRECONDITION: A write operation is in process. Wait for the next
  //    kDataSent event.
  //   INVALID_ARGUMENT: The write buffer is empty.
  //
  // To send data:
  // 1. Get a buffer to write to with GetWriteBuffer().
  // 2. Write into buffer.
  // 3. Call write with the buffer written to.
  // 4. Wait for kDataSent.
  // 5. Another buffer can be requested.
  virtual Status Write(multibuf::MultiBuf&& buffer) = 0;

  // Reads a packet into the provided buffer without blocking. The user cannot
  // modify the buffer when Read() is called until the read operation is done
  // and the kDataRead event is emitted. The event has the operation status and
  // how many bytes were read.
  //
  // Precondition:
  //   Link is open.
  //   No read operation is in progress.
  //
  // Returns:
  //  OK: The buffer is successfully in the read process.
  //    Wait for kDataRead event.
  //  FAILED_PRECONDITION: A read operation is in progress. Wait for the next
  //    kDataReceived event.
  //
  // To read data:
  // 1. (Optional) Wait for the kDataReceived event.
  // 2. Pass in the input buffer with the amount of bytes to read.
  // 3. Wait for the kDataRead event.
  // 4. The buffer can be reused now.
  virtual Status Read(pw::ByteSpan buffer) = 0;
};

inline DataLink::~DataLink() {}

}  // namespace pw::data_link
