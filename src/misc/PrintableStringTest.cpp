/**
@file    PrintableStringTest.cpp
@brief   Unit test for SvxLink::printableString
@author  Mark Rose
@date    2026-10-16

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2026 Tobias Blomberg / SM0SVX

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.
\endverbatim
*/

#include <iostream>
#include <string>

#include "common.h"

using namespace std;
using SvxLink::printableString;

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
  check(printableString("SM0SVX") == "SM0SVX",
        "a normal string is unchanged");
  check(printableString("A\nB\rC\tD") == "A?B?C?D",
        "newline, carriage return and tab are replaced");
  check(printableString("x\x1b[2Jy") == "x?[2Jy",
        "an escape character is replaced");
  check(printableString(string("a\0b", 3)) == "a?b", "NUL is replaced");
  check(printableString("a\x7f" "b") == "a?b", "DEL is replaced");
  check(printableString("\xc3\xa5") == "\xc3\xa5",
        "bytes above 0x7f (e.g. UTF-8) are kept");
  check(printableString(string(1000, 'x')).size() == 256,
        "the string is cut to 256 characters by default");
  check(printableString("abcdef", 3) == "abc",
        "the maximum length can be given");

  cout << (failures == 0 ? "ALL TESTS PASSED" : "TESTS FAILED") << endl;
  return failures != 0;
} /* main */
