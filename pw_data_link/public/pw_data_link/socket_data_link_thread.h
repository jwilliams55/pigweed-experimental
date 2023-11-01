#pragma once

#include <algorithm>
#include <array>
#include <mutex>

#include "pw_data_link/socket_data_link.h"
#include "pw_status/status.h"
#include "pw_sync/lock_annotations.h"
#include "pw_sync/mutex.h"
#include "pw_sync/thread_notification.h"
#include "pw_thread/thread_core.h"
#include "pw_thread/yield.h"

namespace pw::data_link {

class SocketDataLinkThread : public pw::thread::ThreadCore {
 public:
  explicit SocketDataLinkThread(span<SocketDataLink*> active_links)
      : active_links_(active_links) {}

  pw::Status RegisterLink(SocketDataLink& link) PW_LOCKS_EXCLUDED(lock_) {
    std::lock_guard lock(lock_);
    auto ptr = std::find(active_links_.begin(), active_links_.end(), nullptr);
    if (ptr == active_links_.end()) {
      return pw::Status::ResourceExhausted();
    }
    *ptr = &link;
    return pw::OkStatus();
  }

  pw::Status UnregisterLink(SocketDataLink& link) PW_LOCKS_EXCLUDED(lock_) {
    std::lock_guard lock(lock_);
    auto ptr = std::find(active_links_.begin(), active_links_.end(), &link);
    if (ptr == active_links_.end()) {
      return pw::Status::NotFound();
    }
    *ptr = nullptr;
    return pw::OkStatus();
  }

  void Run() override PW_LOCKS_EXCLUDED(lock_) {
    run_ = true;
    while (run_) {
      std::lock_guard lock(lock_);
      for (SocketDataLink* link : active_links_) {
        if (link != nullptr) {
          link->WaitAndConsumeEvents();
        }
      }
      this_thread::yield();
    }
  }

  void Stop() { run_ = false; }

 private:
  pw::sync::Mutex lock_;
  span<SocketDataLink*> active_links_ PW_GUARDED_BY(lock_);
  bool run_ = false;
};

template <size_t max_links>
class SocketDataLinkThreadWithContainer : public SocketDataLinkThread {
 public:
  SocketDataLinkThreadWithContainer()
      : SocketDataLinkThread(active_links_), active_links_{} {}

 private:
  std::array<SocketDataLink*, max_links> active_links_;
};

}  // namespace pw::data_link
