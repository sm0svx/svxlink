/**
@file    ConfigTest.cpp
@brief   Unit tests for Async::Config value get/set and type coercion.
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

#include <algorithm>
#include <iostream>
#include <list>
#include <string>

#include "AsyncConfig.h"

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


void test_string_round_trip(void)
{
  cout << "test_string_round_trip" << endl;
  Config cfg;
  cfg.setValue("S", "name", string("hello"));
  string v;
  check(cfg.getValue("S", "name", v) && v == "hello", "string get/set");
}

void test_typed_coercion(void)
{
  cout << "test_typed_coercion" << endl;
  Config cfg;
  cfg.setValue("S", "num", string("42"));
  cfg.setValue("S", "flt", string("3.5"));

  int i = 0;
  check(cfg.getValue("S", "num", i) && i == 42, "int coercion '42' -> 42");
  float f = 0.0f;
  check(cfg.getValue("S", "flt", f) && f == 3.5f, "float coercion '3.5' -> 3.5");

  cfg.setValue("S", "notnum", string("abc"));
  int bad = -1;
  check(!cfg.getValue("S", "notnum", bad),
        "non-numeric value fails int coercion");
}

void test_missing_and_default(void)
{
  cout << "test_missing_and_default" << endl;
  Config cfg;
  int i = 99;
  check(!cfg.getValue("S", "absent", i), "missing variable returns false");
  check(i == 99, "value untouched when missing");

  int j = 7;
  check(cfg.getValue("S", "absent", j, /*missing_ok=*/true),
        "missing_ok=true returns true for missing variable");
  check(j == 7, "value untouched with missing_ok");
}

void test_list_section(void)
{
  cout << "test_list_section" << endl;
  Config cfg;
  cfg.setValue("Sec", "a", string("1"));
  cfg.setValue("Sec", "b", string("2"));
  cfg.setValue("Other", "c", string("3"));

  list<string> tags = cfg.listSection("Sec");
  bool has_a = find(tags.begin(), tags.end(), "a") != tags.end();
  bool has_b = find(tags.begin(), tags.end(), "b") != tags.end();
  check(tags.size() == 2 && has_a && has_b,
        "listSection returns the tags of that section only");

  list<string> none = cfg.listSection("Nonexistent");
  check(none.empty(), "listSection of unknown section is empty");
}

} /* anonymous namespace */


int main(void)
{
  test_string_round_trip();
  test_typed_coercion();
  test_missing_and_default();
  test_list_section();

  cout << endl;
  if (failures == 0)
  {
    cout << "All Config tests passed" << endl;
    return 0;
  }
  cout << failures << " Config test check(s) FAILED" << endl;
  return 1;
}
