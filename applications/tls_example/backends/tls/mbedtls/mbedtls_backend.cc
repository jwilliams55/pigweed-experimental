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

#include "mbedtls_backend.h"

#include "pw_log/log.h"

namespace {

int MbedtlsWrite(void* ctx, const unsigned char* buf, size_t len) {
  TransportInterface* transport = *static_cast<TransportInterface**>(ctx);
  int status = transport->Write(buf, len);
  return status;
}

int MbedtlsRead(void* ctx, unsigned char* buf, size_t len) {
  TransportInterface* transport = *static_cast<TransportInterface**>(ctx);
  int status = transport->Read(buf, len);
  if (status == 0) {
    return MBEDTLS_ERR_SSL_WANT_READ;
  }
  return status;
}

// A dummy source for generating random bytes for demo purpose only.
// Real applicaiton should provide an implementation.
int DummyEntropySource(void*, unsigned char*, size_t len, size_t* olen) {
  *olen = len;
  return 0;
}

}  // namespace

MbedtlsBackend::MbedtlsBackend() {
  mbedtls_ssl_init(&ssl_);
  mbedtls_ssl_config_init(&conf_);
  mbedtls_x509_crt_init(&cacert_);
  mbedtls_x509_crl_init(&cacrl_);
  mbedtls_ctr_drbg_init(&ctr_drbg_);
  mbedtls_entropy_init(&entropy_);
  mbedtls_ssl_set_bio(&ssl_, &mbedtls_io_ctx_, MbedtlsWrite, MbedtlsRead, NULL);
}

int MbedtlsBackend::SetHostName(const char* host) {
  int ret = 0;
  if ((ret = mbedtls_ssl_set_hostname(&ssl_, host)) != 0) {
    PW_LOG_INFO("Failed to set host name, -0x%x",
                static_cast<unsigned int>(-ret));
    return -1;
  }
  return 0;
}

int MbedtlsBackend::Setup() {
  int ret = 0;
  // Add a dummy entropy source.
  const char* personalization_string = "ssl_client";
  mbedtls_entropy_add_source(
      &entropy_, DummyEntropySource, NULL, 16, MBEDTLS_ENTROPY_SOURCE_STRONG);
  if ((ret = mbedtls_ctr_drbg_seed(&ctr_drbg_,
                                   mbedtls_entropy_func,
                                   &entropy_,
                                   (const unsigned char*)personalization_string,
                                   sizeof(personalization_string) - 1)) != 0) {
    PW_LOG_INFO("Failed to add entroy source, -0x%x",
                static_cast<unsigned int>(-ret));
    return -1;
  }

  if ((ret = mbedtls_ssl_config_defaults(&conf_,
                                         MBEDTLS_SSL_IS_CLIENT,
                                         MBEDTLS_SSL_TRANSPORT_STREAM,
                                         MBEDTLS_SSL_PRESET_DEFAULT)) != 0) {
    PW_LOG_INFO("Failed to setup default config. -0x%x",
                static_cast<unsigned int>(-ret));
    return -1;
  }

  mbedtls_ssl_conf_authmode(&conf_, MBEDTLS_SSL_VERIFY_OPTIONAL);
  mbedtls_ssl_conf_ca_chain(&conf_, &cacert_, &cacrl_);
  mbedtls_ssl_conf_rng(&conf_, mbedtls_ctr_drbg_random, &ctr_drbg_);

  if ((ret = mbedtls_ssl_setup(&ssl_, &conf_)) != 0) {
    PW_LOG_INFO("Failed to set up ssl. -0x%x", static_cast<unsigned int>(-ret));
    return -1;
  }

  return 0;
}

int MbedtlsBackend::Handshake(TransportInterface* transport) {
  if (Setup() < 0) {
    return -1;
  }
  int ret = 0;
  mbedtls_io_ctx_ = transport;
  while ((ret = mbedtls_ssl_handshake(&ssl_)) != 0) {
    if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
      PW_LOG_INFO("Failed to handshake -0x%x", static_cast<unsigned int>(-ret));
      return -1;
    }
  }

  // Check certificate verification result.
  uint32_t flags = 0;
  if ((flags = mbedtls_ssl_get_verify_result(&ssl_)) != 0) {
    char verify_buf[512];
    mbedtls_x509_crt_verify_info(verify_buf, sizeof(verify_buf), "  ! ", flags);
    PW_LOG_INFO("certificate verification failed, %s", verify_buf);
    return -1;
  }

  return 0;
}

int MbedtlsBackend::Write(const void* buffer,
                          size_t size,
                          TransportInterface* transport) {
  mbedtls_io_ctx_ = transport;
  int ret = 0;
  while ((ret = mbedtls_ssl_write(
              &ssl_, static_cast<const unsigned char*>(buffer), size)) <= 0) {
    if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
      PW_LOG_INFO("Failed to write. -0x%x", static_cast<unsigned int>(-ret));
      return -1;
    }
  }
  return static_cast<int>(size);
}

int MbedtlsBackend::Read(void* buffer,
                         size_t size,
                         TransportInterface* transport) {
  mbedtls_io_ctx_ = transport;
  int ret = 0;
  while (true) {
    ret = mbedtls_ssl_read(&ssl_, static_cast<unsigned char*>(buffer), size);
    if (ret < 0) {
      if (ret == MBEDTLS_ERR_SSL_WANT_READ ||
          ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
        continue;
      }
      PW_LOG_INFO("Failed while reading. -0x%x",
                  static_cast<unsigned int>(-ret));
      return -1;
    }
    return ret;
  }
}

int MbedtlsBackend::LoadCACert(const void* buffer, size_t size) {
  int ret = mbedtls_x509_crt_parse(
      &cacert_, static_cast<const unsigned char*>(buffer), size);
  if (ret < 0) {
    PW_LOG_INFO("Failed to load CA certificate. -0x%x",
                static_cast<unsigned int>(-ret));
    return ret;
  }
  return 0;
}

int MbedtlsBackend::LoadCrl(const void* buffer, size_t size) {
  int ret = mbedtls_x509_crl_parse(
      &cacrl_, static_cast<const unsigned char*>(buffer), size);
  if (ret < 0) {
    PW_LOG_INFO("Failed to load crls. -0x%x. Not treated as fatal error.",
                static_cast<unsigned int>(-ret));
    // mbedtls can't handle certain tags in crl, i.e. ASN1 tag.
    // Bail out and continue if it fails.
  }
  return 0;
}

TlsInterface* CreateTls() { return new MbedtlsBackend(); }
