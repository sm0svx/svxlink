/**
@file    SslWrapperTest.cpp
@brief   Unit tests for robustness fixes in the Async OpenSSL wrappers
@author  Mark Rose
@date    2026-10-16

Covers PEM data longer than 16 KiB, ownership of certificates read into a
non-owning SslX509, CSR move assignment, copying empty X.509 extensions,
and the behaviour of the certificate getters for a null certificate. Run it
under AddressSanitizer to also catch the leaks the ownership fixes remove.

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2003-2026 Tobias Blomberg / SM0SVX

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.
\endverbatim
*/

#include <iostream>
#include <sstream>
#include <string>
#include <utility>

#include <AsyncSslCertSigningReq.h>
#include <AsyncSslKeypair.h>
#include <AsyncSslX509.h>
#include <AsyncSslX509Extensions.h>

using namespace std;
using namespace Async;

namespace {

int failures = 0;

void check(bool cond, const string& msg)
{
  cout << (cond ? "  ok   " : "  FAIL ") << msg << endl;
  if (!cond)
  {
    ++failures;
  }
}

  // A self-signed certificate with enough subject alternative names to make
  // its PEM encoding longer than 16 KiB
SslX509 bigCertificate(SslKeypair& key)
{
  SslX509 cert;
  cert.setVersion(SslX509::VERSION_3);
  cert.setSerialNumber();
  cert.addSubjectName("CN", "test.example.com");
  cert.setIssuerName(cert.subjectName());
  std::ostringstream san;
  for (int i = 0; i < 600; ++i)
  {
    san << (i > 0 ? "," : "") << "DNS:host" << i << ".example.com";
  }
  SslX509Extensions exts;
  exts.addSubjectAltNames(san.str());
  cert.addExtensions(exts);
  cert.setValidityTime(1);
  cert.setPublicKey(key);
  cert.sign(key);
  return cert;
}

} /* anonymous namespace */


int main(void)
{
  SslKeypair key;
  check(key.generate(2048), "a key pair can be generated");

    // PEM data longer than 16 KiB must not be truncated
  SslX509 cert = bigCertificate(key);
  const std::string pem = cert.pem();
  check(pem.size() > 16384, "the test certificate PEM is longer than 16 KiB");
  SslX509 copy;
  check(copy.readPem(pem), "the long PEM can be read back");
  check(copy.pem() == pem, "the long PEM round-trips unchanged");

    // A certificate read into a non-owning SslX509 is owned by it, so it is
    // freed with the object (otherwise the leak checker reports it)
  X509* raw = X509_new();
  {
    SslX509 view(raw, false);
    check(view.readPem(pem), "a PEM can be read into a non-owning SslX509");
  }
  X509_free(raw);

    // Move assigning a CSR frees the request it held (otherwise the leak
    // checker reports it) and self move assignment keeps the request
  {
    SslCertSigningReq a;
    SslCertSigningReq b;
    a = std::move(b);
    check(!a.isNull(), "a move assigned CSR holds the moved request");
    SslCertSigningReq& alias = a;
    a = std::move(alias);
    check(!a.isNull(), "self move assignment keeps the request");
  }

    // Extensions added to a copy of an empty extension set are kept
  {
    SslX509Extensions empty;
    SslX509Extensions exts(empty);
    check(exts.addKeyUsage("digitalSignature"),
          "an extension can be added to a copy of an empty set");
    check(sk_X509_EXTENSION_num(exts) == 1,
          "the added extension is in the set");
  }

    // A null certificate must not be dereferenced
  {
    SslX509 null_cert(nullptr);
    check(null_cert.isNull(), "the certificate is null");
    check(null_cert.notBeforeString().empty() &&
          null_cert.notAfterString().empty(),
          "the time strings of a null certificate are empty");
    check(!null_cert.matchHost("test.example.com"),
          "a null certificate does not match a host name");
    check(!null_cert.matchIp(IpAddress("127.0.0.1")),
          "a null certificate does not match an IP address");
  }

  cout << (failures == 0 ? "ALL TESTS PASSED" : "TESTS FAILED") << endl;
  return failures != 0;
} /* main */
