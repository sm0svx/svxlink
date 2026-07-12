/**
@file    SerialDeviceTest.cpp
@brief   Unit test for SerialDevice::open dev_map cleanup on failure
@author  Mark Rose
@date    2026-07-11

SerialDevice::open() keeps a static map, dev_map, from port name to the
(reference-counted) SerialDevice for that port. When a brand new
SerialDevice's first openPort() call fails, the object is deleted, but
before the fix nothing removed its entry from dev_map, leaving a dangling
pointer behind. A later open() call for the same port name would then find
that stale entry and reuse the dangling pointer instead of allocating and
opening a fresh SerialDevice -- incorrectly returning a non-null "success"
result for a port that still cannot be opened.

This test opens a port name that can never exist twice in a row, using no
real hardware and no Async event loop: SerialDevice::openPort() fails via a
plain ::open() syscall failure before it ever touches a FdWatch/event loop.
Both calls must fail (return 0); a second non-null return indicates the
dangling dev_map entry was reused.

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

#include "AsyncSerialDevice.h"

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
    // A path that is syntactically valid but is guaranteed not to exist, so
    // that SerialDevice::openPort() fails on a plain ::open() syscall.
  const string port = "/nonexistent/svxlink-serialdevice-devmap-test-port";

  SerialDevice *dev1 = SerialDevice::open(port, false);
  check(dev1 == 0, "first open() of a nonexistent port fails");

    // Before the fix, the failed first open() above deleted the SerialDevice
    // object without erasing it from the private static dev_map, leaving a
    // dangling pointer keyed on `port`. This second call for the very same
    // port must not find (and reuse) that stale entry -- it must allocate a
    // fresh SerialDevice, attempt to open it, fail again, and return 0.
  SerialDevice *dev2 = SerialDevice::open(port, false);
  check(dev2 == 0,
        "second open() of the same nonexistent port also fails "
        "(no dangling dev_map entry was reused)");

    // Defensive: never touch a pointer that check() above already flagged
    // as unexpectedly non-null, since it may be dangling.
  if (dev1 != 0)
  {
    SerialDevice::close(dev1);
  }
  if (dev2 != 0 && dev2 != dev1)
  {
    SerialDevice::close(dev2);
  }

  cout << (failures == 0 ? "ALL TESTS PASSED" : "TESTS FAILED") << endl;
  return failures != 0;
} /* main */
