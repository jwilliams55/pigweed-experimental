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

#include <memory>
#include <optional>
#include <string>

#include "pw_async_bench/base.h"
#include "pw_async_bench/poll.h"
#include "pw_result/result.h"

namespace pw::async_bench {

class RemoteEcho {
 public:
  RemoteEcho() {}

  class EchoFuture {
   public:
    EchoFuture() = default;
    pw::async::Poll<pw::Result<EchoResponse>> Poll(pw::async::Waker& waker);

   private:
    std::string value_;
    bool is_first_time_ = true;
  };
  EchoFuture Echo([[maybe_unused]] EchoRequest request) { return EchoFuture(); }
};

template <typename Impl>
void PostEcho(pw::async::Dispatcher& dispatcher,
              Impl& impl,
              EchoRequest request,
              std::optional<pw::Result<EchoResponse>>& result_out) {
  // This is sort of like a `Task` result oneshot channel.
  // In a more "real" version, we'd use such a channel instead of a manual
  // out pointer (which presents lifetime scenarios if the caller stack may
  // disappear).
  class TaskData {
   public:
    TaskData(std::optional<pw::Result<EchoResponse>>* result_out,
             typename Impl::EchoFuture&& echo_future)
        : result_out_(result_out), echo_future_(std::move(echo_future)) {}

    std::optional<pw::Result<EchoResponse>>* result_out_;
    typename Impl::EchoFuture echo_future_;
  };
  auto task_data =
      std::make_unique<TaskData>(&result_out, impl.Echo(std::move(request)));

  auto task = std::make_unique<pw::async::Task>(
      [task_data = std::move(task_data)](pw::async::Context& context,
                                         pw::Status status) {
        // This status value isn't super meaningful in a poll-based world-- the
        // task itself is alerted to the cancellation by seeing that it has been
        // dropped.
        if (status.IsCancelled()) {
          return;
        }

        pw::async::Waker waker(*context.dispatcher, *context.task);
        pw::async::Poll<pw::Result<EchoResponse>> result =
            task_data->echo_future_.Poll(waker);
        if (result.IsReady()) {
          *task_data->result_out_ = std::optional(std::move(result.value()));
        }
      });

  dispatcher.Post(*task);

  // `pw_async` does not currently provide hooks for knowing
  // when a task has been `Post`ed or when it has been cancelled
  // (no, the `Cancelled` status does not communicate this clearly today),
  // so we need to leak top-level `Task` (or allocate it statically).
  //
  // If we decide to adopt the poll-based model, this should be changed.
  task.release();
}

}  // namespace pw::async_bench
