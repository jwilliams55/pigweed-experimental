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

#include <openssl/pem.h>

/**
 * Load PEM format certificates/crls from a buffer
 *
 * @buffer: buffer containing the certificates/crls
 * @size: size of |buffer|
 * @store: Pointer of a X509_STORE object for storing the loaded certificates/crls.
 *
 * Returns 0 on successs, other values on failure.
 */
int LoadCACertCrls(const void* buffer, size_t size, X509_STORE *store);
