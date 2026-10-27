/**
@file    TcpServerPortTest.cpp
@brief   Unit tests for Async::TcpServerBase::parsePort range validation.
@author  Mark Rose
@date    2026-06-10

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

#include "AsyncTcpServerBase.h"

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
  cout << "test_parse_port" << endl;

  // Valid in-range numeric ports are accepted as-is.
  check(TcpServerBase::parsePort("5300") == 5300, "valid port 5300");
  check(TcpServerBase::parsePort("1") == 1, "minimum port 1");
  check(TcpServerBase::parsePort("65535") == 65535, "maximum port 65535");

  // The bug: out-of-range values must be rejected, not truncated mod 65536.
  // Pre-fix, strtol("70000") narrowed to uint16_t == 4464 and silently bound
  // the wrong port. parsePort must return -1 instead.
  check(TcpServerBase::parsePort("70000") == -1, "70000 rejected (no 16-bit wrap)");
  check(TcpServerBase::parsePort("65536") == -1, "65536 rejected (one past max)");
  check(TcpServerBase::parsePort("4294967296") == -1, "huge value rejected");

  // Out-of-range low / invalid values.
  check(TcpServerBase::parsePort("0") == -1, "port 0 rejected");
  check(TcpServerBase::parsePort("-1") == -1, "negative rejected");

  // Non-numeric strings return -1 so the caller falls back to a service
  // name lookup (e.g. getservbyname).
  check(TcpServerBase::parsePort("http") == -1, "service name -> -1 (fallback)");
  check(TcpServerBase::parsePort("53x") == -1, "trailing junk -> -1");
  check(TcpServerBase::parsePort("") == -1, "empty -> -1");

  if (failures == 0)
  {
    cout << "All TcpServerBase::parsePort tests passed" << endl;
    return 0;
  }
  cout << failures << " parsePort test check(s) FAILED" << endl;
  return 1;
} /* main */
