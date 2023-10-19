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

#include <cstdint>

#include "pw_assert/assert.h"
#include "pw_result/result.h"
#include "pw_status/status.h"

namespace pw::data_link {

/// `ServerSocket` wraps a POSIX-style server socket, producing a `SocketStream`
/// for each accepted client connection.
///
/// Call `Listen` to create the socket and start listening for connections.
/// Then call `Accept` any number of times to accept client connections.
class ServerSocket {
 public:
  explicit ServerSocket(int backlog) : backlog_(backlog) {
    PW_DASSERT(backlog > 0);
  }
  ~ServerSocket() { Close(); }

  ServerSocket(const ServerSocket& other) = delete;
  ServerSocket& operator=(const ServerSocket& other) = delete;

  // Listen for connections on the given port.
  // If port is 0, a random unused port is chosen and can be retrieved with
  // port().
  Status Listen(uint16_t port = 0);

  // Accepts a connection. Blocks until after a client is connected.
  Result<int> Accept();

  // Close the server socket, preventing further connections.
  void Close();

  // Returns the port this socket is listening on.
  uint16_t port() const { return port_; }

 private:
  static constexpr int kInvalidFd = -1;

  uint16_t port_ = -1;
  int socket_fd_ = kInvalidFd;
  int backlog_ = 0;
};

}  // namespace pw::data_link
