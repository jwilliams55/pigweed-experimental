# Copyright 2021 The Pigweed Authors
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License. You may obtain a copy of
# the License at
#
#     https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations under
# the License.

import os

"""Generate sources.gni for the list of mbedtls source files"""

LICENSE_HEADER = """# Copyright 2021 The Pigweed Authors
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License. You may obtain a copy of
# the License at
#
#     https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations under
# the License.

"""

if __name__ == "__main__":

    cfiles = []
    for (path, dirnames, filenames) in os.walk("src/library"):
        for filename in filenames:
            if not filename.endswith('.c'):
                continue
            cfiles.append(os.path.join(path, filename))
    cfiles.sort()

    with open("sources.gni", 'w') as f:
        f.write(LICENSE_HEADER)
        f.write("mbedtls_sources = [\n")
        for source in cfiles:
            f.write(f'  \"{source}\",\n')
        f.write("]\n")
