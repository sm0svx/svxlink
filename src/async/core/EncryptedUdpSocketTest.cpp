/**
@file    EncryptedUdpSocketTest.cpp
@brief   Unit test for UDP cipher key length handling in EncryptedUdpSocket
@author  Mark Rose
@date    2026-10-16

The cipher reads as many key bytes as the cipher requires, whatever the
size of the stored key, so EncryptedUdpSocket must never use a key of the
wrong length. This checks that setCipherKey() rejects such a key without
storing it, and that write() refuses to send without a valid key.

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2003-2026 Tobias Blomberg / SM0SVX

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.
\endverbatim
*/

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include <AsyncCppApplication.h>
#include <AsyncEncryptedUdpSocket.h>
#include <AsyncIpAddress.h>

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

} /* anonymous namespace */


int main(void)
{
  CppApplication app;

  EncryptedUdpSocket sock;
  check(sock.initOk(), "socket initializes");
  check(sock.setCipher("AES-128-GCM"), "AES-128-GCM cipher can be set");
  check(sock.setCipherIV(vector<uint8_t>(12, 1)), "IV can be set");

  const char data[] = "hello";
  const IpAddress localhost("127.0.0.1");
  const int port = 7;

  check(!sock.setCipherKey(vector<uint8_t>(5, 2)),
        "a cipher key of the wrong length is rejected");
  check(sock.cipherKey().empty(),
        "a rejected cipher key is not stored");
  check(!sock.write(localhost, port, data, sizeof(data)),
        "writing without a valid cipher key fails");

  check(!sock.setCipherKey(vector<uint8_t>()),
        "an empty cipher key is rejected");

  check(sock.setCipherKey(vector<uint8_t>(16, 3)),
        "a cipher key of the right length is accepted");
  check(sock.cipherKey().size() == 16, "the accepted cipher key is stored");
  check(sock.write(localhost, port, data, sizeof(data)),
        "writing with a valid cipher key succeeds");

  cout << (failures == 0 ? "ALL TESTS PASSED" : "TESTS FAILED") << endl;
  return failures != 0;
} /* main */
