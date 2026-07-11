/**
@file    MsgSetMuteStateTest.cpp
@brief   Unit test for mute state validation in NetTrxMsg::MsgSetMuteState
@author  Mark Rose
@date    2026-10-16

The mute state in a MsgSetMuteState message comes from the network, so it
may hold any 32-bit value. This checks that isValid() accepts exactly the
Rx::MuteState values and rejects everything else, for messages built the
same way as a received one (raw bytes reinterpreted as the message).

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2026 Tobias Blomberg / SM0SVX

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.
\endverbatim
*/

#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>

#include "NetTrxMsg.h"

using namespace std;
using namespace NetTrxMsg;

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

  // Build a message as received from the network: the bytes of a valid
  // message with the mute state field overwritten by the given raw value.
bool isValidRaw(int32_t raw)
{
  MsgSetMuteState msg(Rx::MUTE_NONE);
  alignas(MsgSetMuteState) unsigned char buf[sizeof(MsgSetMuteState)];
  memcpy(buf, &msg, sizeof(buf));
  memcpy(buf + sizeof(buf) - sizeof(raw), &raw, sizeof(raw));
  return reinterpret_cast<const MsgSetMuteState*>(buf)->isValid();
}

} /* anonymous namespace */


int main(void)
{
  check(MsgSetMuteState(Rx::MUTE_NONE).isValid(), "MUTE_NONE is valid");
  check(MsgSetMuteState(Rx::MUTE_CONTENT).isValid(), "MUTE_CONTENT is valid");
  check(MsgSetMuteState(Rx::MUTE_ALL).isValid(), "MUTE_ALL is valid");
  check(MsgSetMuteState(Rx::MUTE_ALL).muteState() == Rx::MUTE_ALL,
        "the mute state is returned unchanged");

  check(isValidRaw(Rx::MUTE_CONTENT), "a received valid value is valid");
  check(!isValidRaw(Rx::MUTE_ALL + 1), "one past MUTE_ALL is invalid");
  check(!isValidRaw(-1), "a negative value is invalid");
  check(!isValidRaw(0x7fffffff), "a huge value is invalid");

  cout << (failures == 0 ? "ALL TESTS PASSED" : "TESTS FAILED") << endl;
  return failures != 0;
} /* main */
