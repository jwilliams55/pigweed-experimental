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
"""Trusts tore synthesis

The script generates source code for implementing built-in certificates. The
code template is:

namespace {
const unsigned char kRootCACert_0[] = {...};

const unsigned char kRootCACert_1[] = {...};

...

const std::span<const unsigned char> kRootCerts[] = {
    std::span{kRootCACert_0},
    std::span{kRootCACert_1},
};
}  // namespace

std::span<const std::span<const unsigned char>> GetBuiltInRootCert() {
  return std::span{kRootCerts};
}

GetBuiltInRootCert() is the API for retrieving the certificates.

Usage:

synthesis_trust_store <output> -r <cert0> -r <cert1> .....
"""

import argparse
import subprocess
import sys
from OpenSSL import crypto  # type: ignore

LICENSE_HEADER = """// Copyright 2021 The Pigweed Authors
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

"""

ROOT_CERT_API_FUNC = """
std::span<const std::span<const unsigned char>> GetBuiltInRootCert() {
    return std::span{kRootCerts};
}
"""


def parse_args():
    """Setup argparse."""
    parser = argparse.ArgumentParser()
    parser.add_argument("out", help="output header")
    parser.add_argument("--root_cert",
                        "-r",
                        help="root CA certificate",
                        action="append")
    return parser.parse_args()


def generate_c_array_declaration(data, var_name):
    """Generate a C array declaration for a byte data"""
    return "".join([
        f'const unsigned char {var_name}[] = {{',
        " ".join([f'0x{b:x},' for b in data]), "};"
    ])


def load_certificate_file(file):
    """Load certificate file from a PEM/DER format file"""
    with open(file, 'rb') as cert:
        data = cert.read()
        try:
            x509 = crypto.load_certificate(crypto.FILETYPE_PEM, data)
        except crypto.Error:
            x509 = crypto.load_certificate(crypto.FILETYPE_ASN1, data)
        return crypto.dump_certificate(crypto.FILETYPE_ASN1, x509)


def generate_der_c_array_for_cert(file, var_name):
    """Load cert file and return a generated c array declaration."""

    data = load_certificate_file(file)
    return generate_c_array_declaration(data, var_name)


def main():
    """Generate Trust Store Main."""

    args = parse_args()
    print(args)
    with open(args.out, "w") as header:
        header.write(LICENSE_HEADER)
        header.write("#include <span>\n\n")
        certs = args.root_cert if args.root_cert else []
        cert_collection_array = "".join([
            "const ",
            "std::span<const unsigned char>",
            "kRootCerts[] = {",
        ])
        header.write("namespace {\n\n")
        for i, cert in enumerate(certs):
            var_name = f'kRootCACert_{i}'
            # A comment to indicate the file the content comes from.
            header.write("// Generated from\n")
            header.write(f'// {cert}\n')
            header.write(generate_der_c_array_for_cert(cert, var_name))
            header.write("\n\n")
            cert_collection_array += f'std::span{{{var_name}}},'
        cert_collection_array += "};"
        header.write(cert_collection_array)
        header.write("\n\n")
        # namespace {}
        header.write("}\n\n")
        header.write(ROOT_CERT_API_FUNC)
        header.write("\n\n")

    subprocess.run([
        "clang-format",
        "-i",
        args.out,
    ], check=True)


if __name__ == "__main__":
    sys.exit(main())
