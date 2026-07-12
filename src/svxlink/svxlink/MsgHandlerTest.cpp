/**
@file    MsgHandlerTest.cpp
@brief   Unit test for MsgHandler extensionless-path handling
@author  Mark Rose
@date    2026-07-10

MsgHandler::playFile() classifies a clip by its file extension. For a path with
no '.', strrchr() returns NULL, which was passed straight into strcmp(),
dereferencing NULL and crashing. This test drives playFile() with an
extensionless path and verifies it no longer crashes (it falls through to the
raw-file handler).

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

#include <AsyncAudioSink.h>

#include "MsgHandler.h"

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

  // Minimal sink so MsgHandler has somewhere to flush to (playback of a
  // non-existent file ends in a flush).
class NullSink : public AudioSink
{
  public:
    virtual int writeSamples(const float*, int count) { return count; }
    virtual void flushSamples(void) { sourceAllSamplesFlushed(); }
};

} /* anonymous namespace */


int main(void)
{
  MsgHandler h(16000);
  NullSink sink;
  h.registerSink(&sink);

    // Extensionless path: strrchr(path, '.') == NULL. Before the fix this
    // dereferenced NULL in strcmp(); now it falls through to the raw handler.
  h.playFile("/tmp/svxlink_msghandler_test_no_extension", false);
  check(true, "playFile on an extensionless path did not crash");

    // Extensioned paths still classify without issue.
  h.playFile("/tmp/svxlink_msghandler_test.wav", false);
  h.playFile("/tmp/svxlink_msghandler_test.gsm", false);
  check(true, "playFile on .wav / .gsm paths did not crash");

  h.clear();

  cout << (failures == 0 ? "ALL TESTS PASSED" : "TESTS FAILED") << endl;
  return failures != 0;
} /* main */
