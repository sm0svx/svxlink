/**
@file    AudioCodecRobustnessTest.cpp
@brief   Unit tests for robustness fixes in the audio codecs
@author  Mark Rose
@date    2026-10-16

Covers the RAW decoder, an invalid GSM frame and the Speex
encoder frames per packet option. Codecs that are not available in the
build are skipped.

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2003-2026 Tobias Blomberg / SM0SVX

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.
\endverbatim
*/

#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <memory>
#include <string>
#include <vector>

#include <AsyncAudioDecoder.h>
#include <AsyncAudioEncoder.h>
#include <AsyncAudioSink.h>

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

class CaptureSink : public AudioSink
{
  public:
    vector<float> samples;
    int writeSamples(const float* buf, int count) override
    {
      samples.insert(samples.end(), buf, buf + count);
      return count;
    }
    void flushSamples(void) override { sourceAllSamplesFlushed(); }
};

unique_ptr<AudioDecoder> decoder(const string& name, CaptureSink& sink)
{
  unique_ptr<AudioDecoder> dec(AudioDecoder::create(name));
  if (dec)
  {
    dec->registerSink(&sink);
  }
  return dec;
}

} /* anonymous namespace */


int main(void)
{
  {
    CaptureSink sink;
    auto dec = decoder("RAW", sink);
    const float in[] = {0.5f, std::numeric_limits<float>::quiet_NaN(),
                        std::numeric_limits<float>::infinity(), 5.0f,
                        -5.0f};
    dec->writeEncodedSamples(const_cast<float*>(in), sizeof(in));
    check((sink.samples.size() == 5) && (sink.samples[0] == 0.5f) &&
          (sink.samples[1] == 0.0f) && (sink.samples[2] == 0.0f) &&
          (sink.samples[3] == 1.0f) && (sink.samples[4] == -1.0f),
          "RAW: non-finite samples become silence and others are clamped");
  }

  {
    CaptureSink sink;
    auto dec = decoder("GSM", sink);
    if (dec)
    {
      uint8_t bad_frame[33] = {0};   // wrong GSM magic nibble
      dec->writeEncodedSamples(bad_frame, sizeof(bad_frame));
      check(sink.samples.empty(), "GSM: an invalid frame is not output");
    }
    else
    {
      cout << "  skip GSM decoder not available" << endl;
    }
  }

  {
    unique_ptr<AudioEncoder> enc(AudioEncoder::create("SPEEX"));
    if (enc)
    {
      int packets = 0;
      enc->writeEncodedSamples.connect(
          [&](const void*, int) { ++packets; });
      enc->setOption("FRAMES_PER_PACKET", "0");
      vector<float> samples(16000, 0.1f);    // one second of audio
      enc->writeSamples(samples.data(), samples.size());
      check(packets > 0,
            "SPEEX: an invalid frames per packet value does not stop the "
            "audio");
    }
    else
    {
      cout << "  skip SPEEX encoder not available" << endl;
    }
  }

  cout << (failures == 0 ? "ALL TESTS PASSED" : "TESTS FAILED") << endl;
  return failures != 0;
} /* main */
