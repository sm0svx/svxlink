/**
@file    AsyncMsgTest.cpp
@brief   Unit tests for the Async::Msg serialization framework.
@author  Mark Rose
@date    2026-06-06

Async::Msg (with the ASYNC_MSG_MEMBERS macro) serializes structured messages to
a byte stream. These tests round-trip messages containing scalars, strings, and
nested containers (vector, map) and verify the values survive pack/unpack.

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2026 Tobias Blomberg / SM0SVX

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.
\endverbatim
*/

#include <cstdint>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "AsyncMsg.h"

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


// A message exercising scalars, a string, and nested containers.
class TestMsg : public Async::Msg
{
  public:
    uint32_t                 u32 = 0;
    int16_t                  i16 = 0;
    string                   str;
    vector<int>              vec;
    map<string, int>         mp;

    ASYNC_MSG_MEMBERS(u32, i16, str, vec, mp)
};

template <typename T>
bool round_trip(const T& out, T& in)
{
  stringstream ss;
  return out.pack(ss) && in.unpack(ss);
}


void test_scalars_and_string(void)
{
  cout << "test_scalars_and_string" << endl;
  TestMsg out;
  out.u32 = 0xDEADBEEF;
  out.i16 = -1234;
  out.str = "hello, reflector";
  TestMsg in;
  check(round_trip(out, in), "message round-trips");
  check(in.u32 == 0xDEADBEEF, "uint32 preserved");
  check(in.i16 == -1234, "negative int16 preserved");
  check(in.str == "hello, reflector", "string preserved");
}

void test_vector(void)
{
  cout << "test_vector" << endl;
  TestMsg out;
  out.vec = {1, 2, 3, -7, 1000000};
  TestMsg in;
  check(round_trip(out, in), "message with vector round-trips");
  check(in.vec == out.vec, "vector<int> contents preserved");
}

void test_map(void)
{
  cout << "test_map" << endl;
  TestMsg out;
  out.mp = {{"a", 1}, {"b", 2}, {"c", 3}};
  TestMsg in;
  check(round_trip(out, in), "message with map round-trips");
  check(in.mp == out.mp, "map<string,int> contents preserved");
}

void test_empty_containers(void)
{
  cout << "test_empty_containers" << endl;
  TestMsg out;             // all containers empty, strings empty
  TestMsg in;
  in.vec = {9, 9};         // stale data that must be overwritten
  in.str = "stale";
  check(round_trip(out, in), "empty message round-trips");
  check(in.vec.empty() && in.str.empty() && in.mp.empty(),
        "empty containers/string overwrite stale data");
}

void test_truncated_stream_fails(void)
{
  cout << "test_truncated_stream_fails" << endl;
  TestMsg out;
  out.str = "abcdef";
  out.vec = {1, 2, 3};
  stringstream ss;
  out.pack(ss);
  string full = ss.str();
  // Feed only the first few bytes; unpack must fail rather than succeed.
  stringstream truncated(full.substr(0, full.size() / 3));
  TestMsg in;
  check(!in.unpack(truncated), "unpack of a truncated stream fails");
}

} /* anonymous namespace */


int main(void)
{
  test_scalars_and_string();
  test_vector();
  test_map();
  test_empty_containers();
  test_truncated_stream_fails();

  cout << endl;
  if (failures == 0)
  {
    cout << "All Async::Msg tests passed" << endl;
    return 0;
  }
  cout << failures << " Async::Msg test check(s) FAILED" << endl;
  return 1;
}
