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
"""The script generates a header file containing C macros of GlobalSign and
GTA CA 101 root certificates and their CRLs"""

import os


def generate_pem_macro(file, macro_name):
    """Generates a C macro declaration for a PEM file"""
    ret = []
    with open(file, "r") as f:
        start = []
        end = []
        begin = False
        exit = False
        for l in f:
            l = l.strip('\r\n')
            if l.startswith('-----BEGIN'):
                begin = True
                start = l
            elif l.startswith('-----END'):
                begin = False
                end = l
            elif begin:
                ret.append(l)
        ret.insert(0, start)
        ret.append(end)

    s = ""
    for ele in ret:
        s += f'\"{ele}\\r\\n\" \\\r\n'
    return f'#define {macro_name} \\\r\n {s}'


if __name__ == "__main__":
    with open("ca_certificates_crls.h", "w") as f:
        f.write("""// Copyright 2021 The Pigweed Authors
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

""")
        f.write("#ifndef CA_CERTIFICATE_CRLS_H\r\n")
        f.write("#define CA_CERTIFICATE_CRLS_H\r\n")
        f.write("\n")

        f.write(generate_pem_macro("global_sign_r2.pem", "GLOBAL_SIGN_CA_CRT"))
        f.write("\n")

        f.write(
            generate_pem_macro("global_sign_r2.crl.pem", "GLOBAL_SIGN_CA_CRL"))
        f.write("\n")

        f.write(generate_pem_macro("gts_ca_101.pem", "GTS_CA_101_CRT"))
        f.write("\n")

        f.write(generate_pem_macro("gts_ca_101.crl.pem", "GTS_CA_101_CRL"))
        f.write("\n")

        f.write("#endif // CA_CERTIFICATE_CRLS_H")
        f.write("\n")
