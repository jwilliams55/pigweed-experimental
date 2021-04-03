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
#pragma once

#define GLOBAL_SIGN_CA_CRL                                               \
  "-----BEGIN X509 CRL-----\r\n"                                         \
  "MIIDsjCCApoCAQEwDQYJKoZIhvcNAQELBQAwTDEgMB4GA1UECxMXR2xvYmFsU2ln\r\n" \
  "biBSb290IENBIC0gUjIxEzARBgNVBAoTCkdsb2JhbFNpZ24xEzARBgNVBAMTCkds\r\n" \
  "b2JhbFNpZ24XDTIwMTAwMTAwMDAwMFoXDTIxMDQxNTAwMDAwMFowggHnMCoCCwQA\r\n" \
  "AAAAASINPA91Fw0xNDExMjUwMDAwMDBaMAwwCgYDVR0VBAMKAQUwKgILBAAAAAAB\r\n" \
  "Ig08FMUXDTE0MTEyNTAwMDAwMFowDDAKBgNVHRUEAwoBBTAqAgsEAAAAAAEQC4yh\r\n" \
  "GxcNMTQxMTI1MDAwMDAwWjAMMAoGA1UdFQQDCgEFMCoCCwQAAAAAASf792cAFw0x\r\n" \
  "NDExMjUwMDAwMDBaMAwwCgYDVR0VBAMKAQUwKgILBAAAAAABRE7wRk4XDTE2MTAw\r\n" \
  "NzAwMDAwMFowDDAKBgNVHRUEAwoBBTAqAgsEAAAAAAESVq1fshcNMTYxMDA3MDAw\r\n" \
  "MDAwWjAMMAoGA1UdFQQDCgEFMCoCCwQAAAAAAS9O4VtjFw0xNzA0MDcwMDAwMDBa\r\n" \
  "MAwwCgYDVR0VBAMKAQUwKgILBAAAAAABL07hXdQXDTE3MDQwNzAwMDAwMFowDDAK\r\n" \
  "BgNVHRUEAwoBBTAqAgsEAAAAAAFETvBKVRcNMTkwOTMwMDAwMDAwWjAMMAoGA1Ud\r\n" \
  "FQQDCgEFMCsCDDASm/2A0ZPWs+I05BcNMTkwOTMwMDAwMDAwWjAMMAoGA1UdFQQD\r\n" \
  "CgEFMCwCDQHjqTAc/HIGOD+aUx0XDTE5MDkzMDAwMDAwMFowDDAKBgNVHRUEAwoB\r\n" \
  "BaAvMC0wCgYDVR0UBAMCASIwHwYDVR0jBBgwFoAUm+IHV2ccHsBqBt5ZtJot39wZ\r\n" \
  "hi4wDQYJKoZIhvcNAQELBQADggEBACNjTpjVd8K0aImkgYdqr0wqfNxOvdv1E/Zg\r\n" \
  "cxjhGyMb9ol8lzz8W5qK+SuqX9lv0kJoeM4tuYyNlYvuMzVSQoen40KNXgqbsv4f\r\n" \
  "FAZdsFfe7yIBIqpgOxSwGuegShfm5e2tliZeDcV2cS6XRoF+dX9CeHsUuv1IeJRd\r\n" \
  "OO+fPiqPSdyTmRsFcifSbWQq2Qh0xvKrDlH416AyGzOXWSieui09eM1UvMLjzAqP\r\n" \
  "iQksl/HPzJ4BHCxw7b1yOfJWL2s305qyA6pACS2CpkzLSh3ZDSVtSkbKkXgMsmf5\r\n" \
  "7oCW+hJX2X+THdOPhg1dmsudewYTSviUTOSNdEyWo0n5JXJDAWM=\r\n"             \
  "-----END X509 CRL-----\r\n"

#define GTS_CA_101_CRL                                                   \
  "-----BEGIN X509 CRL-----\r\n"                                         \
  "MIIEwzCCA6sCAQEwDQYJKoZIhvcNAQELBQAwQjELMAkGA1UEBhMCVVMxHjAcBgNV\r\n" \
  "BAoTFUdvb2dsZSBUcnVzdCBTZXJ2aWNlczETMBEGA1UEAxMKR1RTIENBIDFPMRcN\r\n" \
  "MjEwMjIyMTg0MjI5WhcNMjEwMzA0MTc0MjI4WjCCAsgwIgIRAPOeVuL3pOWMCAAA\r\n" \
  "AABxCdoXDTIxMDIxNTExMzM0NlowIgIRALPSpDcvsGBZCAAAAABxCd8XDTIxMDIx\r\n" \
  "NTExMzM0N1owIgIRALFzu+oSx0W2CAAAAABxDAMXDTIxMDIxNTE0MTczMFowIQIQ\r\n" \
  "KdqEr0zEct8IAAAAAHEMCBcNMjEwMjE1MTQxNzMxWjAiAhEA+6BtbqQ+8ewIAAAA\r\n" \
  "AHEMyhcNMjEwMjE2MTEzMzQ0WjAiAhEAkfsP0PIrH08IAAAAAHEM0hcNMjEwMjE2\r\n" \
  "MTEzMzQ2WjAhAhAcxc/PVxodqwgAAAAAcQ2sFw0yMTAyMTcxMTMzNDVaMCECEHsF\r\n" \
  "sk/6a6UQCAAAAABxDbQXDTIxMDIxNzExMzM0N1owIgIRAI6CTuDMsL/oCAAAAABx\r\n" \
  "Dn4XDTIxMDIxODExMzM0NVowIgIRAOLZnq2ph1MpCAAAAABxDoQXDTIxMDIxODEx\r\n" \
  "MzM0OFowIgIRANoBw6kMQ3agCAAAAABxDqUXDTIxMDIxODE0NTMyNVowIQIQU0+z\r\n" \
  "IUNAjfcIAAAAAHEOqRcNMjEwMjE4MTQ1MzI3WjAiAhEAoKn9zoskKLwIAAAAAHEP\r\n" \
  "dhcNMjEwMjE5MTQ1MzI4WjAhAhB0V6Mx1YFI7QgAAAAAcQ99Fw0yMTAyMTkxNDUz\r\n" \
  "MjdaMCICEQC+tg2atFn2KQgAAAAAcRA+Fw0yMTAyMjAxNDUzMjVaMCECEFH4Oss7\r\n" \
  "Ux84CAAAAABxEEYXDTIxMDIyMDE0NTMyNlowIQIQZcbYEr1xuBsIAAAAAHERCBcN\r\n" \
  "MjEwMjIxMTQ1MzM3WjAiAhEA31c6SN9C0jsIAAAAAHERCRcNMjEwMjIxMTQ1MzM3\r\n" \
  "WjAhAhABFs/t9ooeaAgAAAAAcRHWFw0yMTAyMjIxNDUzMjZaMCICEQDQqZXLbVbw\r\n" \
  "RQgAAAAAcRHbFw0yMTAyMjIxNDUzMjZaoGkwZzAfBgNVHSMEGDAWgBSY0fhuEOvP\r\n" \
  "m+xgnxiQG6DrfQn9KzALBgNVHRQEBAICBbwwNwYDVR0cAQH/BC0wK6AmoCSGImh0\r\n" \
  "dHA6Ly9jcmwucGtpLmdvb2cvR1RTMU8xY29yZS5jcmyBAf8wDQYJKoZIhvcNAQEL\r\n" \
  "BQADggEBABcDrP2YlEoVf6hgtyztTftJLS+a2FAI5LpbRfqNeyM1wLP8XqmufhV4\r\n" \
  "3/2H2dj9YO0cokpan31ZwJX2NV9YIM8OU4wha5FWxHxBQ1MQ/MhIFjmboQ36D0rl\r\n" \
  "jYZOuc4F8uRwU+5/+e62t0+ZO7e8oz+1uVYDcOXCgQgJkr4aSKJuGrhYTiV9iRpI\r\n" \
  "Wjf3hZECG4bOfjrAchses+xxAd2WDb09cUa8qeqFqFqLRHSiqJBxqCzb9kDEayBI\r\n" \
  "XYvkJ/XPJFoSVLvzZpL+UjDDtY+YCkWU3m6QJ9Mamhy8bqtU9bNyoV7JTOa+V4cd\r\n" \
  "XA7cpwQ3pwG2LJTQjLG3+Lvk0HECG14=\r\n"                                 \
  "-----END X509 CRL-----\r\n"

std::span<const std::span<const unsigned char>> GetBuiltInRootCert();
