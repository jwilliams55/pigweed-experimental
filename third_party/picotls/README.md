# Picotls Library

The folder hosts picotls library. A build script is provided and currently
defines a library target `picotls_lib_baremetal` for use in other modules. The
target has a dependency on `//third_party/boringssl:crypto_lib_baremetal` for
cryptography algorithms.

The library has a bug in lib/openssl.c that incorrectly sets the expected
purpose attribute of root CA certificates for verification. A patch
`fix_cert_purpose_patch.diff` is provided to fix the problem. Enter source
folder `src` and run `git apply ../fix_cert_purpose_patch.diff` to fix the bug.

About the bug, as client, the library expects the provided root CA to have
client authentication extended usage. But it should in fact be expecting server
authentication extended usage instead. Most CA certificates offer both. But
some CA certificates are more specific and only offer server authenticattion
usage, i.e. GTS CA 101 used by www.google.com:443. This causes certificate
verification to fail.
