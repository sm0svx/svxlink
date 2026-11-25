/**
@file    EmphasisTest.cpp
@brief   Unit tests for the pre-/de-emphasis audio filters.
@author  Mark Rose
@date    2026-06-06

Pre-emphasis boosts higher frequencies; de-emphasis is its inverse. These tests
verify that pre-emphasis applies more gain to a high tone than a low one, and
that a pre-emphasis followed by de-emphasis approximately recovers the original
mid-band signal level.

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

#include "Emphasis.h"

using namespace std;
using namespace Async;

namespace {

int failures = 0;
const int RATE = INTERNAL_SAMPLE_RATE;

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
  size_t start = v.size() / 2;
  double sum = 0.0;
  size_t n = 0;
  for (size_t i = start; i < v.size(); ++i, ++n)
  {
    sum += static_cast<double>(v[i]) * v[i];
  }
  return n > 0 ? sqrt(sum / n) : 0.0;
}

vector<float> tone(float freq, int n, float amp = 0.2f)
{
  vector<float> v(n);
  for (int i = 0; i < n; ++i)
  {
    v[i] = amp * sinf(2.0f * M_PI * freq * i / RATE);
  }
  return v;
}


// Pass a tone through one emphasis filter and return output RMS.
template <typename Filter>
double filtered_rms(float freq)
{
  PushSource src;
  Filter filt;
  Capture cap;
  src.registerSink(&filt);
  filt.registerSink(&cap);
  src.push(tone(freq, 4800));
  return rms_tail(cap.data);
}


void test_preemphasis_boosts_high(void)
{
  cout << "test_preemphasis_boosts_high" << endl;
  double low = filtered_rms<PreemphasisFilter>(600.0f);
  double high = filtered_rms<PreemphasisFilter>(2500.0f);
  check(high > low * 1.5,
        "pre-emphasis boosts 2500 Hz more than 600 Hz (low=" +
        to_string(low) + " high=" + to_string(high) + ")");
}

void test_pre_then_de_recovers(void)
{
  cout << "test_pre_then_de_recovers" << endl;
  const float freq = 1000.0f;
  PushSource src;
  PreemphasisFilter pre;
  DeemphasisFilter de;
  Capture cap;
  src.registerSink(&pre);
  pre.registerSink(&de);
  de.registerSink(&cap);

  vector<float> in = tone(freq, 4800);
  src.push(in);
  double in_rms = rms_tail(in);
  double out_rms = rms_tail(cap.data);
  double ratio = out_rms / in_rms;
  check(ratio > 0.5 && ratio < 2.0,
        "pre+de recovers mid-band level within ~6 dB (ratio " +
        to_string(ratio) + ")");
}

} /* anonymous namespace */


int main(void)
{
  CppApplication app;
  test_preemphasis_boosts_high();
  test_pre_then_de_recovers();

  cout << endl;
  if (failures == 0)
  {
    cout << "All Emphasis tests passed" << endl;
    return 0;
  }
  cout << failures << " Emphasis test check(s) FAILED" << endl;
  return 1;
}
