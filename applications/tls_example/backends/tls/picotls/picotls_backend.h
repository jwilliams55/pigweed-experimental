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
#include "picotls.h"
#include "picotls/openssl.h"

class PicotlsBackend final : public TlsInterface {
 public:
  PicotlsBackend();
  ~PicotlsBackend();
  const char* Name() override { return "picotls"; }
  int SetHostName(const char* host) override;
  int Handshake(TransportInterface* transport) override;
  int Write(const void* buffer,
            size_t size,
            TransportInterface* transport) override;
  int Read(void* buffer, size_t size, TransportInterface* transport) override;
  int LoadCACert(const void* buffer,
                 size_t size,
                 X509LoadFormat format) override;
  int LoadCrl(const void* buffer, size_t size, X509LoadFormat format) override;

 private:
  ptls_buffer_t read_buffer_;
  ptls_buffer_t encode_buffer_;
  ptls_context_t ctx_;
  ptls_handshake_properties_t hsprop_;
  ptls_t* tls_ = nullptr;
  X509_STORE* trusted_store_ = nullptr;
  ptls_openssl_verify_certificate_t vc_;
  // Temporaray receive buffer for raw packet.
  char recv_buffer_[4096];
  size_t recv_available_ = 0;
};
