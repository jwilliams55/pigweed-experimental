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

#include "picotls_backend.h"

#include "boringssl/boringssl_utils.h"
#include "pw_log/log.h"

extern ptls_key_exchange_algorithm_t* ptls_openssl_key_exchanges[];
ptls_cipher_suite_t* cipher_suites[] = {
    &ptls_openssl_aes256gcmsha384, &ptls_openssl_aes128gcmsha256, NULL};

PicotlsBackend::PicotlsBackend() {
  static uint8_t dummy;
  void* smallbuf = static_cast<void*>(&dummy);
  // |ptls_buffer_init| requires a valid buffer, even though it
  // can be of 0 capacity.
  ptls_buffer_init(&read_buffer_, smallbuf, 0);
  ptls_buffer_init(&encode_buffer_, smallbuf, 0);
  ctx_ = {ptls_openssl_random_bytes,
          &ptls_get_time,
          ptls_openssl_key_exchanges,
          cipher_suites};
  hsprop_ = {{{{NULL}}}};
  tls_ = ptls_new(&ctx_, 0);
}

PicotlsBackend::~PicotlsBackend() {
  if (tls_)
    ptls_free(tls_);
}

int PicotlsBackend::SetHostName(const char* host) {
  ptls_set_server_name(tls_, host, 0);
  return 0;
}

int PicotlsBackend::Handshake(TransportInterface* transport) {
  // Initialize certificate validation
  if (trusted_store_) {
    PW_LOG_INFO("Setting upt certificate validation");
    ptls_openssl_init_verify_certificate(&vc_, trusted_store_);
    ctx_.verify_certificate = &vc_.super;
  }
  // |ptls_handshake| only prepares the data int |encode_buffer_|. User
  // takes care of sending the data to the server.
  if (int status = ptls_handshake(tls_, &encode_buffer_, NULL, NULL, &hsprop_);
      status != PTLS_ERROR_IN_PROGRESS) {
    PW_LOG_INFO("Failed to prepare handshake data, %d", status);
    return -1;
  }

  recv_available_ = 0;
  while (true) {
    int read = transport->Read(recv_buffer_ + recv_available_,
                               sizeof(recv_buffer_) - recv_available_);
    if (read < 0) {
      PW_LOG_INFO("Failed to read from transport %d", read);
      return -1;
    }
    recv_available_ += static_cast<size_t>(read);
    size_t processed = recv_available_;
    int status = ptls_handshake(
        tls_, &encode_buffer_, recv_buffer_, &processed, &hsprop_);
    recv_available_ -= processed;
    // Shift the remaining unprocessed data to the start of the buffer.
    memmove(recv_buffer_, recv_buffer_ + processed, recv_available_);
    // Data in |encode_buffer_| must be sent regardless of handshake status.
    if (encode_buffer_.off) {
      if (int write = transport->Write(encode_buffer_.base, encode_buffer_.off);
          write < 0) {
        PW_LOG_INFO("Failed to write to transport %d", write);
        return -1;
      }
      // Clear the buffer.
      encode_buffer_.off = 0;
    }
    if (status == 0) {
      return 0;
    } else if (status == PTLS_ERROR_IN_PROGRESS) {
      continue;
    } else {
      PW_LOG_INFO("handshake error: %d", status);
      return -1;
    }
  }
}

int PicotlsBackend::Write(const void* buffer,
                          size_t size,
                          TransportInterface* transport) {
  // Similar to handshake, |ptls_send| prepares the data only. User sends it
  // to the network.
  if (int status = ptls_send(tls_, &encode_buffer_, buffer, size);
      status != 0) {
    PW_LOG_INFO("Failed to prepare send buffer, %d", status);
    return -1;
  }
  if (int status = transport->Write(encode_buffer_.base, encode_buffer_.off);
      status != static_cast<int>(encode_buffer_.off)) {
    PW_LOG_INFO("Failed to write request %d", status);
    return -1;
  }
  return 0;
}

namespace {
void shift_buffer(ptls_buffer_t* buf, size_t delta) {
  if (delta != 0) {
    assert(delta <= buf->off);
    if (delta != buf->off)
      memmove(buf->base, buf->base + delta, buf->off - delta);
    buf->off -= delta;
  }
}
}  // namespace

int PicotlsBackend::Read(void* buffer,
                         size_t size,
                         TransportInterface* transport) {
  while (true) {
    int read = transport->Read(recv_buffer_ + recv_available_,
                               sizeof(recv_buffer_) - recv_available_);
    if (read < 0) {
      PW_LOG_INFO("Error while reading: %d", read);
      return -1;
    }
    recv_available_ += read;
    size_t processed = recv_available_;
    if (int status =
            ptls_receive(tls_, &read_buffer_, recv_buffer_, &processed);
        status != PTLS_ERROR_IN_PROGRESS && status != 0) {
      PW_LOG_INFO("Recevie parsing error %d\n", status);
      return -1;
    }
    recv_available_ -= processed;
    memmove(recv_buffer_, recv_buffer_ + processed, recv_available_);
    if (read_buffer_.off) {
      size_t to_copy = std::min(size, static_cast<size_t>(read_buffer_.off));
      memcpy(buffer, read_buffer_.base, to_copy);
      shift_buffer(&read_buffer_, to_copy);
      return to_copy;
    }
  }
  return -1;
}

int PicotlsBackend::LoadCACert(const void* buffer,
                               size_t size,
                               X509LoadFormat format) {
  // picotls certificate validation is based on boringssl/opensl X509
  if (!trusted_store_ && (trusted_store_ = X509_STORE_new()) == NULL) {
    PW_LOG_INFO("Failed to create cert store");
    return -1;
  }
  // We don't provide a fixed check time. Thus make sure that time(0) is used
  // to obtain date time.
  X509_VERIFY_PARAM_clear_flags(trusted_store_->param,
                                X509_V_FLAG_USE_CHECK_TIME);
  if (format == X509LoadFormat::kPEM || format == X509LoadFormat::kTryAll) {
    int ret = LoadCACertCrlsPEMFormat(buffer, size, trusted_store_);
    if (ret == 0) {
      return 0;
    } else if (format == X509LoadFormat::kPEM) {
      PW_LOG_INFO("Failed to load CA cert as PEM. %d", ret);
      return ret;
    }
  }
  if (format == X509LoadFormat::kDER || format == X509LoadFormat::kTryAll) {
    int ret = LoadCACertCrlDERFormat(buffer, size, trusted_store_);
    if (ret == 0) {
      return 0;
    } else if (format == X509LoadFormat::kDER) {
      PW_LOG_INFO("Failed to load CA cert as DER. %d", ret);
      return ret;
    }
  }
  return -1;
}

int PicotlsBackend::LoadCrl(const void* buffer,
                            size_t size,
                            X509LoadFormat format) {
  X509_VERIFY_PARAM_set_flags(trusted_store_->param, X509_V_FLAG_CRL_CHECK);
  return LoadCACert(buffer, size, format);
}

TlsInterface* CreateTls() { return new PicotlsBackend(); }
