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

// The following fakes certain system APIs to get the example working.
// In practice, product needs to provide a real implementation.

#include <string.h>
#include <sys/time.h>

#include "pw_log/log.h"

extern "C" {
// boringssl reads from file "dev/urandom" for generating randome bytes.
// For demo purpose, we fake these file io functions.

int open(const char* file, int, ...) {
  if (strcmp(file, "/dev/urandom") == 0) {
    return 1;
  }
  return -1;
}

int fcntl(int, int, ...) { return 0; }

ssize_t read(int, void*, size_t len) {
  return static_cast<ssize_t>(len);
}

int close(int) { return 0; }

int _stat(const char*, struct _stat*) { return 0; }

int _open(const char* file, int, int) {
  if (strcmp(file, "/dev/urandom") == 0) {
    return 1;
  }
  return -1;
}

int _gettimeofday(struct timeval* tp, void*) {
  tp->tv_sec = 0;
  tp->tv_usec = 0;
  return 0;
}

void perror(const char* __s) { PW_LOG_INFO("%s", __s); }

}
