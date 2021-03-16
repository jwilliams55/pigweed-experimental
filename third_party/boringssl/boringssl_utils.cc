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

#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/pem.h>

int LoadCACertCrls(const void* buffer, size_t size, X509_STORE *store) {
  BIO * ca_crt_bio = BIO_new_mem_buf(buffer, size);
  STACK_OF(X509_INFO) *inf = PEM_X509_INFO_read_bio(ca_crt_bio, NULL, NULL, NULL);
  BIO_free(ca_crt_bio);
  if (!inf) {
    return -1;
  }
  for (size_t i = 0; i < sk_X509_INFO_num(inf); i++) {
      X509_INFO *itmp = sk_X509_INFO_value(inf, i);
      if (itmp->x509 && !X509_STORE_add_cert(store, itmp->x509)) {
          return -1;
      }
      if (itmp->crl && !X509_STORE_add_crl(store, itmp->crl)) {
          return -1;
      }
  }
  sk_X509_INFO_pop_free(inf, X509_INFO_free);
  return 0;
}