/**
@file    SwSel5DecoderTest.cpp
@brief   Unit test for the SEL5 decoder Goertzel offset-bank fix
@author  Mark Rose
@date    2026-07-11

SwSel5Decoder::writeSamples() runs two Goertzel filter banks per tone: a
main bank aligned to the detection block boundary and a half-block-offset
bank meant to catch tones that straddle a block boundary. Both banks write
their result into the same row_energy[] slot. If the offset bank is never
fed samples, it always evaluates to zero energy and periodically clobbers
the valid energy computed by the main bank, intermittently blanking out the
detected tone. That makes the active_timer streak in Sel5PostProcess()
reset before it accumulates enough consecutive hits, so a full 4-digit
ZVEI1 selcall sequence is dropped.

This test feeds a short 4-digit ZVEI1 sequence, with each digit held for
just long enough to be recognized with a correctly-fed offset bank but too
short to survive the ~50% duty-cycle blanking caused by the unfed offset
bank. It verifies the complete sequence is detected.

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2026 Tobias Blomberg / SM0SVX

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.
\endverbatim
*/

#include <cmath>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include <AsyncConfig.h>

#include "SwSel5Decoder.h"

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

  // ZVEI1 tone table, matching the order of SwSel5Decoder's "0123456789ABCDEF"
const char* TONEDEF = "0123456789ABCDEF";
const float ZVEI1_FREQ[] = {
  2400.0f, 1060.0f, 1160.0f, 1270.0f, 1400.0f, 1530.0f,
  1670.0f, 1830.0f, 2000.0f, 2200.0f, 2800.0f,  810.0f,
   970.0f,  885.0f, 2600.0f,  680.0f
};

float freqForDigit(char digit)
{
  const char *p = strchr(TONEDEF, digit);
  return ZVEI1_FREQ[p - TONEDEF];
}

  // Feed a ZVEI1 selcall sequence into the decoder, holding each digit's
  // tone for 'hold_samples' samples, followed by trailing silence long
  // enough to flush a detected sequence.
string decodeSequence(const string &digits, int hold_samples)
{
  Async::Config cfg;
  cfg.setValue("Sel5", "SEL5_TYPE", "ZVEI1");

  SwSel5Decoder dec(cfg, "Sel5");
  dec.initialize();

  string detected;
  dec.sequenceDetected.connect(
      [&](const string &s) { detected = s; });

  const int SAMPLE_RATE = 16000;
  double phase = 0.0;
  vector<float> buf(64);

  for (char digit : digits)
  {
    float freq = freqForDigit(digit);
    int remaining = hold_samples;
    while (remaining > 0)
    {
      int chunk = min(static_cast<int>(buf.size()), remaining);
      for (int i = 0; i < chunk; i++)
      {
        buf[i] = 0.8f * sinf(2.0f * static_cast<float>(M_PI) * freq *
                              static_cast<float>(phase) / SAMPLE_RATE);
        phase += 1.0;
      }
      dec.writeSamples(buf.data(), chunk);
      remaining -= chunk;
    }
  }

    // Trailing silence: stable_timer must exceed 120 outer (16-sample)
    // detection blocks with no tone present to flush the sequence.
  vector<float> silence(64, 0.0f);
  int remaining = 16 * 200;
  while (remaining > 0)
  {
    int chunk = min(static_cast<int>(silence.size()), remaining);
    dec.writeSamples(silence.data(), chunk);
    remaining -= chunk;
  }

  return detected;
} /* decodeSequence */

} /* anonymous namespace */


int main(void)
{
    // Each digit is held for only 360 samples (22.5 ms) -- long enough for
    // the fully-fed Goertzel filter bank to reliably detect the tone, but
    // short enough that the ~50%-duty-cycle blanking caused by a dead
    // half-block-offset bank (the pre-fix bug) prevents the active_timer
    // streak from ever completing.
  string detected = decodeSequence("1234", 360);
  check(detected == "1234",
        "4-digit ZVEI1 sequence with short-held digits is fully decoded");

  cout << (failures == 0 ? "ALL TESTS PASSED" : "TESTS FAILED") << endl;
  return failures != 0;
} /* main */
