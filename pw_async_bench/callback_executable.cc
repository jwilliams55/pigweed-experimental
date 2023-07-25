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

#include <memory>
#include <optional>

#include "pw_async/function_dispatcher.h"
#include "pw_async/heap_dispatcher.h"
#include "pw_async_basic/dispatcher.h"
#include "pw_async_bench/callback_impl.h"
#include "pw_thread/thread.h"
#include "pw_thread_stl/options.h"

int main() {
  pw::async::BasicDispatcher basic_dispatcher;
  pw::thread::Thread work_thread(pw::thread::stl::Options(), basic_dispatcher);
  // `HeapDispatcher` is needed so that we can post callbacks without managing
  // `Task` object lifetimes within the class that makes an asynchronous call.
  pw::async::HeapDispatcher heap_dispatcher(basic_dispatcher);
  pw::async_bench::RpcSystem rpc_system(heap_dispatcher);

  const char* ECHO_VALUE = "some value";
  pw::async_bench::EchoRequest request{ECHO_VALUE};
  std::optional<pw::Result<pw::async_bench::EchoResponse>> result_storage(
      std::nullopt);

  pw::async_bench::RemoteEcho remote(rpc_system);
  pw::async_bench::ProxyEchoImpl impl(remote);
  pw::async_bench::PostEcho(
      rpc_system, impl, std::move(request), result_storage);
  basic_dispatcher.RunUntilIdle();

  PW_ASSERT(result_storage.has_value());
  PW_ASSERT(result_storage->ok());
  PW_ASSERT(result_storage->value().value == ECHO_VALUE);
  return 0;
}
