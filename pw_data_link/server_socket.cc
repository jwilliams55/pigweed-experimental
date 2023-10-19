#include "pw_data_link/server_socket.h"

#include <cstdint>

#if defined(_WIN32) && _WIN32
// TODO(cachinchilla): add support for windows.
#error Windows not supported yet!
#else
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#endif  // defined(_WIN32) && _WIN32

#include "pw_status/status.h"

namespace pw::data_link {

// Listen for connections on the given port.
// If port is 0, a random unused port is chosen and can be retrieved with
// port().
Status ServerSocket::Listen(uint16_t port) {
  socket_fd_ = socket(AF_INET6, SOCK_STREAM, 0);
  if (socket_fd_ == kInvalidFd) {
    return Status::Unknown();
  }

  // Allow binding to an address that may still be in use by a closed socket.
  constexpr int value = 1;
  setsockopt(socket_fd_,
             SOL_SOCKET,
             SO_REUSEADDR,
             reinterpret_cast<const char*>(&value),
             sizeof(int));

  if (port != 0) {
    struct sockaddr_in6 addr = {};
    socklen_t addr_len = sizeof(addr);
    addr.sin6_family = AF_INET6;
    addr.sin6_port = htons(port);
    addr.sin6_addr = in6addr_any;
    if (bind(socket_fd_, reinterpret_cast<sockaddr*>(&addr), addr_len) < 0) {
      return Status::Unknown();
    }
  }

  if (listen(socket_fd_, backlog_) < 0) {
    return Status::Unknown();
  }

  // Find out which port the socket is listening on, and fill in port_.
  struct sockaddr_in6 addr = {};
  socklen_t addr_len = sizeof(addr);
  if (getsockname(socket_fd_, reinterpret_cast<sockaddr*>(&addr), &addr_len) <
          0 ||
      static_cast<size_t>(addr_len) > sizeof(addr)) {
    close(socket_fd_);
    return Status::Unknown();
  }

  port_ = ntohs(addr.sin6_port);

  return OkStatus();
}

Result<int> ServerSocket::Accept() {
  struct sockaddr_in6 sockaddr_client_ = {};
  socklen_t len = sizeof(sockaddr_client_);
  const int connection_fd =
      accept(socket_fd_, reinterpret_cast<sockaddr*>(&sockaddr_client_), &len);
  if (connection_fd == kInvalidFd) {
    return Status::Unknown();
  }
  return connection_fd;
}

// Close the server socket, preventing further connections.
void ServerSocket::Close() {
  if (socket_fd_ != kInvalidFd) {
    close(socket_fd_);
    socket_fd_ = kInvalidFd;
  }
}

}  // namespace pw::data_link
