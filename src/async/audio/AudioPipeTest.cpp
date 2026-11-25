/**
@file    AudioPipeTest.cpp
@brief   Unit tests for basic audio pipe components (Amp, Valve, Splitter).
@author  Mark Rose
@date    2026-06-06

Pushes samples through individual audio pipe components and captures the output
at a leaf sink to verify their behaviour: AudioAmp scales by its gain, AudioValve
passes when open and blocks when closed, and AudioSplitter fans the same audio
out to all registered sinks.

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
#include <AsyncAudioAmp.h>
#include <AsyncAudioValve.h>
#include <AsyncAudioSplitter.h>

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

// A source that pushes samples on demand. With a leaf sink that accepts every
// sample there is no backpressure, so resumeOutput/allSamplesFlushed are no-ops.
class PushSource : public AudioSource
{
  public:
    int push(const vector<float>& s) { return sinkWriteSamples(s.data(), s.size()); }
    void resumeOutput(void) override {}
    void allSamplesFlushed(void) override {}
};

// A leaf sink that records everything written to it and accepts all samples.
class Capture : public AudioSink
{
  public:
    int writeSamples(const float* s, int count) override
    {
      for (int i = 0; i < count; ++i)
      {
        data.push_back(s[i]);
      }
      return count;
    }
    void flushSamples(void) override { sourceAllSamplesFlushed(); }
    vector<float> data;
};

bool near(float a, float b, float tol = 0.001f) { return fabs(a - b) < tol; }


void test_amp_scales(void)
{
  cout << "test_amp_scales" << endl;
  PushSource src;
  AudioAmp amp;
  Capture cap;
  src.registerSink(&amp);
  amp.registerSink(&cap);

  amp.setGain(0.0f);                       // 0 dB = unity
  src.push({0.1f, 0.2f, 0.3f});
  check(cap.data.size() == 3 &&
        near(cap.data[0], 0.1f) && near(cap.data[2], 0.3f),
        "0 dB gain passes samples unchanged");

  cap.data.clear();
  amp.setGain(6.0206f);                     // +6.02 dB ~ x2
  src.push({0.1f});
  check(cap.data.size() == 1 && near(cap.data[0], 0.2f, 0.005f),
        "+6 dB doubles the amplitude");

  cap.data.clear();
  amp.setGain(-6.0206f);                    // -6.02 dB ~ x0.5
  src.push({0.4f});
  check(cap.data.size() == 1 && near(cap.data[0], 0.2f, 0.005f),
        "-6 dB halves the amplitude");
}

void test_valve_open_close(void)
{
  cout << "test_valve_open_close" << endl;
  PushSource src;
  AudioValve valve;
  Capture cap;
  src.registerSink(&valve);
  valve.registerSink(&cap);

  valve.setOpen(true);
  src.push({0.1f, 0.2f});
  check(cap.data.size() == 2, "open valve passes samples");

  cap.data.clear();
  valve.setOpen(false);
  src.push({0.3f, 0.4f, 0.5f});
  check(cap.data.empty(), "closed valve blocks samples");

  cap.data.clear();
  valve.setOpen(true);
  src.push({0.6f});
  check(cap.data.size() == 1 && near(cap.data[0], 0.6f),
        "valve passes again after reopening");
}

void test_splitter_fans_out(void)
{
  cout << "test_splitter_fans_out" << endl;
  PushSource src;
  AudioSplitter sp;
  Capture a, b;
  src.registerSink(&sp);
  sp.addSink(&a);
  sp.addSink(&b);

  src.push({0.1f, 0.2f});
  check(a.data.size() == 2 && b.data.size() == 2,
        "both sinks receive the audio");
  check(near(a.data[0], 0.1f) && near(b.data[1], 0.2f),
        "both sinks receive identical samples");
}

} /* anonymous namespace */


int main(void)
{
  CppApplication app;   // some audio components use the task dispatcher

  test_amp_scales();
  test_valve_open_close();
  test_splitter_fans_out();

  cout << endl;
  if (failures == 0)
  {
    cout << "All audio pipe tests passed" << endl;
    return 0;
  }
  cout << failures << " audio pipe test check(s) FAILED" << endl;
  return 1;
}
