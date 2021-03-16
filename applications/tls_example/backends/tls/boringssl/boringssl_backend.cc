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

#include "boringssl/boringssl_utils.h"
#include "pw_log/log.h"

namespace {

int BioRead(BIO* bio, char* out, int outl) {
  TransportInterface* transport = static_cast<TransportInterface*>(bio->ptr);
  int status = transport->Read(out, static_cast<size_t>(outl));
  if (status == 0) {
    BIO_set_retry_read(bio);
    return -1;
  }
  return status;
}

int BioWrite(BIO* bio, const char* in, int inl) {
  TransportInterface* transport = static_cast<TransportInterface*>(bio->ptr);
  int status = transport->Write(in, inl);
  return status;
}

static int BioNew(BIO* bio) {
  bio->init = 1;
  return 1;
}

long BioCtrl(BIO*, int, long, void*) { return 1; }

int BioFree(BIO*) { return 1; }

const BIO_METHOD bio_method = {
    BIO_TYPE_MEM,
    "demo bio",
    BioWrite,
    BioRead,
    nullptr, /* puts */
    nullptr, /* gets */
    BioCtrl, /* ctrl */
    BioNew,
    BioFree, /* free */
    nullptr /* callback_ctrl */,
};

}  // namespace

BoringsslBackend::BoringsslBackend()
    : ctx_(SSL_CTX_new(TLS_method())),
      ssl_(SSL_new(ctx_.get())),
      bio_(BIO_new(&bio_method)) {
  SSL_set_bio(ssl_.get(), bio_.get(), bio_.get());
}

int BoringsslBackend::SetHostName(const char* host) {
  SSL_set_tlsext_host_name(ssl_.get(), host);
  return 0;
}

int BoringsslBackend::Handshake(TransportInterface* transport) {
  bio_->ptr = transport;
  while (true) {
    int ret = SSL_connect(ssl_.get());
    if (ret == 1)
      break;
    int ssl_err = SSL_get_error(ssl_.get(), ret);
    if (ssl_err != SSL_ERROR_WANT_READ) {
      PW_LOG_INFO("Error connecting. %d", ssl_err);
      return -1;
    }
  }
  long verify_result = SSL_get_verify_result(ssl_.get());
  if (verify_result) {
    PW_LOG_INFO("x.509 cert verification failed: %ld", verify_result);
  }
  return verify_result == 0 ? 0 : -1;
}

int BoringsslBackend::Write(const void* buffer,
                            size_t size,
                            TransportInterface* transport) {
  bio_->ptr = transport;
  int ssl_ret = SSL_write(ssl_.get(), buffer, size);
  if (ssl_ret <= 0) {
    PW_LOG_INFO("Failed to write");
    return -1;
  }
  return static_cast<int>(size);
}

int BoringsslBackend::Read(void* buffer,
                           size_t size,
                           TransportInterface* transport) {
  bio_->ptr = transport;
  while (true) {
    int ssl_ret = SSL_read(ssl_.get(), buffer, size);
    if (ssl_ret < 0) {
      int ssl_err = SSL_get_error(ssl_.get(), ssl_ret);
      if (ssl_err == SSL_ERROR_WANT_READ) {
        continue;
      }
      PW_LOG_INFO("Error while reading");
      return -1;
    }
    return ssl_ret;
  }
}

int BoringsslBackend::LoadCACert(const void* buffer, size_t size) {
  auto store = SSL_CTX_get_cert_store(ctx_.get());
  // We don't provide a fixed check time. Thus make sure that time(0) is used
  // to obtain date time.
  X509_VERIFY_PARAM_clear_flags(store->param, X509_V_FLAG_USE_CHECK_TIME);
  if (int status = LoadCACertCrls(buffer, size, store); status < 0) {
    PW_LOG_INFO("Failed to load CA cert. %d", status);
    return -1;
  }
  return 0;
}

int BoringsslBackend::LoadCrl(const void* buffer, size_t size) {
  auto store = SSL_CTX_get_cert_store(ctx_.get());
  X509_VERIFY_PARAM_set_flags(store->param, X509_V_FLAG_CRL_CHECK);
  return LoadCACert(buffer, size);
}

TlsInterface* CreateTls() { return new BoringsslBackend(); }
