# Copyright 2021 The Pigweed Authors
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License. You may obtain a copy of
# the License at
#
#     https:#www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations under
# the License.
"""Generate time for injection

The script generates a .c file that implement time_t time(time_t *) c API
that returns a pre-set time. This is used to inject a time for the tls
example application. The template of the code is

#include <sys/time.h>

static time_t injected_time = <time>;
time_t time(time_t* timer) {
  return timer ? *timer = injected_time : injected_time;
}

Usage:
    inject specified time: genearte_time_code <output> -t "03/29/2021 12:00:00"
    inject current time: genearte_time_code <output>
"""
from datetime import datetime
import argparse
import subprocess

TIME_FORMAT = "%m/%d/%Y %H:%M:%S"

HEADER = """// Copyright 2021 The Pigweed Authors
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

#include <sys/time.h>

// libraries such as boringssl, picotls, wolfssl use time() to get current
// date/time for certificate time check. For demo purpose, the following fakes
// this function and provides a pre-set date.

"""

FUNCTION = """
time_t time(time_t* t) {
  return t ? *t = injected_time : injected_time;
}
"""


def parse_args():
    """Setup argparse."""
    parser = argparse.ArgumentParser()
    parser.add_argument("out", help="path for output header file")
    parser.add_argument("--time", "-t", help="time to inject")
    return parser.parse_args()


if __name__ == "__main__":
    args = parse_args()
    if args.time:
        time_stamp = datetime.strptime(args.time, TIME_FORMAT).timestamp()
    else:
        time_stamp = datetime.now().timestamp()
    time_stamp = int(time_stamp)

    # Generate source file
    with open(args.out, "w") as header:
        header.write(HEADER)
        string_date = datetime.fromtimestamp(time_stamp).strftime(TIME_FORMAT)
        header.write(f'// {string_date}\n')
        header.write(f'static time_t injected_time = {int(time_stamp)};\n')
        header.write(FUNCTION)
        header.write("\n")

    subprocess.run([
        "clang-format",
        "-i",
        args.out,
    ], check=True)
