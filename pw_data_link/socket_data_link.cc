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
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#endif  // defined(_WIN32) && _WIN32

#include <chrono>
#include <optional>

#include "pw_assert/check.h"
#include "pw_bytes/span.h"
#include "pw_data_link/simple_allocator.h"
#include "pw_log/log.h"
#include "pw_status/status.h"
#include "pw_status/status_with_size.h"

namespace pw::data_link {
namespace {

const char* kLinkStateNameOpen = "Open";
const char* kLinkStateNameOpenRequest = "Open Request";
const char* kLinkStateNameClosed = "Closed";
const char* kLinkStateNameWaitingForOpen = "Waiting For Open";
const char* kLinkStateNameUnknown = "Unknown";
const auto kEpollTimeout = std::chrono::seconds(1);

// Configures the file descriptor as non blocking. Returns true when successful.
bool MakeSocketNonBlocking(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags == -1) {
    return false;
  }
  flags += O_NONBLOCK;
  if (fcntl(fd, F_SETFL, flags) == -1) {
    PW_LOG_ERROR("Failed to create a socket: %s", std::strerror(errno));
    return false;
  }
  return true;
}

}  // namespace

SocketDataLink::SocketDataLink(const char* host, uint16_t port) : host_(host) {
  PW_CHECK(host_ != nullptr);
  PW_CHECK(ToString(port, port_).ok());
}

SocketDataLink::SocketDataLink(int connection_fd,
                               EventHandlerCallback&& event_handler,
                               allocator::Allocator& write_buffer_allocator)
    : connection_fd_(connection_fd),
      write_buffer_allocator_(&write_buffer_allocator) {
  PW_DCHECK(connection_fd > 0);
  PW_CHECK(MakeSocketNonBlocking(connection_fd_));
  PW_CHECK(ConfigureEpoll());
  event_handler_ = std::move(event_handler);
  set_link_state(LinkState::kOpen);
  write_state_ = WriteState::kIdle;
  read_state_ = ReadState::kIdle;
  event_handler_(DataLink::Event::kOpen, StatusWithSize());
}

SocketDataLink::~SocketDataLink() {
  lock_.lock();
  if (link_state_ != LinkState::kClosed) {
    DoClose(/*notify_closed=*/true);
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
    case LinkState::kWaitingForOpen:
      link_name = kLinkStateNameWaitingForOpen;
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
    case LinkState::kWaitingForOpen:
      new_link_name = kLinkStateNameWaitingForOpen;
      break;
    default:
      new_link_name = kLinkStateNameUnknown;
  };
  PW_LOG_DEBUG("Transitioning from %s to %s", link_name, new_link_name);
  link_state_ = new_state;
}

void SocketDataLink::Open(EventHandlerCallback&& event_handler,
                          allocator::Allocator& write_buffer_allocator) {
  std::lock_guard lock(lock_);
  PW_CHECK(link_state_ == LinkState::kClosed);

  write_buffer_allocator_ = &write_buffer_allocator;
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
    case LinkState::kWaitingForOpen: {
      // Copy the epoll file descriptor to reduce the critical section.
      const int fd = epoll_fd_;
      lock_.unlock();
      epoll_event event;
      const int count = epoll_wait(
          fd, &event, 1, std::chrono::milliseconds(kEpollTimeout).count());
      if (count == 0) {
        return;
      }
      if (event.events & EPOLLERR || event.events & EPOLLHUP) {
        lock_.lock();
        // Check if link was closed on another thread.
        if (link_state_ != LinkState::kClosed) {
          DoClose(/*notify_closed=*/false);
        } else {
          lock_.unlock();
        }
        event_handler_(DataLink::Event::kOpen, StatusWithSize::Unknown());
        return;
      }
      if (event.events == EPOLLOUT) {
        {
          std::lock_guard lock(lock_);
          set_link_state(LinkState::kOpen);
        }
        {
          std::lock_guard lock(write_lock_);
          write_state_ = WriteState::kIdle;
        }
        {
          std::lock_guard lock(read_lock_);
          read_state_ = ReadState::kIdle;
        }
        event_handler_(DataLink::Event::kOpen, StatusWithSize());
        return;
      }
      PW_LOG_ERROR("Unhandled event %d while waiting to open link",
                   static_cast<int>(event.events));
    }
      return;
    case LinkState::kClosed:
      lock_.unlock();
      return;
    case LinkState::kOpenRequest:
      DoOpen();
      return;
  }
  // Copy the epoll file descriptor to reduce the critical section.
  const int fd = epoll_fd_;
  lock_.unlock();

  epoll_event event;
  const int count = epoll_wait(
      fd, &event, 1, std::chrono::milliseconds(kEpollTimeout).count());
  if (count == 0) {
    return;
  }

  if (event.events & EPOLLERR || event.events & EPOLLHUP) {
    lock_.lock();
    // Check if link was closed on another thread.
    if (link_state_ != LinkState::kClosed) {
      DoClose(/*notify_closed=*/true);
    } else {
      lock_.unlock();
    }
    return;
  }

  if (event.events & EPOLLIN) {
    // Can read!
    read_lock_.lock();
    if (read_state_ == ReadState::kReadRequested) {
      DoRead();
    } else if (read_state_ == ReadState::kIdle) {
      read_lock_.unlock();
      event_handler_(DataLink::Event::kDataReceived, StatusWithSize());
    } else {
      read_lock_.unlock();
    }
  }
  if (event.events & EPOLLOUT) {
    // Can Write!
    write_lock_.lock();
    if (write_state_ == WriteState::kPending) {
      DoWrite();
    } else {
      write_lock_.unlock();
    }
  }
  if ((event.events & EPOLLIN) == 0 && (event.events & EPOLLOUT) == 0) {
    PW_LOG_WARN("Unhandled event %d", static_cast<int>(event.events));
  }
}

void SocketDataLink::DoOpen() {
  addrinfo hints = {};
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_NUMERICSERV;
  addrinfo* res;
  if (getaddrinfo(host_, port_, &hints, &res) != 0) {
    PW_LOG_ERROR("Failed to configure connection address for socket");
    HandleOpenFailure(/*info=*/nullptr);
    return;
  }

  addrinfo* rp;
  for (rp = res; rp != nullptr; rp = rp->ai_next) {
    PW_LOG_DEBUG("Opening socket");
    connection_fd_ = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if (connection_fd_ != kInvalidFd) {
      break;
    }
  }
  if (connection_fd_ == kInvalidFd) {
    HandleOpenFailure(res);
    return;
  }
  // Set necessary options on a socket file descriptor.
  PW_LOG_DEBUG("Configuring socket");
  if (!MakeSocketNonBlocking(connection_fd_)) {
    HandleOpenFailure(res);
    return;
  }
  PW_LOG_DEBUG("Connecting socket");
  if (connect(connection_fd_, rp->ai_addr, rp->ai_addrlen) == -1) {
    if (errno != EINPROGRESS) {
      HandleOpenFailure(res);
      return;
    }
    set_link_state(LinkState::kWaitingForOpen);
  } else {
    set_link_state(LinkState::kOpen);
  }

  PW_LOG_DEBUG("Configuring Epoll");
  if (!ConfigureEpoll()) {
    HandleOpenFailure(res);
    return;
  }
  const bool open_completed = link_state_ == LinkState::kOpen;
  lock_.unlock();
  freeaddrinfo(res);
  if (open_completed) {
    {
      std::lock_guard lock(write_lock_);
      write_state_ = WriteState::kIdle;
    }
    {
      std::lock_guard lock(read_lock_);
      read_state_ = ReadState::kIdle;
    }
    event_handler_(DataLink::Event::kOpen, StatusWithSize());
  }
}

void SocketDataLink::HandleOpenFailure(addrinfo* info) {
  if (connection_fd_ != kInvalidFd) {
    close(connection_fd_);
    connection_fd_ = kInvalidFd;
  }
  if (epoll_fd_ != kInvalidFd) {
    close(epoll_fd_);
    epoll_fd_ = kInvalidFd;
  }
  set_link_state(LinkState::kClosed);
  lock_.unlock();
  PW_LOG_ERROR(
      "Failed to connect to %s:%s: %s", host_, port_, std::strerror(errno));
  if (info != nullptr) {
    freeaddrinfo(info);
  }
  event_handler_(DataLink::Event::kOpen, StatusWithSize::Unknown());
}

bool SocketDataLink::ConfigureEpoll() {
  epoll_fd_ = epoll_create1(0);
  if (epoll_fd_ == kInvalidFd) {
    return false;
  }
  epoll_event_.events = EPOLLOUT | EPOLLIN;
  epoll_event_.data.fd = connection_fd_;
  if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, connection_fd_, &epoll_event_) ==
      -1) {
    return false;
  }
  return true;
}

void SocketDataLink::Close() {
  lock_.lock();
  PW_DCHECK(link_state_ != LinkState::kClosed);
  DoClose(/*notify_closed=*/true);
}

void SocketDataLink::DoClose(bool notify_closed) {
  set_link_state(LinkState::kClosed);
  // Copy file descriptors and unlock to minimize the critical section.
  const int connection_fd = connection_fd_;
  connection_fd_ = kInvalidFd;
  const int epoll_fd = epoll_fd_;
  epoll_fd_ = kInvalidFd;
  lock_.unlock();
  {
    std::lock_guard lock(write_lock_);
    write_state_ = WriteState::kClosed;
  }
  {
    std::lock_guard lock(read_lock_);
    read_state_ = ReadState::kClosed;
  }

  // Close file descriptors if valid.
  if (connection_fd != kInvalidFd) {
    close(connection_fd);
  }
  if (epoll_fd != kInvalidFd) {
    close(epoll_fd);
  }
  if (notify_closed) {
    event_handler_(DataLink::Event::kClosed, StatusWithSize());
  }
}

std::optional<multibuf::MultiBuf> SocketDataLink::GetWriteBuffer(size_t size) {
  if (size == 0) {
    return multibuf::MultiBuf();
  }
  {
    std::lock_guard lock(write_lock_);
    if (write_state_ != WriteState::kIdle) {
      return std::nullopt;
    }
  }
  multibuf::MultiBuf buffer;
  std::optional<multibuf::OwnedChunk> chunk =
      multibuf::HeaderChunkRegionTracker::AllocateRegionAsChunk(
          write_buffer_allocator_, size);
  if (!chunk) {
    return std::nullopt;
  }
  buffer.PushFrontChunk(std::move(*chunk));
  return buffer;
}

Status SocketDataLink::Write(multibuf::MultiBuf&& buffer) {
  if (buffer.size() == 0) {
    return Status::InvalidArgument();
  }
  std::lock_guard lock(write_lock_);
  if (write_state_ != WriteState::kIdle) {
    return Status::FailedPrecondition();
  }

  tx_multibuf_ = std::move(buffer);
  num_bytes_to_send_ = tx_multibuf_.size();
  num_bytes_sent_ = 0;
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

  auto [chunk_iter, chunk] =
      tx_multibuf_.TakeChunk(tx_multibuf_.Chunks().begin());
  ssize_t bytes_sent;
  {
    std::lock_guard lock(lock_);
    // TODO: Send one chunk at a time.
    bytes_sent = send(connection_fd_,
                      reinterpret_cast<const char*>(chunk.data()),
                      chunk.size(),
                      send_flags);
  }

  // Check for errors.
  if (bytes_sent < 0) {
    if (errno == EPIPE) {
      // An EPIPE indicates that the connection is closed.
      tx_multibuf_.Release();
      write_lock_.unlock();
      event_handler_(DataLink::Event::kDataSent, StatusWithSize::OutOfRange());
      Close();
      return;
    }
    tx_multibuf_.Release();
    write_lock_.unlock();
    event_handler_(DataLink::Event::kDataSent, StatusWithSize::Unknown());
    return;
  }
  num_bytes_sent_ += bytes_sent;

  // Resize chunk if it was a partial write.
  if (static_cast<size_t>(bytes_sent) < chunk.size()) {
    chunk->DiscardFront(static_cast<size_t>(bytes_sent));
    tx_multibuf_.PushFrontChunk(std::move(chunk));
    write_lock_.unlock();
    return;
  }

  // Check if we are done with the MultiBuf.
  if (num_bytes_sent_ >= num_bytes_to_send_) {
    write_state_ = WriteState::kIdle;
    tx_multibuf_.Release();
    write_lock_.unlock();
    event_handler_(DataLink::Event::kDataSent,
                   StatusWithSize(num_bytes_to_send_));
    return;
  }

  write_lock_.unlock();
}

Status SocketDataLink::Read(ByteSpan buffer) {
  PW_DCHECK(buffer.size() > 0);
  std::lock_guard lock(read_lock_);
  if (read_state_ != ReadState::kIdle) {
    return Status::FailedPrecondition();
  }
  rx_buffer_ = buffer;
  read_state_ = ReadState::kReadRequested;
  return OkStatus();
}

void SocketDataLink::DoRead() {
  ssize_t bytes_rcvd;
  {
    std::lock_guard lock(lock_);
    bytes_rcvd = recv(connection_fd_,
                      reinterpret_cast<char*>(rx_buffer_.data()),
                      rx_buffer_.size_bytes(),
                      0);
  }

  if (bytes_rcvd == 0) {
    // Remote peer has closed the connection.
    read_state_ = ReadState::kClosed;
    read_lock_.unlock();
    event_handler_(DataLink::Event::kDataRead, StatusWithSize::Internal());
    Close();
    return;
  } else if (bytes_rcvd < 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      // Socket timed out when trying to read.
      // This should only occur if SO_RCVTIMEO was configured to be nonzero, or
      // if the socket was opened with the O_NONBLOCK flag to prevent any
      // blocking when performing reads or writes.
      read_state_ = ReadState::kIdle;
      read_lock_.unlock();
      event_handler_(DataLink::Event::kDataRead,
                     StatusWithSize::ResourceExhausted());
      return;
    }
    read_state_ = ReadState::kIdle;
    read_lock_.unlock();
    event_handler_(DataLink::Event::kDataRead, StatusWithSize::Unknown());
    return;
  }
  read_state_ = ReadState::kIdle;
  read_lock_.unlock();
  event_handler_(DataLink::Event::kDataRead, StatusWithSize(bytes_rcvd));
}

}  // namespace pw::data_link
