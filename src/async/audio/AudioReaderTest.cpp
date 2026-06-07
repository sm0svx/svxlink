/**
@file    AudioReaderTest.cpp
@brief   Unit test for AudioReader synchronous pull reads.
@author  Mark Rose
@date    2026-06-06

AudioReader lets calling code pull samples synchronously from a connected
source. This test wires a small data source to a reader and verifies that
readSamples() pulls the queued samples in order, across multiple reads.

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
#include <AsyncAudioReader.h>

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

// A source that writes its queued samples to the sink whenever the sink is
// ready (resumeOutput). When the reader is not reading, the write returns 0,
// which the AudioReader treats as "input stopped" and remembers to pull later.
class DataSource : public AudioSource
{
  public:
    vector<float> data;
    size_t pos = 0;

    void resumeOutput(void) override
    {
      while (pos < data.size())
      {
        int wrote = sinkWriteSamples(&data[pos], data.size() - pos);
        if (wrote <= 0) break;
        pos += wrote;
      }
    }
    void allSamplesFlushed(void) override {}
};

bool near(float a, float b) { return fabs(a - b) < 0.0001f; }


void test_pull_reads(void)
{
  cout << "test_pull_reads" << endl;
  DataSource src;
  AudioReader reader;
  src.registerSink(&reader);
  src.data = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f};

  // Prime the handshake: the source offers data while the reader has no
  // buffer, which marks the reader "input stopped" so the next readSamples
  // pulls.
  src.resumeOutput();

  float buf[3];
  int n1 = reader.readSamples(buf, 3);
  check(n1 == 3 && near(buf[0], 1.0f) && near(buf[1], 2.0f) && near(buf[2], 3.0f),
        "first read returns the first 3 samples");

  int n2 = reader.readSamples(buf, 3);
  check(n2 == 2 && near(buf[0], 4.0f) && near(buf[1], 5.0f),
        "second read returns the remaining 2 samples");
}

} /* anonymous namespace */


int main(void)
{
  CppApplication app;
  test_pull_reads();

  cout << endl;
  if (failures == 0)
  {
    cout << "All AudioReader tests passed" << endl;
    return 0;
  }
  cout << failures << " AudioReader test check(s) FAILED" << endl;
  return 1;
}
