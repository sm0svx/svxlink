/**
@file    CommonTest.cpp
@brief   Unit tests for the SvxLink common string utilities (common.h).
@author  Mark Rose
@date    2026-06-06

Covers setValueFromString (typed parse with full-consume semantics), splitStr
(tokenising with delimiter sets, skipping tokens that don't parse) and SepPair
(separated-pair stream in/out).

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
#include <sstream>
#include <string>
#include <vector>

#include "common.h"

using namespace std;
using namespace SvxLink;

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


void test_set_value_from_string(void)
{
  cout << "test_set_value_from_string" << endl;
  int i = 0;
  check(setValueFromString(i, "42") && i == 42, "parse int '42'");
  check(setValueFromString(i, "42 ") && i == 42, "trailing space consumed");
  check(!setValueFromString(i, "42x"), "trailing junk rejected");
  check(!setValueFromString(i, ""), "empty string rejected for int");

  float f = 0.0f;
  check(setValueFromString(f, "3.5") && f == 3.5f, "parse float '3.5'");

  string s;
  check(setValueFromString(s, "hello world") && s == "hello world",
        "string overload copies verbatim (keeps spaces)");
}

void test_split_str_int(void)
{
  cout << "test_split_str_int" << endl;
  vector<int> v;
  size_t n = splitStr(v, "1,2,3", ",");
  check(n == 3 && v.size() == 3 && v[0] == 1 && v[2] == 3,
        "split '1,2,3' -> {1,2,3}");

  splitStr(v, "1,,2", ",");
  check(v.size() == 2 && v[0] == 1 && v[1] == 2,
        "consecutive delimiters produce no empty tokens");

  splitStr(v, "1,x,3", ",");
  check(v.size() == 2 && v[0] == 1 && v[1] == 3,
        "tokens that don't parse as int are skipped ('x')");
}

void test_split_str_string(void)
{
  cout << "test_split_str_string" << endl;
  vector<string> v;
  size_t n = splitStr(v, "a, b ,c", ", ");
  check(n == 3 && v[0] == "a" && v[1] == "b" && v[2] == "c",
        "split with ', ' delimiters trims spaces -> {a,b,c}");
}

void test_sep_pair(void)
{
  cout << "test_sep_pair" << endl;
  {
    istringstream is("100.5:1000");
    SepPair<float, unsigned> p;
    is >> p;
    check(!is.fail() && p.first == 100.5f && p.second == 1000u,
          "parse '100.5:1000' -> (100.5, 1000)");
  }
  {
    ostringstream os;
    SepPair<float, unsigned> p;
    p.first = 12.0f;
    p.second = 34u;
    os << p;
    check(os.str() == "12:34", "stream out -> '12:34'");
  }
  {
    istringstream is("noseparator");
    SepPair<float, unsigned> p;
    is >> p;
    check(is.fail(), "missing separator sets failbit");
  }
}

} /* anonymous namespace */


int main(void)
{
  test_set_value_from_string();
  test_split_str_int();
  test_split_str_string();
  test_sep_pair();

  cout << endl;
  if (failures == 0)
  {
    cout << "All common.h tests passed" << endl;
    return 0;
  }
  cout << failures << " common.h test check(s) FAILED" << endl;
  return 1;
}
