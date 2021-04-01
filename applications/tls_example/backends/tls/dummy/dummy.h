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

class DummyTls final : public TlsInterface {
 public:
  DummyTls() {}
  const char* Name() override { return ""; }
  int SetHostName(const char*) override { return 0; }
  int Handshake(TransportInterface*) override { return 0; }
  int Write(const void*, size_t, TransportInterface*) override { return 0; }
  int Read(void*, size_t, TransportInterface*) override { return 0; }
  int LoadCACert(const void*, size_t, X509LoadFormat) override { return 0; }
  int LoadCrl(const void*, size_t, X509LoadFormat) override { return 0; }
};
