/**
@file    ModulationTest.cpp
@brief   Unit tests for Modulation::fromString / toString.
@author  Mark Rose
@date    2026-06-06

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

#include "Modulation.h"

using namespace std;

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

const Modulation::Type ALL[] = {
  Modulation::MOD_FM, Modulation::MOD_NBFM, Modulation::MOD_WBFM,
  Modulation::MOD_AM, Modulation::MOD_NBAM, Modulation::MOD_USB,
  Modulation::MOD_LSB, Modulation::MOD_CW, Modulation::MOD_WBCW
};


void test_known_strings(void)
{
  cout << "test_known_strings" << endl;
  check(Modulation::fromString("FM") == Modulation::MOD_FM, "FM");
  check(Modulation::fromString("NBFM") == Modulation::MOD_NBFM, "NBFM");
  check(Modulation::fromString("WBFM") == Modulation::MOD_WBFM, "WBFM");
  check(Modulation::fromString("AM") == Modulation::MOD_AM, "AM");
  check(Modulation::fromString("USB") == Modulation::MOD_USB, "USB");
  check(Modulation::fromString("LSB") == Modulation::MOD_LSB, "LSB");
  check(Modulation::fromString("CW") == Modulation::MOD_CW, "CW");
}

void test_unknown_string(void)
{
  cout << "test_unknown_string" << endl;
  check(Modulation::fromString("BOGUS") == Modulation::MOD_UNKNOWN,
        "unknown string -> MOD_UNKNOWN");
  check(Modulation::fromString("") == Modulation::MOD_UNKNOWN,
        "empty string -> MOD_UNKNOWN");
  check(Modulation::fromString("fm") == Modulation::MOD_UNKNOWN,
        "lowercase is not matched (case sensitive)");
}

void test_to_string(void)
{
  cout << "test_to_string" << endl;
  check(string(Modulation::toString(Modulation::MOD_FM)) == "FM", "MOD_FM");
  check(string(Modulation::toString(Modulation::MOD_WBCW)) == "WBCW", "MOD_WBCW");
  check(string(Modulation::toString(Modulation::MOD_UNKNOWN)) == "UNKNOWN",
        "MOD_UNKNOWN -> UNKNOWN");
}

void test_round_trip(void)
{
  cout << "test_round_trip" << endl;
  bool all_ok = true;
  for (Modulation::Type m : ALL)
  {
    if (Modulation::fromString(Modulation::toString(m)) != m)
    {
      all_ok = false;
    }
  }
  check(all_ok, "fromString(toString(m)) == m for every modulation type");
}

} /* anonymous namespace */


int main(void)
{
  test_known_strings();
  test_unknown_string();
  test_to_string();
  test_round_trip();

  cout << endl;
  if (failures == 0)
  {
    cout << "All Modulation tests passed" << endl;
    return 0;
  }
  cout << failures << " Modulation test check(s) FAILED" << endl;
  return 1;
}
