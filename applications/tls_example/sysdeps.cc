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

extern "C" {
// libraries such as boringssl, picotls, wolfssl use time() to get current
// date/time for certificate time check. For demo purpose, the following fakes
// this function and provides a pre-set date.
//
// TLS_EXAMPLE_TIME specifies the current time in seconds since epoch. One easy
// way to figure out this value for a certain date is to use a one-line python
// code:
//
// import datetime
// datetime.datetime(2021,5,21,0,0).timestamp()
//
// The above gives the seconds since epoch for 05/21/2021 00:00
#ifndef TLS_EXAMPLE_TIME
#define TLS_EXAMPLE_TIME 1621580400  // 2021-05-21 00:00:00
#endif

time_t time(time_t* timer) {
  time_t ret = TLS_EXAMPLE_TIME;
  return timer ? *timer = ret : ret;
}

// boringssl reads from file "dev/urandom" for generating randome bytes.
// For demo purpose, we fake these file io functions.

int open(const char* file, int, ...) {
  if (strcmp(file, "dev/urandom") == 0) {
    return 1;
  }
  return -1;
}

int fcntl(int, int, ...) { return 0; }

extern "C" ssize_t read(int, void*, size_t len) {
  return static_cast<ssize_t>(len);
}

int close(int) { return 0; }

int _stat(const char*, struct _stat*) { return 0; }

int _open(const char* file, int, int) {
  if (strcmp(file, "dev/urandom") == 0) {
    return 1;
  }
  return -1;
}

int _gettimeofday(struct timeval* tp, void*) {
  tp->tv_sec = 0;
  tp->tv_usec = 0;
  return 0;
}
}
