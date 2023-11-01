#include <unistd.h>

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <optional>
#include <thread>

#include "pw_assert/check.h"
#include "pw_data_link/data_link.h"
#include "pw_data_link/server_socket.h"
#include "pw_data_link/socket_data_link.h"
#include "pw_data_link/socket_data_link_thread.h"
#include "pw_log/log.h"
#include "pw_result/result.h"
#include "pw_status/status.h"
#include "pw_sync/thread_notification.h"
#include "pw_thread/detached_thread.h"
#include "pw_thread/thread_core.h"
#include "pw_thread_stl/options.h"

namespace {

const char* kLocalHost = "localhost";
constexpr int kDefaultPort = 33001;

struct LinkSignals {
  bool run = true;
  pw::sync::ThreadNotification ready_to_read;
  pw::sync::ThreadNotification data_read;
  pw::sync::ThreadNotification ready_to_write;
  pw::StatusWithSize last_status = pw::StatusWithSize(0);
};

class Reader : public pw::thread::ThreadCore {
 public:
  Reader(pw::data_link::SocketDataLink& link, LinkSignals& link_signals)
      : link_(link), link_signals_(link_signals) {}

  void Run() override {
    while (link_signals_.run) {
      PW_LOG_DEBUG("Waiting to read");
      link_signals_.ready_to_read.acquire();
      if (!link_signals_.run) {
        break;
      }
      PW_LOG_DEBUG("Reading");
      const pw::Status status = link_.Read(buffer_);
      if (!status.ok()) {
        PW_LOG_ERROR("Failed to read. Error: %s", status.str());
        continue;
      }
      PW_LOG_DEBUG("Waiting for read to be done");
      link_signals_.data_read.acquire();
      PW_LOG_DEBUG("Read returned %s (%d bytes)",
                   link_signals_.last_status.status().str(),
                   static_cast<int>(link_signals_.last_status.size()));
      if (link_signals_.last_status.ok()) {
        PW_LOG_INFO("%s", reinterpret_cast<char*>(buffer_.data()));
      }
    }
    PW_LOG_INFO("Reader thread stopped");
  }

 private:
  pw::data_link::SocketDataLink& link_;
  LinkSignals& link_signals_;
  std::array<std::byte, 1024> buffer_;
};

class Writer : public pw::thread::ThreadCore {
 public:
  Writer(pw::data_link::SocketDataLink& link, LinkSignals& link_signals)
      : link_(link), link_signals_(link_signals) {}

  void Run() override {
    while (link_signals_.run) {
      PW_LOG_DEBUG("Waiting to write");
      link_signals_.ready_to_write.acquire();
      if (!link_signals_.run) {
        break;
      }
      PW_LOG_DEBUG("Waiting for write buffer");
      std::optional<pw::ByteSpan> buffer = link_.GetWriteBuffer();
      if (!buffer.has_value()) {
        continue;
      }
      for (size_t i = 0; i < buffer->size(); ++i) {
        buffer.value()[i] = std::byte('C');
      }
      buffer.value()[buffer->size() - 1] = std::byte(0);
      PW_LOG_DEBUG("Writing");
      const pw::Status status = link_.Write(*buffer);
      if (!status.ok()) {
        PW_LOG_ERROR("Write failed. Error: %s", status.str());
      }
    }
    PW_LOG_INFO("Writer thread stopped");
  }

 private:
  pw::data_link::SocketDataLink& link_;
  LinkSignals& link_signals_;
};

}  // namespace

void print_help_menu() {
  std::cout << "Data Link sample app." << std::endl << std::endl;
  std::cout << "Use --server to serve a socket." << std::endl;
  std::cout << "Use --port <NUMBER> to:" << std::endl;
  std::cout << "  - serve a socket on the given port when --server is set, or"
            << std::endl;
  std::cout << "  - connect to a socket on the given port." << std::endl;
  std::cout << "  Defaults to port " << kDefaultPort << "." << std::endl;
  std::cout << "Use --reader to make the link's role read only." << std::endl;
  std::cout << "  Defaults to writer only role." << std::endl;
  std::cout << "Use -h to print this menu and exit." << std::endl;
}

int main(int argc, char** argv) {
  constexpr size_t kMaxLinks = 1;
  bool is_reader = false;
  bool is_server = false;
  int port = kDefaultPort;

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--port") == 0 && i < argc - 1) {
      port = std::atoi(argv[++i]);
    } else if (strcmp(argv[i], "--server") == 0) {
      is_server = true;
    } else if (strcmp(argv[i], "--reader") == 0) {
      is_reader = true;
    } else if (strcmp(argv[i], "-h") == 0) {
      print_help_menu();
      exit(0);
    } else {
      PW_LOG_ERROR("Invalid argument '%s'", argv[i]);
      print_help_menu();
      exit(-1);
    }
  }

  PW_LOG_INFO("Started");

  LinkSignals link_signals{};
  auto event_callback = [&link_signals](pw::data_link::DataLink::Event event,
                                        pw::StatusWithSize status) {
    link_signals.last_status = status;
    switch (event) {
      case pw::data_link::DataLink::Event::kOpen: {
        if (!link_signals.last_status.ok()) {
          PW_LOG_ERROR("Link failed to open: %s",
                       link_signals.last_status.status().str());
          link_signals.run = false;
        } else {
          PW_LOG_DEBUG("Link open");
        }
        link_signals.ready_to_write.release();
        link_signals.ready_to_read.release();
      } break;
      case pw::data_link::DataLink::Event::kClosed:
        link_signals.run = false;
        link_signals.ready_to_read.release();
        link_signals.ready_to_write.release();
        break;
      case pw::data_link::DataLink::Event::kDataReceived:
        link_signals.ready_to_read.release();
        break;
      case pw::data_link::DataLink::Event::kDataRead:
        link_signals.data_read.release();
        break;
      case pw::data_link::DataLink::Event::kDataSent:
        link_signals.ready_to_write.release();
        break;
    }
  };

  std::unique_ptr<pw::data_link::SocketDataLink> link;
  if (is_server) {
    PW_LOG_INFO("Serving on port %d", static_cast<int>(port));
    pw::data_link::ServerSocket server{kMaxLinks};
    PW_CHECK_OK(server.Listen(port));

    PW_LOG_INFO("Waiting for connection");
    const pw::Result<int> connection_fd = server.Accept();
    PW_CHECK_OK(connection_fd.status());

    PW_LOG_INFO("New Connection! Creating Link");
    link = std::make_unique<pw::data_link::SocketDataLink>(*connection_fd,
                                                           event_callback);
  } else {
    PW_LOG_INFO("Openning Link");
    link = std::make_unique<pw::data_link::SocketDataLink>(kLocalHost, port);
    link->Open(event_callback);
  }

  pw::data_link::SocketDataLinkThreadWithContainer<kMaxLinks> links_thread{};
  PW_CHECK_OK(links_thread.RegisterLink(*link));

  PW_LOG_INFO("Starting links thread");
  pw::thread::DetachedThread(pw::thread::stl::Options(), links_thread);

  if (is_reader) {
    PW_LOG_INFO("Starting reader thread");
    Reader reader_thread{*link, link_signals};
    pw::thread::DetachedThread(pw::thread::stl::Options(), reader_thread);
  } else {
    PW_LOG_INFO("Starting writer thread");
    Writer writer_thread{*link, link_signals};
    pw::thread::DetachedThread(pw::thread::stl::Options(), writer_thread);
  }

  if (link_signals.run) {
    PW_LOG_INFO("Running for some time");
    for (int i = 0; i < 30 && link_signals.run; ++i) {
      sleep(1);
    }
  }

  PW_LOG_INFO("Closing link");
  links_thread.UnregisterLink(*link);
  PW_LOG_INFO("Stopping links thread");
  links_thread.Stop();
  PW_LOG_INFO("Terminating");
  return 0;
}
