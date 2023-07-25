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

#include "pw_async/function_dispatcher.h"
#include "pw_async_bench/base.h"
#include "pw_async_bench/callback.h"
#include "pw_result/result.h"
#include "pw_status/status.h"

namespace pw::async_bench {

class RpcSystem {
 public:
  RpcSystem(pw::async::FunctionDispatcher& dispatcher)
      : dispatcher_(&dispatcher) {}

  pw::async::FunctionDispatcher& Dispatcher() const { return *dispatcher_; }

 private:
  pw::async::FunctionDispatcher* dispatcher_;
};

class EchoResponder {
 public:
  EchoResponder(RpcSystem& rpc_system,
                pw::Function<void(pw::Result<EchoResponse>)> send_handler)
      : rpc_system_(&rpc_system), send_handler_(std::move(send_handler)) {}

  pw::Status Send(pw::Result<EchoResponse> response,
                  pw::Function<void(pw::Status)> on_sent);

 private:
  RpcSystem* rpc_system_;
  pw::Function<void(pw::Result<EchoResponse>)> send_handler_;
};

class RemoteEcho {
 public:
  RemoteEcho(RpcSystem& rpc_system) : rpc_system_(&rpc_system) {}

  pw::Status Echo(EchoRequest request,
                  pw::Function<void(pw::Result<EchoResponse>)> on_response);

 private:
  RpcSystem* rpc_system_;
};

template <typename Impl>
void PostEcho(RpcSystem& rpc_system,
              Impl& impl,
              EchoRequest request,
              std::optional<pw::Result<EchoResponse>>& result_out) {
  EchoResponder responder(
      rpc_system, [&result_out](pw::Result<EchoResponse> result) mutable {
        result_out = std::optional(std::move(result));
      });
  pw::Status status = impl.Echo(std::move(request), std::move(responder));
}

}  // namespace pw::async_bench