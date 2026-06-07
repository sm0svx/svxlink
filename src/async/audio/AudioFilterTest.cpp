/**
@file    AudioFilterTest.cpp
@brief   Unit tests for AudioFilter frequency response.
@author  Mark Rose
@date    2026-06-06

Feeds a single-tone signal through an AudioFilter and measures the steady-state
output level (RMS) relative to the input. A lowpass filter should pass a tone
well below the cutoff and strongly attenuate one well above it; a highpass
filter should do the reverse.

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
#include <AsyncAudioFilter.h>

using namespace std;
using namespace Async;

namespace {

int failures = 0;
const int RATE = 16000;

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
    int push(const vector<float>& s) { return sinkWriteSamples(s.data(), s.size()); }
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
  // Measure the steady-state second half, skipping the filter's settling
  // transient.
  size_t start = v.size() / 2;
  double sum = 0.0;
  size_t n = 0;
  for (size_t i = start; i < v.size(); ++i, ++n)
  {
    sum += static_cast<double>(v[i]) * v[i];
  }
  return n > 0 ? sqrt(sum / n) : 0.0;
}

// Run a `freq` Hz tone (amplitude 0.3) through `spec` and return the ratio of
// output RMS to input RMS.
double gain_ratio(const string& spec, float freq)
{
  const int n = 4800;            // ~0.3 s at 16 kHz, plenty of settling time
  vector<float> in(n);
  for (int i = 0; i < n; ++i)
  {
    in[i] = 0.3f * sinf(2.0f * M_PI * freq * i / RATE);
  }
  PushSource src;
  AudioFilter filt(spec, RATE);
  Capture cap;
  src.registerSink(&filt);
  filt.registerSink(&cap);
  src.push(in);
  double in_rms = 0.3 / sqrt(2.0);
  return rms_tail(cap.data) / in_rms;
}


void test_lowpass(void)
{
  cout << "test_lowpass" << endl;
  double passband = gain_ratio("LpBu8/2000", 500.0f);
  double stopband = gain_ratio("LpBu8/2000", 6000.0f);
  check(passband > 0.7, "lowpass passes 500 Hz (ratio " +
        to_string(passband) + ")");
  check(stopband < 0.1, "lowpass attenuates 6000 Hz (ratio " +
        to_string(stopband) + ")");
}

void test_highpass(void)
{
  cout << "test_highpass" << endl;
  double passband = gain_ratio("HpBu8/2000", 6000.0f);
  double stopband = gain_ratio("HpBu8/2000", 500.0f);
  check(passband > 0.7, "highpass passes 6000 Hz (ratio " +
        to_string(passband) + ")");
  check(stopband < 0.1, "highpass attenuates 500 Hz (ratio " +
        to_string(stopband) + ")");
}

void test_bandpass(void)
{
  cout << "test_bandpass" << endl;
  double inband = gain_ratio("BpBu8/1500-2500", 2000.0f);
  double below = gain_ratio("BpBu8/1500-2500", 300.0f);
  double above = gain_ratio("BpBu8/1500-2500", 6000.0f);
  check(inband > 0.6, "bandpass passes 2000 Hz (ratio " +
        to_string(inband) + ")");
  check(below < 0.1 && above < 0.1,
        "bandpass attenuates out-of-band tones");
}

} /* anonymous namespace */


int main(void)
{
  CppApplication app;

  test_lowpass();
  test_highpass();
  test_bandpass();

  cout << endl;
  if (failures == 0)
  {
    cout << "All AudioFilter tests passed" << endl;
    return 0;
  }
  cout << failures << " AudioFilter test check(s) FAILED" << endl;
  return 1;
}
