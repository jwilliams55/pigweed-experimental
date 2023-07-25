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

#include "pw_async_bench/poll_impl.h"

namespace pw::async_bench {

pw::async::Poll<pw::Result<EchoResponse>> ProxyEchoImpl::EchoFuture::Poll(
    pw::async::Waker& waker) {
  // We're in one of two state: before the remote call, or waiting on the remote
  // response.
  if (state_.is_a()) {
    auto& before_call = state_.value_a();
    // Make the remote call.
    state_ = WaitingOnRemote{
        before_call.remote->Echo(std::move(before_call.request))};
  }
  // Check if the remote call is finished.
  auto& waiting_on_remote = state_.value_b();
  return waiting_on_remote.remote_future.Poll(waker);
}

}  // namespace pw::async_bench
