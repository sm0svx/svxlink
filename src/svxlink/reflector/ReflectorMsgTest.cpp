/**
@file    ReflectorMsgTest.cpp
@brief   Round-trip (pack/unpack) tests for Reflector protocol messages.
@author  Mark Rose
@date    2026-06-06

Reflector messages serialize via the Async::Msg framework. These tests pack a
message to a stream, unpack it into a fresh instance, and verify the message
type and field values survive the round-trip.

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

#include "ReflectorMsg.h"

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

// Pack `out` into a stream and unpack it back into `in`. Returns false if
// either step fails.
template <typename T>
bool round_trip(const T& out, T& in)
{
  stringstream ss;
  if (!out.pack(ss))
  {
    return false;
  }
  return in.unpack(ss);
}


void test_proto_ver(void)
{
  cout << "test_proto_ver" << endl;
  MsgProtoVer out(3, 1);
  MsgProtoVer in(0, 0);
  check(round_trip(out, in), "MsgProtoVer packs and unpacks");
  check(in.type() == MsgProtoVer::TYPE, "type preserved (5)");
  check(in.majorVer() == 3 && in.minorVer() == 1,
        "major/minor preserved (3.1)");
}

void test_heartbeat_no_members(void)
{
  cout << "test_heartbeat_no_members" << endl;
  MsgHeartbeat out;
  MsgHeartbeat in;
  check(round_trip(out, in), "MsgHeartbeat (no members) round-trips");
  check(in.type() == MsgHeartbeat::TYPE, "type preserved (1)");
}

void test_error_string(void)
{
  cout << "test_error_string" << endl;
  MsgError out("something failed");
  MsgError in;
  check(round_trip(out, in), "MsgError packs and unpacks");
  check(in.message() == "something failed", "string field preserved");
  check(in.type() == MsgError::TYPE, "type preserved (13)");
}

void test_node_list_vector(void)
{
  cout << "test_node_list_vector" << endl;
  vector<string> nodes = {"SK3AB", "SM0SVX", "VE6HM"};
  MsgNodeList out(nodes);
  MsgNodeList in;
  check(round_trip(out, in), "MsgNodeList packs and unpacks");
  check(in.nodes() == nodes, "vector<string> field preserved");
  check(in.type() == MsgNodeList::TYPE, "type preserved (101)");
}

void test_empty_node_list(void)
{
  cout << "test_empty_node_list" << endl;
  MsgNodeList out;   // empty
  MsgNodeList in(vector<string>{"stale"});
  check(round_trip(out, in), "empty MsgNodeList round-trips");
  check(in.nodes().empty(), "empty vector preserved (overwrites previous)");
}

} /* anonymous namespace */


int main(void)
{
  test_proto_ver();
  test_heartbeat_no_members();
  test_error_string();
  test_node_list_vector();
  test_empty_node_list();

  cout << endl;
  if (failures == 0)
  {
    cout << "All ReflectorMsg tests passed" << endl;
    return 0;
  }
  cout << failures << " ReflectorMsg test check(s) FAILED" << endl;
  return 1;
}
