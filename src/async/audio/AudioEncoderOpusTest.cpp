/**
@file    AudioEncoderOpusTest.cpp
@brief   Unit tests for AudioEncoderOpus frame-size validation
@author  Mark Rose
@date    2026-07-10

Verifies that AudioEncoderOpus rejects a FRAME_SIZE that does not resolve to a
positive number of samples. Previously a value rounding to zero samples produced
a zero-length sample buffer that writeSamples() then ran off (heap overflow),
and a negative value threw bad_array_new_length. The fix keeps the previously
configured (valid) frame size instead.

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
#include <vector>

#include "AsyncAudioEncoderOpus.h"

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
  AudioEncoderOpus enc;

  const int default_fs = enc.frameSize();
  check(default_fs > 0, "default frame size is positive");

    // A frame size that rounds to zero samples must be rejected, leaving the
    // previous valid frame size in place (was: zero-length allocation feeding
    // an unbounded heap overflow in writeSamples).
  enc.setFrameSize(0.0f);
  check(enc.frameSize() == default_fs,
        "FRAME_SIZE 0 ignored, previous frame size retained");
  check(enc.frameSize() > 0, "frame size remains positive after FRAME_SIZE 0");

    // Encoding after a rejected zero frame size must not run off the buffer.
  vector<float> samples(2 * default_fs, 0.0f);
  enc.writeSamples(samples.data(), static_cast<int>(samples.size()));
  check(true, "writeSamples after rejected FRAME_SIZE 0 did not crash");

    // A negative frame size must likewise be rejected (was: bad_array_new_length
    // -> abort).
  enc.setFrameSize(-20.0f);
  check(enc.frameSize() == default_fs,
        "negative FRAME_SIZE ignored, previous frame size retained");

    // The config-option path must reject it too.
  enc.setOption("FRAME_SIZE", "0");
  check(enc.frameSize() == default_fs,
        "FRAME_SIZE=0 option ignored, previous frame size retained");

    // A valid frame size is still applied.
  enc.setFrameSize(40.0f);
  check(enc.frameSize() > 0, "valid FRAME_SIZE (40 ms) applied");

  cout << (failures == 0 ? "ALL TESTS PASSED" : "TESTS FAILED") << endl;
  return failures != 0;
} /* main */
