/**
@file    GoertzelTest.cpp
@brief   Unit tests for the Goertzel single-bin DFT detector.
@author  Mark Rose
@date    2026-06-06

Goertzel computes the DFT for a single frequency bin. These tests feed known
sine waves through it and verify the documented relationships: the amplitude of
an on-bin tone can be recovered as A = 2*sqrt(magnitudeSquared)/block_len, an
off-bin tone produces near-zero magnitude, and reset() clears the state.

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

#include "Goertzel.h"

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

const unsigned RATE = 8000;
const unsigned N = 80;            // bin width = 100 Hz; 1000 Hz is an exact bin

// Feed `n` samples of an amplitude-A sine at `freq` into a detector tuned to
// `det_freq`, and return the recovered amplitude (A = 2*sqrt(mag^2)/n).
float recovered_amplitude(float det_freq, float freq, float amp, unsigned n)
{
  Goertzel g(det_freq, RATE);
  g.reset();
  for (unsigned i = 0; i < n; ++i)
  {
    g.calc(amp * sinf(2.0f * M_PI * freq * i / RATE));
  }
  return 2.0f * sqrtf(g.magnitudeSquared()) / n;
}


void test_on_bin_amplitude_recovery(void)
{
  cout << "test_on_bin_amplitude_recovery" << endl;
  float a = recovered_amplitude(1000.0f, 1000.0f, 0.5f, N);
  check(fabs(a - 0.5f) < 0.02f,
        "recovered amplitude ~0.5 for on-bin 1000 Hz tone (got "
        + to_string(a) + ")");

  float a2 = recovered_amplitude(1000.0f, 1000.0f, 0.25f, N);
  check(fabs(a2 - 0.25f) < 0.02f,
        "recovered amplitude ~0.25 (got " + to_string(a2) + ")");
}

void test_off_bin_rejected(void)
{
  cout << "test_off_bin_rejected" << endl;
  // Detector at 1000 Hz, tone at 1500 Hz (another exact bin) -> near zero.
  float a = recovered_amplitude(1000.0f, 1500.0f, 0.5f, N);
  check(a < 0.02f, "off-bin 1500 Hz tone rejected (got " + to_string(a) + ")");
}

void test_zero_input(void)
{
  cout << "test_zero_input" << endl;
  Goertzel g(1000.0f, RATE);
  g.reset();
  for (unsigned i = 0; i < N; ++i)
  {
    g.calc(0.0f);
  }
  check(g.magnitudeSquared() < 1e-6f, "zero input -> ~zero magnitude");
}

void test_relative_power_db(void)
{
  cout << "test_relative_power_db" << endl;
  // Halving the amplitude should reduce power by ~6 dB.
  Goertzel g1(1000.0f, RATE), g2(1000.0f, RATE);
  g1.reset();
  g2.reset();
  for (unsigned i = 0; i < N; ++i)
  {
    float s = sinf(2.0f * M_PI * 1000.0f * i / RATE);
    g1.calc(1.0f * s);
    g2.calc(0.5f * s);
  }
  float diff_db = 10.0f * log10f(g2.magnitudeSquared() / g1.magnitudeSquared());
  check(fabs(diff_db - (-6.02f)) < 0.5f,
        "half amplitude -> ~-6 dB (got " + to_string(diff_db) + ")");
}

void test_reset_clears(void)
{
  cout << "test_reset_clears" << endl;
  Goertzel g(1000.0f, RATE);
  g.reset();
  for (unsigned i = 0; i < N; ++i)
  {
    g.calc(sinf(2.0f * M_PI * 1000.0f * i / RATE));
  }
  check(g.magnitudeSquared() > 1.0f, "magnitude non-zero after a tone block");
  g.reset();
  check(g.magnitudeSquared() < 1e-6f, "magnitude ~zero after reset()");
}

} /* anonymous namespace */


int main(void)
{
  test_on_bin_amplitude_recovery();
  test_off_bin_rejected();
  test_zero_input();
  test_relative_power_db();
  test_reset_clears();

  cout << endl;
  if (failures == 0)
  {
    cout << "All Goertzel tests passed" << endl;
    return 0;
  }
  cout << failures << " Goertzel test check(s) FAILED" << endl;
  return 1;
}
