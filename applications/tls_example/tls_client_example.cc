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

#include <sys/types.h>

#include <span>

#include "backends/backend_interface.h"
#include "pw_log/log.h"
#include "pw_spin_delay/delay.h"
#include "pw_sys_io/sys_io.h"
#include "trust_store/ca_certificates_crls.h"

#define GOOGLE_IP_ADDRESS "172.217.7.228"
#define GOOGLE_SERVER_NAME "www.google.com"
#define HTTPS_PORT 443
#define HTTPS_REQUEST "GET / HTTP/1.0\r\n\r\n"

namespace {

void MyAbort() {
  PW_LOG_INFO("abort");
  while (1)
    ;
}

#ifdef CRL_CHECK
// The following are CRLs previously downloaded from the above CAs and they
// are likely to be expired when application is compiled. To perform CRL
// check, make sure that you download the latest CRL from CA into folder
// applications/tls_example/trust_store and re-run
// applications/tls_example/trust_store/generate_cert_crl_header_file.py
// to update the header. See applications/tls_example/trust_store/README.md
// for more detail.
//
// TODO(zyecheng): Alternatively, consider build time CRL download and
// injection for demo purpose.
const char kCrls[] = {GLOBAL_SIGN_CA_CRL GTS_CA_101_CRL};
#endif

void TlsClientExample() {
  TlsInterface* tls = CreateTls();
  TransportInterface* transport = CreateTransport();
  PW_LOG_INFO("tls: %s, transport: %s", tls->Name(), transport->Name());

  uint32_t start_ms = pw::spin_delay::Millis();

  // Connects to server.
  PW_LOG_INFO("Connecting to %s:%d", GOOGLE_SERVER_NAME, HTTPS_PORT);
  if (int status = transport->Connect(GOOGLE_IP_ADDRESS, HTTPS_PORT);
      status < 0) {
    PW_LOG_INFO("Failed to connect to google, %d", status);
    MyAbort();
  }
  PW_LOG_INFO("Connected. Time elapsed: %lums",
              pw::spin_delay::Millis() - start_ms);

  // Sets host name (google server requires SNI extention).
  if (int status = tls->SetHostName(GOOGLE_SERVER_NAME); status < 0) {
    PW_LOG_INFO("Failed to set host name, %d", status);
    MyAbort();
  }

  // Loads trusted CA certificates.
  auto builtin_certs = GetBuiltInRootCert();
  PW_LOG_INFO("Found %zu built-in CA certificates", builtin_certs.size());
  for (auto cert : builtin_certs) {
    PW_LOG_INFO("loading cert");
    if (int status = tls->LoadCACert(cert.data(), cert.size()); status < 0) {
      PW_LOG_INFO("Failed to load trusted CA certificates, %d", status);
      MyAbort();
    }
  }

#ifdef CRL_CHECK
  // Loads CRLs.
  if (int status = tls->LoadCrl(kCrls, sizeof(kCrls)); status < 0) {
    PW_LOG_INFO("Failed to load crls, %d", status);
    MyAbort();
  }
#endif

  // Performs TLS handshake.
  PW_LOG_INFO("Performing handshake...");
  if (int status = tls->Handshake(transport); status < 0) {
    PW_LOG_INFO("Failed to handshake, %d", status);
    MyAbort();
  }
  PW_LOG_INFO("Done. Time elapsed: %lums", pw::spin_delay::Millis() - start_ms);

  // Sends a https request.
  PW_LOG_INFO("Sending https request...");
  if (int status = tls->Write(HTTPS_REQUEST, sizeof(HTTPS_REQUEST), transport);
      status < 0) {
    PW_LOG_INFO("Failed to send https request, %d", status);
    MyAbort();
  }
  PW_LOG_INFO("Done");

  // Reads response.
  PW_LOG_INFO("Listening for response...");
  char recv_buffer[4096];
  while (true) {
    int read = 0;
    if (read = tls->Read(recv_buffer, sizeof(recv_buffer), transport);
        read < 0) {
      PW_LOG_INFO("Error while reading, %d", read);
      MyAbort();
    }
    // Display on serial console
    pw::sys_io::WriteBytes(
        std::as_bytes(std::span{recv_buffer, static_cast<size_t>(read)}));
  }
}
}  // namespace

int main() {
  // Allow some time to open serial console, so that no logging is missed.
  int delay = 5;
  while (delay) {
    pw::spin_delay::WaitMillis(1000);
    PW_LOG_INFO("%d...", delay--);
  }
  PW_LOG_INFO("pigweed tls example");
  TlsClientExample();
  return 0;
}
