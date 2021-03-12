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

#include <stddef.h>

class TransportInterface {
 public:
  virtual ~TransportInterface() = default;
  // A name to identify the implementation.
  virtual const char* Name() = 0;
  // Returns negative value on failure, other values on success.
  virtual int Connect(const char* ip, int port) = 0;
  // Returns negative value on failure, number of bytes written otherwise.
  virtual int Write(const void* buffer, size_t size) = 0;
  // Returns negative value on failure, number of bytes read otherwise.
  virtual int Read(void* buffer, size_t size) = 0;
};

class TlsInterface {
 public:
  virtual ~TlsInterface() = default;
  // A name to identify the implementation.
  virtual const char* Name() = 0;
  // Returns negative value on failure, other values on success.
  virtual int SetHostName(const char* host) = 0;
  // Returns negative value on failure, other values on success.
  virtual int Handshake(TransportInterface* transport) = 0;
  // Returns negative value on failure, number of bytes written otherwise.
  virtual int Write(const void* buffer,
                    size_t size,
                    TransportInterface* transport) = 0;
  // Returns negative value on failure, number of bytes read otherwise.
  virtual int Read(void* buffer,
                   size_t size,
                   TransportInterface* transport) = 0;
  // Returns negative value on failure, other values on success.
  virtual int LoadCACert(const void* buffer, size_t size) = 0;
  // Returns negative value on failure, other values on success.
  virtual int LoadCrl(const void* buffer, size_t size) = 0;
};

// The following methods shall be implemented by selected backends.
TlsInterface* CreateTls();
TransportInterface* CreateTransport();
