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

#include "boringssl_backend.h"

#include "pw_log/log.h"

BoringsslBackend::BoringsslBackend() {}

int BoringsslBackend::SetHostName(const char*) {
  PW_LOG_INFO("%s: Not implemented", __func__);
  return 0;
}

int BoringsslBackend::Handshake(TransportInterface*) {
  PW_LOG_INFO("%s: Not implemented", __func__);
  return 0;
}

int BoringsslBackend::Write(const void*, size_t, TransportInterface*) {
  PW_LOG_INFO("%s: Not implemented", __func__);
  return 0;
}

int BoringsslBackend::Read(void*, size_t, TransportInterface*) {
  PW_LOG_INFO("%s: Not implemented", __func__);
  return 0;
}

int BoringsslBackend::LoadCACert(const void*, size_t) {
  PW_LOG_INFO("%s: Not implemented", __func__);
  return 0;
}

int BoringsslBackend::LoadCrl(const void*, size_t) {
  PW_LOG_INFO("%s: Not implemented", __func__);
  return 0;
}

TlsInterface* CreateTls() { return new BoringsslBackend(); }
