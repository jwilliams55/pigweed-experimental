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

#if defined(_WIN32) && _WIN32
// TODO(cachinchilla): add support for windows.
#error Windows not supported yet!
#else
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#endif  // defined(_WIN32) && _WIN32

#include <optional>

#include "pw_assert/check.h"
#include "pw_bytes/span.h"
#include "pw_log/log.h"
#include "pw_status/status.h"
#include "pw_status/status_with_size.h"

namespace pw::data_link {
namespace {

const char* kLinkStateNameOpen = "Open";
const char* kLinkStateNameOpenRequest = "kOpenRequest";
const char* kLinkStateNameClosed = "Closed";
const char* kLinkStateNameUnknown = "Unknown";

}  // namespace

SocketDataLink::SocketDataLink(const char* host, uint16_t port) : host_(host) {
  PW_CHECK(host_ != nullptr);
  PW_CHECK(ToString(port, port_).ok());
}

SocketDataLink::SocketDataLink(int connection_fd,
                               EventHandlerCallback&& event_handler)
    : connection_fd_(connection_fd) {
  PW_DCHECK(connection_fd > 0);
  std::lock_guard lock(lock_);
  event_handler_ = std::move(event_handler);
  set_link_state(LinkState::kOpen);
}

SocketDataLink::~SocketDataLink() {
  lock_.lock();
  if (link_state_ != LinkState::kClosed) {
    DoClose();
    return;
  }
  lock_.unlock();
}

void SocketDataLink::set_link_state(LinkState new_state) {
  const char* link_name;
  switch (link_state_) {
    case LinkState::kOpen:
      link_name = kLinkStateNameOpen;
      break;
    case LinkState::kOpenRequest:
      link_name = kLinkStateNameOpenRequest;
      break;
    case LinkState::kClosed:
      link_name = kLinkStateNameClosed;
      break;
    default:
      link_name = kLinkStateNameUnknown;
  };

  const char* new_link_name;
  switch (new_state) {
    case LinkState::kOpen:
      new_link_name = kLinkStateNameOpen;
      break;
    case LinkState::kOpenRequest:
      new_link_name = kLinkStateNameOpenRequest;
      break;
    case LinkState::kClosed:
      new_link_name = kLinkStateNameClosed;
      break;
    default:
      new_link_name = kLinkStateNameUnknown;
  };
  PW_LOG_DEBUG("Transitioning from %s to %s", link_name, new_link_name);
  link_state_ = new_state;
}

void SocketDataLink::Open(EventHandlerCallback&& event_handler) {
  std::lock_guard lock(lock_);
  PW_CHECK(link_state_ == LinkState::kClosed);

  event_handler_ = std::move(event_handler);
  set_link_state(LinkState::kOpenRequest);
}

void SocketDataLink::WaitAndConsumeEvents() {
  // Manually lock and unlock, since some functions may perform unlocking before
  // calling the user's event callback, when locks cannot be held.
  lock_.lock();
  switch (link_state_) {
    case LinkState::kOpen:
      break;
    case LinkState::kClosed:
      break;
    case LinkState::kOpenRequest:
      DoOpen();
      return;
  }

  // Read and Write procedures should be on their own thread, so they don't
  // block each other. For now keep them in a single thread.
  switch (write_state_) {
    case WriteState::kIdle:
      break;
    case WriteState::kWaitingForWrite:
      break;
    case WriteState::kPending:
      DoWrite();
      lock_.lock();
  }

  switch (read_state_) {
    case ReadState::kIdle:
      break;
    case ReadState::kReadRequested:
      DoRead();
      return;
  }
  lock_.unlock();
}

void SocketDataLink::DoOpen() {
  addrinfo hints = {};
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_NUMERICSERV;
  addrinfo* res;
  if (getaddrinfo(host_, port_, &hints, &res) != 0) {
    PW_LOG_ERROR("Failed to configure connection address for socket");
    set_link_state(LinkState::kClosed);
    lock_.unlock();
    event_handler_(DataLink::Event::kOpen, StatusWithSize::InvalidArgument());
    return;
  }

  addrinfo* rp;
  for (rp = res; rp != nullptr; rp = rp->ai_next) {
    connection_fd_ = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if (connection_fd_ != kInvalidFd) {
      break;
    }
  }

  if (connection_fd_ == kInvalidFd) {
    PW_LOG_ERROR("Failed to create a socket: %s", std::strerror(errno));
    set_link_state(LinkState::kClosed);
    lock_.unlock();
    freeaddrinfo(res);
    event_handler_(DataLink::Event::kOpen, StatusWithSize::Unknown());
    return;
  }

// Set necessary options on a socket file descriptor.
#if defined(__APPLE__)
  // Use SO_NOSIGPIPE to avoid getting a SIGPIPE signal when the remote peer
  // drops the connection. This is supported on macOS only.
  constexpr int value = 1;
  if (setsockopt(socket, SOL_SOCKET, SO_NOSIGPIPE, &value, sizeof(int)) < 0) {
    PW_LOG_WARN("Failed to set SO_NOSIGPIPE: %s", std::strerror(errno));
  }
#endif  // defined(__APPLE__)

  if (connect(connection_fd_, rp->ai_addr, rp->ai_addrlen) == -1) {
    close(connection_fd_);
    connection_fd_ = kInvalidFd;
    set_link_state(LinkState::kClosed);
    lock_.unlock();
    PW_LOG_ERROR(
        "Failed to connect to %s:%s: %s", host_, port_, std::strerror(errno));
    freeaddrinfo(res);
    event_handler_(DataLink::Event::kOpen, StatusWithSize::Unknown());
    return;
  }
  set_link_state(LinkState::kOpen);
  lock_.unlock();
  freeaddrinfo(res);
  event_handler_(DataLink::Event::kOpen, StatusWithSize());
}

void SocketDataLink::Close() {
  lock_.lock();
  PW_DCHECK(link_state_ != LinkState::kClosed);
  DoClose();
}

void SocketDataLink::DoClose() {
  set_link_state(LinkState::kClosed);
  if (connection_fd_ != kInvalidFd) {
    close(connection_fd_);
    connection_fd_ = kInvalidFd;
  }
  lock_.unlock();
  event_handler_(DataLink::Event::kClosed, StatusWithSize());
}

std::optional<ByteSpan> SocketDataLink::GetWriteBuffer() {
  std::lock_guard lock(lock_);
  PW_CHECK(link_state_ == LinkState::kOpen);
  if (write_state_ != WriteState::kIdle) {
    return std::nullopt;
  }
  write_state_ = WriteState::kWaitingForWrite;
  return tx_buffer_storage_;
}

Status SocketDataLink::Write(ByteSpan buffer) {
  PW_DCHECK(buffer.size() > 0);
  std::lock_guard lock(lock_);
  PW_DCHECK(link_state_ == LinkState::kOpen);
  if (write_state_ != WriteState::kWaitingForWrite) {
    return pw::Status::FailedPrecondition();
  }

  tx_buffer_ = buffer;
  num_bytes_to_send_ = tx_buffer_.size_bytes();
  write_state_ = WriteState::kPending;
  return OkStatus();
}

void SocketDataLink::DoWrite() {
  int send_flags = 0;
#if defined(__linux__)
  // Use MSG_NOSIGNAL to avoid getting a SIGPIPE signal when the remote
  // peer drops the connection. This is supported on Linux only.
  send_flags |= MSG_NOSIGNAL;
#endif  // defined(__linux__)

  ssize_t bytes_sent = send(connection_fd_,
                            reinterpret_cast<const char*>(tx_buffer_.data()),
                            tx_buffer_.size_bytes(),
                            send_flags);
  if (static_cast<size_t>(bytes_sent) == tx_buffer_.size_bytes()) {
    write_state_ = WriteState::kIdle;
    lock_.unlock();
    event_handler_(DataLink::Event::kDataSent,
                   StatusWithSize(num_bytes_to_send_));
    return;
  }

  if (bytes_sent < 0) {
    if (errno == EPIPE) {
      // An EPIPE indicates that the connection is closed.
      lock_.unlock();
      event_handler_(DataLink::Event::kDataSent, StatusWithSize::OutOfRange());
      Close();
      return;
    }
    lock_.unlock();
    event_handler_(DataLink::Event::kDataSent, StatusWithSize::Unknown());
    return;
  }

  // Partial send.
  tx_buffer_ = tx_buffer_.subspan(static_cast<size_t>(bytes_sent));
  lock_.unlock();
}

Status SocketDataLink::Read(ByteSpan buffer) {
  PW_DCHECK(buffer.size() > 0);
  std::lock_guard lock(lock_);
  PW_DCHECK(link_state_ == LinkState::kOpen);
  if (read_state_ != ReadState::kIdle) {
    return Status::FailedPrecondition();
  }
  rx_buffer_ = buffer;
  read_state_ = ReadState::kReadRequested;
  return OkStatus();
}

void SocketDataLink::DoRead() {
  ssize_t bytes_rcvd = recv(connection_fd_,
                            reinterpret_cast<char*>(rx_buffer_.data()),
                            rx_buffer_.size_bytes(),
                            0);

  if (bytes_rcvd == 0) {
    // Remote peer has closed the connection.
    lock_.unlock();
    event_handler_(DataLink::Event::kDataRead, StatusWithSize::Internal());
    Close();
    return;
  } else if (bytes_rcvd < 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      // Socket timed out when trying to read.
      // This should only occur if SO_RCVTIMEO was configured to be nonzero, or
      // if the socket was opened with the O_NONBLOCK flag to prevent any
      // blocking when performing reads or writes.
      lock_.unlock();
      event_handler_(DataLink::Event::kDataRead,
                     StatusWithSize::ResourceExhausted());
      return;
    }
    lock_.unlock();
    event_handler_(DataLink::Event::kDataRead, StatusWithSize::Unknown());
    return;
  }
  lock_.unlock();
  event_handler_(DataLink::Event::kDataRead, StatusWithSize(bytes_rcvd));
}

}  // namespace pw::data_link
