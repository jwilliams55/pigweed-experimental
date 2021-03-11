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

#include <NativeEthernet.h>

#include "backend_interface.h"

class TeensyEthernetTransport final : public TransportInterface {
public:
  TeensyEthernetTransport();
  const char* Name() override { return "teensy-ethernet"; };
  int Connect(const char* ip, int port) override;
  int Write(const void* buffer, size_t size) override;
  int Read(void* buffer, size_t size) override;
private:
  EthernetClient client_;
};
