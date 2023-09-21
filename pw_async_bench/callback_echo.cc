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

#include "pw_async_bench/callback_echo.h"

namespace pw::async_bench {

pw::Status EchoResponder::Send(pw::Result<EchoResponse> response,
                               pw::Function<void(pw::Status)> on_sent) {
  send_handler_(std::move(response));

  // Note: we can't capture or wrap other callbacks, even with no additional
  // state, without allocation! This would be annoying for async cases where
  // we want to provide some kind of transformation / processing of a
  // resulting value. :(
  rpc_system_->Dispatcher().Post(
      [on_sent = std::make_unique<pw::Function<void(pw::Status)>>(
           std::move(on_sent))](pw::async::Context&, pw::Status status) {
        if (!status.IsCancelled()) {
          (*on_sent)(pw::OkStatus());
        }
      });
  return pw::OkStatus();
}

pw::Status RemoteEcho::Echo(
    EchoRequest request,
    pw::Function<void(pw::Result<EchoResponse>)> on_response) {
  pw::Result<EchoResponse> response(EchoResponse{
      .value = std::move(request.value),
  });
  // Moving these objects into the pw::Function like this also requires an
  // allocation, or else somewhere different to store the information being
  // sent into the callback.
  struct CallbackState {
    pw::Result<EchoResponse> response;
    pw::Function<void(pw::Result<EchoResponse>)> on_response;
  };
  auto state = std::make_unique<CallbackState>(
      CallbackState{std::move(response), std::move(on_response)});
  rpc_system_->Dispatcher().Post(
      [state = std::move(state)](pw::async::Context&, pw::Status status) {
        if (!status.IsCancelled()) {
          state->on_response(std::move(state->response));
        }
      });
  return pw::OkStatus();
}

}  // namespace pw::async_bench
