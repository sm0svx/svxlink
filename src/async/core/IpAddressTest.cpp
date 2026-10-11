/**
@file    IpAddressTest.cpp
@brief   Unit test for IpAddress::isWithinSubet() netmask prefix validation
@author  Mark Rose
@date    2026-07-11

IpAddress::isWithinSubet() parses a "<ip>/<prefix>" subnet specification. The
prefix used to be turned into a mask via atoi() and pow(2.0, 32-prefix) with
no validation. A non-numeric or out-of-range prefix (e.g. "/x", "/33", "/-1")
made that computation collapse to a mask of 0, which matches every address --
silently turning an ALLOW_IP/subnet check into an "allow all" rule. This test
verifies that a malformed prefix is now rejected (isWithinSubet() returns
false) instead of failing open, while valid prefixes still behave correctly.

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2026 Tobias Blomberg / SM0SVX

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.
\endverbatim
*/

#include <iostream>
#include <string>

#include "AsyncIpAddress.h"

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
  IpAddress addr("192.168.1.50");

    // A malformed (non-numeric) prefix must not fail open and match
    // every address.
  check(!addr.isWithinSubet("10.0.0.0/x"),
        "non-numeric prefix does not match (fails closed)");

    // Out of range prefixes must likewise be rejected.
  check(!addr.isWithinSubet("192.168.1.0/33"),
        "prefix > 32 does not match (fails closed)");
  check(!addr.isWithinSubet("192.168.1.0/-1"),
        "negative prefix does not match (fails closed)");

    // Trailing garbage after a numeric prefix must be rejected too.
  check(!addr.isWithinSubet("192.168.1.0/24abc"),
        "prefix with trailing garbage does not match (fails closed)");

    // A completely unrelated network with a malformed prefix must not
    // match either -- this is the "allow all" failure mode of the bug.
  check(!addr.isWithinSubet("8.8.8.8/x"),
        "unrelated network with bad prefix does not match");

    // Sanity: valid prefixes still work correctly, both inside and
    // outside the subnet.
  check(addr.isWithinSubet("192.168.1.0/24"),
        "address within valid /24 subnet matches");
  check(!addr.isWithinSubet("192.168.2.0/24"),
        "address outside valid /24 subnet does not match");
  check(addr.isWithinSubet("192.168.1.48/30"),
        "address within valid /30 subnet matches");
  check(addr.isWithinSubet("0.0.0.0/0"),
        "prefix /0 matches everything as expected");

  cout << (failures == 0 ? "ALL TESTS PASSED" : "TESTS FAILED") << endl;
  return failures != 0;
} /* main */
