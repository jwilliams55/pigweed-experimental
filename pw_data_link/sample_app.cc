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
#include <utility>

#include "pw_log/levels.h"
#define PW_LOG_LEVEL PW_LOG_LEVEL_INFO

#include "pw_allocator/simple_allocator.h"
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

constexpr size_t kReadBufferSize = 1024;
constexpr size_t kWriteBufferSize = 1024;
constexpr size_t kAllocatorSize = 2 * kWriteBufferSize;

struct LinkSignals {
  std::atomic<bool> run = true;
  pw::sync::ThreadNotification ready_to_read;
  pw::sync::ThreadNotification data_read;
  pw::sync::ThreadNotification ready_to_write;
  // Note: the last_status variable is not thread-safe. It is set in the link
  // event callback, called in the link worker thread, and read in the user
  // reader or writer threads.
  std::atomic<pw::StatusWithSize> last_status = pw::StatusWithSize(0);
};

class LinkThread : public pw::thread::ThreadCore {
 public:
  LinkThread(std::shared_ptr<pw::data_link::SocketDataLink> link,
             std::shared_ptr<LinkSignals> link_signals)
      : link_(link), link_signals_(link_signals), bytes_transferred_(0) {}

  void Run() override {
    while (link_signals_->run) {
      Step();
    }
    end_time_ = std::chrono::high_resolution_clock::now();
    PW_LOG_INFO("LinkThread stopped");
  }

  // Thread must be stopped to avoid data races.
  size_t bytes_transferred() const { return bytes_transferred_; }
  std::chrono::duration<int> transfer_time() const {
    return std::chrono::duration_cast<std::chrono::seconds>(end_time_ -
                                                            start_time_);
  }

  void Stop() {
    link_signals_->run = false;
    link_signals_->ready_to_write.release();
    link_signals_->ready_to_read.release();
  }

 protected:
  virtual void Step() = 0;

  std::shared_ptr<pw::data_link::SocketDataLink> link_;
  std::shared_ptr<LinkSignals> link_signals_;
  size_t bytes_transferred_ = 0;
  bool start_time_set_ = false;
  std::chrono::steady_clock::time_point start_time_;
  std::chrono::steady_clock::time_point end_time_;
};

class Reader : public LinkThread {
 public:
  Reader(std::shared_ptr<pw::data_link::SocketDataLink> link,
         std::shared_ptr<LinkSignals> link_signals)
      : LinkThread(link, link_signals), buffer_() {}

  void Step() override {
    PW_LOG_DEBUG("Waiting to read");
    link_signals_->ready_to_read.acquire();
    if (!link_signals_->run) {
      return;
    }
    PW_LOG_DEBUG("Reading");
    if (const pw::Status status = link_->Read(buffer_); !status.ok()) {
      PW_LOG_ERROR("Failed to read. Error: %s", status.str());
      return;
    }
    if (!start_time_set_) {
      start_time_set_ = true;
      start_time_ = std::chrono::high_resolution_clock::now();
    }
    PW_LOG_DEBUG("Waiting for read to be done");
    link_signals_->data_read.acquire();
    const pw::StatusWithSize status = link_signals_->last_status;
    PW_LOG_DEBUG("Read returned %s (%d bytes)",
                 status.status().str(),
                 static_cast<int>(status.size()));
    if (status.ok()) {
      bytes_transferred_ += status.size();
      // PW_LOG_DEBUG("%s", reinterpret_cast<char*>(buffer_.data()));
    }
  }

 private:
  std::array<std::byte, kReadBufferSize> buffer_{};
};

class Writer : public LinkThread {
 public:
  Writer(std::shared_ptr<pw::data_link::SocketDataLink> link,
         std::shared_ptr<LinkSignals> link_signals)
      : LinkThread(link, link_signals) {}

  void Step() override {
    PW_LOG_DEBUG("Waiting to write");
    link_signals_->ready_to_write.acquire();
    if (!link_signals_->run) {
      return;
    }
    PW_LOG_DEBUG("Waiting for write buffer");
    std::optional<pw::multibuf::MultiBuf> buffer =
        link_->GetWriteBuffer(kWriteBufferSize);
    if (!buffer.has_value()) {
      return;
    }
    if (!start_time_set_) {
      start_time_set_ = true;
      start_time_ = std::chrono::high_resolution_clock::now();
    }
    for (auto& chunk_byte : *buffer) {
      chunk_byte = std::byte('C');
    }
    PW_LOG_DEBUG("Writing");
    const size_t bytes_written = buffer->size();
    const pw::Status status = link_->Write(std::move(*buffer));
    if (status.ok()) {
      bytes_transferred_ += bytes_written;
    } else {
      PW_LOG_ERROR("Write failed. Error: %s", status.str());
    }
  }
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
  const std::chrono::duration<int> test_time = std::chrono::seconds(10);

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

  std::shared_ptr<LinkSignals> link_signals = std::make_shared<LinkSignals>();
  auto event_callback = [&link_signals](pw::data_link::DataLink::Event event,
                                        pw::StatusWithSize status) {
    link_signals->last_status = status;
    switch (event) {
      case pw::data_link::DataLink::Event::kOpen: {
        if (!status.ok()) {
          PW_LOG_ERROR("Link failed to open: %s", status.status().str());
          link_signals->run = false;
        } else {
          PW_LOG_DEBUG("Link open");
        }
        link_signals->ready_to_write.release();
        link_signals->ready_to_read.release();
      } break;
      case pw::data_link::DataLink::Event::kClosed:
        link_signals->run = false;
        link_signals->ready_to_read.release();
        link_signals->ready_to_write.release();
        break;
      case pw::data_link::DataLink::Event::kDataReceived:
        link_signals->ready_to_read.release();
        break;
      case pw::data_link::DataLink::Event::kDataRead:
        link_signals->data_read.release();
        break;
      case pw::data_link::DataLink::Event::kDataSent:
        link_signals->ready_to_write.release();
        break;
    }
  };

  std::shared_ptr<pw::data_link::SocketDataLink> link;
  pw::allocator::SimpleAllocator link_buffer_allocator{};
  std::array<std::byte, kAllocatorSize> allocator_storage{};
  PW_CHECK_OK(link_buffer_allocator.Init(allocator_storage));
  if (is_server) {
    PW_LOG_INFO("Serving on port %d", static_cast<int>(port));
    pw::data_link::ServerSocket server{kMaxLinks};
    PW_CHECK_OK(server.Listen(port));

    PW_LOG_INFO("Waiting for connection");
    const pw::Result<int> connection_fd = server.Accept();
    PW_CHECK_OK(connection_fd.status());

    PW_LOG_INFO("New Connection! Creating Link");
    link = std::make_shared<pw::data_link::SocketDataLink>(
        *connection_fd, std::move(event_callback), link_buffer_allocator);
  } else {
    PW_LOG_INFO("Openning Link");
    link = std::make_shared<pw::data_link::SocketDataLink>(kLocalHost, port);
    link->Open(std::move(event_callback), link_buffer_allocator);
  }

  pw::data_link::SocketDataLinkThreadWithContainer<kMaxLinks> links_thread{};
  PW_CHECK_OK(links_thread.RegisterLink(*link));

  PW_LOG_INFO("Starting links thread");
  pw::thread::DetachedThread(pw::thread::stl::Options(), links_thread);

  std::shared_ptr<LinkThread> link_thread;
  if (is_reader) {
    PW_LOG_INFO("Starting reader thread");
    auto reader_thread = std::make_shared<Reader>(link, link_signals);
    link_thread = reader_thread;
    pw::thread::DetachedThread(pw::thread::stl::Options(), *reader_thread);
  } else {
    PW_LOG_INFO("Starting writer thread");
    auto writer_thread = std::make_shared<Writer>(link, link_signals);
    link_thread = writer_thread;
    pw::thread::DetachedThread(pw::thread::stl::Options(), *writer_thread);
  }

  if (link_signals->run) {
    PW_LOG_INFO("Running for %lld seconds",
                std::chrono::seconds(test_time).count());
    for (int i = 0;
         link_signals->run && i < std::chrono::seconds(test_time).count();
         ++i) {
      sleep(1);
    }
    PW_LOG_INFO("Stopping link's work");
    link_thread->Stop();
  }

  // Wait for link thread to stop.
  // TODO(cachinchilla): Figure out how to join these threads properly.
  sleep(3);

  PW_LOG_INFO("Link transferred %d bytes in %lld seconds",
              static_cast<int>(link_thread->bytes_transferred()),
              std::chrono::seconds(link_thread->transfer_time()).count());

  PW_LOG_INFO("Cleaning up");
  links_thread.UnregisterLink(*link);
  PW_LOG_INFO("Stopping links thread");
  links_thread.Stop();
  PW_LOG_INFO("Terminating");
  return 0;
}
