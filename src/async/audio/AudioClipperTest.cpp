/**
@file    AudioClipperTest.cpp
@brief   Unit tests for AudioClipper amplitude clipping.
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
#include <AsyncAudioClipper.h>

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

bool near(float a, float b, float tol = 0.0001f) { return fabs(a - b) < tol; }


void test_clips_to_level(void)
{
  cout << "test_clips_to_level" << endl;
  PushSource src;
  AudioClipper clip(0.5f);
  Capture cap;
  src.registerSink(&clip);
  clip.registerSink(&cap);

  src.push({0.3f, 0.6f, -0.6f, 0.5f, -0.5f, 0.1f, 1.0f, -1.0f});
  const float exp[] = {0.3f, 0.5f, -0.5f, 0.5f, -0.5f, 0.1f, 0.5f, -0.5f};
  bool ok = cap.data.size() == 8;
  for (size_t i = 0; ok && i < cap.data.size(); ++i)
  {
    ok = near(cap.data[i], exp[i]);
  }
  check(ok, "samples above +/-0.5 are clamped, others pass unchanged");
}

void test_default_level_unity(void)
{
  cout << "test_default_level_unity" << endl;
  PushSource src;
  AudioClipper clip;          // default clip level 1.0
  Capture cap;
  src.registerSink(&clip);
  clip.registerSink(&cap);

  src.push({0.9f, 1.5f, -2.0f});
  check(cap.data.size() == 3 && near(cap.data[0], 0.9f) &&
        near(cap.data[1], 1.0f) && near(cap.data[2], -1.0f),
        "default clip level is 1.0");
}

} /* anonymous namespace */


int main(void)
{
  CppApplication app;
  test_clips_to_level();
  test_default_level_unity();

  cout << endl;
  if (failures == 0)
  {
    cout << "All AudioClipper tests passed" << endl;
    return 0;
  }
  cout << failures << " AudioClipper test check(s) FAILED" << endl;
  return 1;
}
