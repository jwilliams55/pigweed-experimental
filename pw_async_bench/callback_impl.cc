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

#include "pw_async_bench/callback_impl.h"

namespace pw::async_bench {

pw::Status ProxyEchoImpl::Echo(EchoRequest request, EchoResponder responder) {
  // NOTE: this requires that `pw::Function` can capture the full `responder`
  // object as well as any associated state (including possibly `this`)
  // required for further steps. This also doesn't give us a way to cancel the
  // ongoing request if something in `ProxyEchoImpl` goes down.
  //
  // We're also assuming here that each `Echo` call stores some state in the
  // RPC system in order to track the ongoing call and the `pw::Function` we
  // gave it.

  // We have to allocate because `responder` doesn't fit in pw::Function.
  auto responder_alloc = std::make_unique<EchoResponder>(std::move(responder));
  pw::Status status = remote_->Echo(
      std::move(request),
      [responder_alloc = std::move(responder_alloc)](
          pw::Result<EchoResponse> response) mutable -> void {
        // Ignore the result of the send.
        //
        // This, too, requires that the RPC system allocates storage for the
        // ongoing call and advances it. We also don't (naiively) have a way
        // to cancel the call without some kind of handle.
        responder_alloc->Send(std::move(response), [](pw::Status) {})
            .IgnoreError();
      });
  if (status.ok()) {
    // WHat would we do here? We can't use `responder` if it has already been
    // moved into the callback above-- this stinks :(
    // responder.Send(std::move(status), [](pw::Status) {}).IgnoreError();
  }
  return pw::OkStatus();
}

}  // namespace pw::async_bench