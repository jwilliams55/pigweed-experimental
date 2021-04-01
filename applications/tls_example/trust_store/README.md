# Trust Store for TLS Example Application.

The example application connects to www.google.com:443 server, which sends the
following 2 CA certificates during handhsake:

1) Global Sign R2
2) GTS CA 101 (subordinate CA)

The certificates and the corresponding crls can be downloaded from
https://pki.goog/repository/
