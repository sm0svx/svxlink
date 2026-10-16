/**
@file    AudioDelayLineTest.cpp
@brief   Unit test for AudioDelayLine: audio is delayed by the configured time.
@author  Mark Rose
@date    2026-06-06

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
#include <AsyncAudioDelayLine.h>

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


void test_impulse_delayed(void)
{
  cout << "test_impulse_delayed" << endl;
  const int len_ms = 10;
  const int delay = len_ms * INTERNAL_SAMPLE_RATE / 1000;   // samples

  PushSource src;
  AudioDelayLine dl(len_ms);
  Capture cap;
  src.registerSink(&dl);
  dl.registerSink(&cap);

  // First block: an impulse at index 0; second block: silence. The delay line
  // emits its (initially zero) buffer first, so the impulse should reappear at
  // output index == delay.
  vector<float> block1(delay, 0.0f);
  block1[0] = 1.0f;
  vector<float> block2(delay, 0.0f);
  src.push(block1);
  src.push(block2);

  check(cap.data.size() == static_cast<size_t>(2 * delay),
        "output sample count matches input");

  // Locate the impulse in the output.
  int peak_idx = -1;
  for (size_t i = 0; i < cap.data.size(); ++i)
  {
    if (fabs(cap.data[i]) > 0.5f)
    {
      peak_idx = static_cast<int>(i);
      break;
    }
  }
  check(peak_idx == delay,
        "impulse delayed by " + to_string(delay) + " samples (found at " +
        to_string(peak_idx) + ")");

  // Everything before the delay should be silence.
  bool quiet_before = true;
  for (int i = 0; i < delay; ++i)
  {
    if (fabs(cap.data[i]) > 1e-6f) quiet_before = false;
  }
  check(quiet_before, "output is silent until the delay elapses");
}

} /* anonymous namespace */


int main(void)
{
  CppApplication app;
  test_impulse_delayed();

  cout << endl;
  if (failures == 0)
  {
    cout << "All AudioDelayLine tests passed" << endl;
    return 0;
  }
  cout << failures << " AudioDelayLine test check(s) FAILED" << endl;
  return 1;
}
