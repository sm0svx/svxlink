/**
@file    SquelchCtcssTest.cpp
@brief   Functional test for SquelchCtcss multi-tone handoff.
@author  Mark Rose
@date    2026-06-10

Feeds synthesized CTCSS tones through a SquelchCtcss configured with two
tones and verifies that, when the first (active) tone drops while a second
valid tone is still present, the squelch stays open instead of closing.

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
#include <iostream>
#include <string>
#include <vector>

#include <AsyncCppApplication.h>
#include <AsyncConfig.h>

#include "SquelchCtcss.h"

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

// Feed n samples that are the sum of the given tone frequencies (an empty
// list feeds silence). Phase is derived from an absolute sample counter so
// successive calls produce a continuous waveform.
long sample_pos = 0;
void feed(Squelch& sql, const vector<float>& fqs, float amp, int n)
{
  const float sr = INTERNAL_SAMPLE_RATE;
  vector<float> buf(n);
  for (int i = 0; i < n; ++i)
  {
    float s = 0.0f;
    for (float f : fqs)
    {
      s += amp * sinf(2.0f * M_PI * f * (sample_pos + i) / sr);
    }
    buf[i] = s;
  }
  sample_pos += n;
  sql.writeSamples(buf.data(), n);
}

} /* anonymous namespace */


int main(void)
{
  CppApplication app;   // Squelch base may construct Async::Timer objects

  cout << "test_ctcss_multitone_handoff" << endl;

  Config cfg;
  cfg.setValue("Rx", "CTCSS_FQ", string("100.0,151.4"));
  cfg.setValue("Rx", "CTCSS_OPEN_THRESH", string("1.0"));
  cfg.setValue("Rx", "CTCSS_CLOSE_THRESH", string("0.5"));
  cfg.setValue("Rx", "SQL_HANGTIME", string("0"));
  cfg.setValue("Rx", "SQL_START_DELAY", string("0"));
  cfg.setValue("Rx", "SQL_DELAY", string("0"));

  SquelchCtcss sql;
  check(sql.initialize(cfg, "Rx"), "SquelchCtcss initializes");
  check(!sql.isOpen(), "closed before any audio");

  // ~1s per phase, well beyond the 100ms detect/undetect delays.
  const int N = INTERNAL_SAMPLE_RATE;

  // Phase 1: first tone only -> squelch opens.
  feed(sql, {100.0f}, 0.5f, N);
  check(sql.isOpen(), "opens on CTCSS tone A (100.0 Hz)");

  // Phase 2: both tones present -> both detectors active.
  feed(sql, {100.0f, 151.4f}, 0.5f, N);
  check(sql.isOpen(), "stays open with both tones present");

  // Phase 3: drop the first tone, keep the second. This is the bug: the
  // active detector deactivates, and pre-fix the squelch closed even though
  // the second tone is still valid. It must hand off and stay open.
  feed(sql, {151.4f}, 0.5f, N);
  check(sql.isOpen(), "stays open when first tone drops but second remains");

  // Phase 4: silence -> squelch closes.
  feed(sql, {}, 0.0f, N);
  check(!sql.isOpen(), "closes when all tones are gone");

  cout << endl;
  if (failures == 0)
  {
    cout << "All SquelchCtcss tests passed" << endl;
    return 0;
  }
  cout << failures << " SquelchCtcss test check(s) FAILED" << endl;
  return 1;
} /* main */
