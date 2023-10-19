#include <array>
#include <chrono>
#include <cstddef>
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
constexpr uint16_t kPort = 123;

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
      link_signals_.ready_to_write.acquire();
      std::optional<pw::ByteSpan> buffer = link_.GetWriteBuffer();
      if (!buffer.has_value()) {
        continue;
      }
      for (size_t i = 0; i < buffer->size(); ++i) {
        buffer.value()[i] = std::byte('C');
      }
      buffer.value()[buffer->size() - 1] = std::byte(0);
      const pw::Status status = link_.Write(*buffer);
      if (!status.ok()) {
        PW_LOG_ERROR("Write failed. Error: %s", status.str());
      }
    }
  }

 private:
  pw::data_link::SocketDataLink& link_;
  LinkSignals& link_signals_;
};

}  // namespace

int main() {
  PW_LOG_INFO("Started");
  constexpr size_t kMaxLinks = 2;
  pw::data_link::SocketDataLinkThread<kMaxLinks> links_thread{};

  pw::data_link::ServerSocket server{kMaxLinks};
  PW_CHECK_OK(server.Listen());
  const pw::Result<int> connection_fd = server.Accept();
  PW_CHECK_OK(connection_fd.status());

  LinkSignals reader_link_signals{};
  pw::data_link::SocketDataLink reader_link{
      *connection_fd,
      [&reader_link_signals](pw::data_link::DataLink::Event event,
                             pw::StatusWithSize status) {
        reader_link_signals.last_status = status;
        switch (event) {
          case pw::data_link::DataLink::Event::kOpen: {
            if (!reader_link_signals.last_status.ok()) {
              PW_LOG_ERROR("Read link failed to open: %s",
                           reader_link_signals.last_status.status().str());
              break;
            }
            reader_link_signals.ready_to_write.release();
            reader_link_signals.ready_to_read.release();
          } break;
          case pw::data_link::DataLink::Event::kClosed:
            reader_link_signals.run = false;
            break;
          case pw::data_link::DataLink::Event::kDataReceived:
            reader_link_signals.ready_to_read.release();
            break;
          case pw::data_link::DataLink::Event::kDataRead:
            reader_link_signals.data_read.release();
            break;
          case pw::data_link::DataLink::Event::kDataSent:
            reader_link_signals.ready_to_write.release();
            break;
        }
      }};
  PW_CHECK_OK(links_thread.RegisterLink(reader_link));

  pw::data_link::SocketDataLink writer_link{kLocalHost, kPort};
  PW_CHECK_OK(links_thread.RegisterLink(writer_link));
  LinkSignals writer_link_signals{};
  writer_link.Open([&writer_link_signals](pw::data_link::DataLink::Event event,
                                          pw::StatusWithSize status) {
    writer_link_signals.last_status = status;
    switch (event) {
      case pw::data_link::DataLink::Event::kOpen: {
        if (!writer_link_signals.last_status.ok()) {
          PW_LOG_ERROR("Write link failed to open: %s",
                       writer_link_signals.last_status.status().str());
          break;
        }
        writer_link_signals.ready_to_write.release();
      } break;
      case pw::data_link::DataLink::Event::kClosed:
        writer_link_signals.run = false;
        break;
      case pw::data_link::DataLink::Event::kDataReceived:
        writer_link_signals.ready_to_read.release();
        break;
      case pw::data_link::DataLink::Event::kDataRead:
        writer_link_signals.ready_to_read.release();
        break;
      case pw::data_link::DataLink::Event::kDataSent:
        writer_link_signals.ready_to_write.release();
        break;
    }
  });

  PW_LOG_INFO("Starting links thread");
  pw::thread::DetachedThread(pw::thread::stl::Options(), links_thread);

  PW_LOG_INFO("Starting reader thread");
  Reader reader_thread{reader_link, reader_link_signals};
  pw::thread::DetachedThread(pw::thread::stl::Options(), reader_thread);

  PW_LOG_INFO("Starting writer thread");
  Writer writer_thread{writer_link, writer_link_signals};
  pw::thread::DetachedThread(pw::thread::stl::Options(), writer_thread);

  while (true) {
    std::this_thread::sleep_for(std::chrono::seconds(10));
  }

  links_thread.UnregisterLink(reader_link);
  links_thread.UnregisterLink(writer_link);
  reader_link.Close();
  writer_link.Close();
  links_thread.Stop();
  PW_LOG_INFO("Terminating");
  return 0;
}
