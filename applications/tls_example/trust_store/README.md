# Trust Store for TLS Example Application.

The example application connects to www.google.com:443 server, which sends the
following 2 CA certificates during handhsake:

1) Global Sign R2
2) GTS CA 101 (subordinate CA)

The certificates and the corresponding crls can be downloaded from
https://pki.goog/repository/

generate_cert_crl_header_file.py is a reference python script for converting
downloaded PEM format certificate/CRL files into C macros in a C header file.
The macro can be used in embedded code for loading root CAs and CRLs to tls
libraries.
