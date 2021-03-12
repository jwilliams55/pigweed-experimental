// Copyright 2021 The Pigweed Authors
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

#include "backend_interface.h"

class BoringsslBackend final : public TlsInterface {
 public:
  BoringsslBackend();
  const char* Name() override { return "boringssl"; }
  int SetHostName(const char* host) override;
  int Handshake(TransportInterface* transport) override;
  int Write(const void* buffer,
            size_t size,
            TransportInterface* transport) override;
  int Read(void* buffer, size_t size, TransportInterface* transport) override;
  int LoadCACert(const void* buffer, size_t size) override;
  int LoadCrl(const void* buffer, size_t size) override;
};
