/**
@file    ResamplerTest.cpp
@brief   Unit tests for AudioInterpolator and AudioDecimator.
@author  Mark Rose
@date    2026-06-06

Uses the project's 16k<->8k multirate filter coefficients to upsample (x2) and
downsample (/2) a test tone, checking the output sample counts scale with the
rate-change factor and that an interpolate->decimate round-trip recovers a tone
that is comfortably below the Nyquist limit.

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
#include <AsyncAudioSource.h>
#include <AsyncAudioSink.h>
#include <AsyncAudioInterpolator.h>
#include <AsyncAudioDecimator.h>

#include "multirate_filter_coeff.h"

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

class PushSource : public AudioSource
{
  public:
    // Push all samples, re-offering the remainder until consumed (processors
    // such as the resamplers accept input in limited-size chunks).
    int push(const vector<float>& s)
    {
      size_t off = 0;
      while (off < s.size())
      {
        int wrote = sinkWriteSamples(s.data() + off, s.size() - off);
        if (wrote <= 0) break;
        off += wrote;
      }
      return off;
    }
    void resumeOutput(void) override {}
    void allSamplesFlushed(void) override {}
};

class Capture : public AudioSink
{
  public:
    int writeSamples(const float* s, int count) override
    {
      for (int i = 0; i < count; ++i) data.push_back(s[i]);
      return count;
    }
    void flushSamples(void) override { sourceAllSamplesFlushed(); }
    vector<float> data;
};

double rms_tail(const vector<float>& v)
{
  if (v.size() < 4) return 0.0;
  size_t start = v.size() / 4;       // skip filter group-delay transient
  size_t end = v.size() - v.size() / 4;
  double sum = 0.0;
  size_t n = 0;
  for (size_t i = start; i < end; ++i, ++n)
  {
    sum += static_cast<double>(v[i]) * v[i];
  }
  return n > 0 ? sqrt(sum / n) : 0.0;
}

// Tone at `freq` Hz sampled at `rate`, amplitude 0.3.
vector<float> tone(float freq, int rate, int n)
{
  vector<float> v(n);
  for (int i = 0; i < n; ++i)
  {
    v[i] = 0.3f * sinf(2.0f * M_PI * freq * i / rate);
  }
  return v;
}


void test_interpolator_doubles_rate(void)
{
  cout << "test_interpolator_doubles_rate" << endl;
  PushSource src;
  AudioInterpolator interp(2, coeff_16_8, coeff_16_8_taps);
  Capture cap;
  src.registerSink(&interp);
  interp.registerSink(&cap);

  const int n = 4800;
  src.push(tone(1000.0f, 8000, n));
  check(cap.data.size() == static_cast<size_t>(2 * n),
        "interpolator x2 produces twice the samples (" +
        to_string(cap.data.size()) + ")");
  check(rms_tail(cap.data) > 0.05, "tone survives interpolation");
}

void test_decimator_halves_rate(void)
{
  cout << "test_decimator_halves_rate" << endl;
  PushSource src;
  AudioDecimator decim(2, coeff_16_8, coeff_16_8_taps);
  Capture cap;
  src.registerSink(&decim);
  decim.registerSink(&cap);

  const int n = 4800;
  src.push(tone(1000.0f, 16000, n));   // 1000 Hz, well below 4 kHz Nyquist
  check(cap.data.size() == static_cast<size_t>(n / 2),
        "decimator /2 produces half the samples (" +
        to_string(cap.data.size()) + ")");
  check(rms_tail(cap.data) > 0.05, "tone survives decimation");
}

void test_round_trip_recovers_level(void)
{
  cout << "test_round_trip_recovers_level" << endl;
  PushSource src;
  AudioInterpolator interp(2, coeff_16_8, coeff_16_8_taps);
  AudioDecimator decim(2, coeff_16_8, coeff_16_8_taps);
  Capture cap;
  src.registerSink(&interp);
  interp.registerSink(&decim);
  decim.registerSink(&cap);

  const int n = 8000;
  vector<float> in = tone(1000.0f, 8000, n);
  src.push(in);
  double ratio = rms_tail(cap.data) / rms_tail(in);
  check(ratio > 0.5 && ratio < 2.0,
        "interpolate->decimate recovers the level within ~6 dB (ratio " +
        to_string(ratio) + ")");
}

} /* anonymous namespace */


int main(void)
{
  CppApplication app;
  test_interpolator_doubles_rate();
  test_decimator_halves_rate();
  test_round_trip_recovers_level();

  cout << endl;
  if (failures == 0)
  {
    cout << "All resampler tests passed" << endl;
    return 0;
  }
  cout << failures << " resampler test check(s) FAILED" << endl;
  return 1;
}
