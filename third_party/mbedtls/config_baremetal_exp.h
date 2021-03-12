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

// See third_party/mbedtls/src/configs/config-psa-crypto.h for a detail
// explanation of these configurations.

// No file system support.
#undef MBEDTLS_FS_IO
// No posix socket support
#undef MBEDTLS_NET_C
// This feature requires file system support.
#undef MBEDTLS_PSA_ITS_FILE_C
// The following two require MBEDTLS_PSA_ITS_FILE_C
#undef MBEDTLS_PSA_CRYPTO_C
#undef MBEDTLS_PSA_CRYPTO_STORAGE_C
// This feature only works on Unix/Windows
#undef MBEDTLS_TIMING_C
// Use a custom entropy generator
#define MBEDTLS_NO_PLATFORM_ENTROPY
