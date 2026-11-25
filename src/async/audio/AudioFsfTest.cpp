/**
@file    AudioFsfTest.cpp
@brief   Unit test for AudioFsf (frequency sampling filter).
@author  Mark Rose
@date    2026-06-06

Builds the bandpass FSF used by the AFSK modulator (N=128, passband centered on
bin 44 = 5500 Hz at 16 kHz) and checks that an in-passband tone passes while an
out-of-band tone is strongly attenuated.

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

#include <AsyncCppApplication.h>
#include <AsyncAudioSource.h>
#include <AsyncAudioSink.h>
#include <AsyncAudioFsf.h>

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
  size_t start = v.size() / 2;
  double sum = 0.0;
  size_t n = 0;
  for (size_t i = start; i < v.size(); ++i, ++n)
  {
    sum += static_cast<double>(v[i]) * v[i];
  }
  return n > 0 ? sqrt(sum / n) : 0.0;
}

// The AFSK-modulator bandpass FSF: center bin 44 (5500 Hz), ~400 Hz wide.
double fsf_gain(float freq)
{
  const size_t N = 128;
  float coeff[N / 2 + 1];
  memset(coeff, 0, sizeof(coeff));
  coeff[42] = 0.39811024f;
  coeff[43] = 1.0f;
  coeff[44] = 1.0f;
  coeff[45] = 1.0f;
  coeff[46] = 0.39811024f;

  PushSource src;
  AudioFsf fsf(N, coeff);
  Capture cap;
  src.registerSink(&fsf);
  fsf.registerSink(&cap);

  const int n = 8000;
  vector<float> in(n);
  for (int i = 0; i < n; ++i)
  {
    in[i] = 0.3f * sinf(2.0f * M_PI * freq * i / RATE);
  }
  double in_rms = 0.3 / sqrt(2.0);
  src.push(in);
  return rms_tail(cap.data) / in_rms;
}


void test_passband_vs_stopband(void)
{
  cout << "test_passband_vs_stopband" << endl;
  double inband = fsf_gain(5500.0f);
  double stopband = fsf_gain(1000.0f);
  check(inband > 0.3,
        "passband tone (5500 Hz) passes (gain " + to_string(inband) + ")");
  check(stopband < 0.1,
        "out-of-band tone (1000 Hz) attenuated (gain " +
        to_string(stopband) + ")");
  check(inband > stopband * 5.0,
        "passband gain is well above stopband gain");
}

} /* anonymous namespace */


int main(void)
{
  CppApplication app;
  test_passband_vs_stopband();

  cout << endl;
  if (failures == 0)
  {
    cout << "All AudioFsf tests passed" << endl;
    return 0;
  }
  cout << failures << " AudioFsf test check(s) FAILED" << endl;
  return 1;
}
