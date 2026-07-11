/**
@file    MsgHandlerWavChunkSizeTest.cpp
@brief   Unit test for WavFileQueueItem RIFF ChunkSize underflow handling
@author  Mark Rose
@date    2026-07-11

WavFileQueueItem::initialize() (in MsgHandler.cpp) parses a WAV file's RIFF
header and then loops over subchunks while `rest_size > 0`, where
`rest_size = chunk_size - 4`. The RIFF ChunkSize field is a uint32_t. Before
the fix, that subtraction was done in unsigned 32-bit arithmetic: a file with
a ChunkSize field smaller than 4 (clearly malformed -- the field must at
least cover the mandatory "WAVE" bytes) wrapped around to a huge value
(~4.29e9) that was then stored as a large *positive* int64_t rest_size,
instead of being rejected outright.

Storing a huge rest_size does not just risk a long-running parse loop -- it
also means the code no longer treats the declared ChunkSize as authoritative.
This test builds a WAV file whose outer ChunkSize field is deliberately
invalid (0) but which also contains one crafted "filler" subchunk whose
declared (also attacker-controlled) size exactly cancels out the
underflowed rest_size, driving the loop to a clean rest_size == 0 exit. Under
the bug, this makes initialize() accept the file as valid -- despite its
ChunkSize field being nonsensical -- and play the audio samples from the
real "data" subchunk. The fix rejects ChunkSize < 4 immediately, before the
subchunk loop ever runs, so the crafted file is never played.

This test drives the defect purely through the public MsgHandler API
(playFile()), which is how svxlink actually invokes WAV parsing.

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2026 Tobias Blomberg / SM0SVX

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.
\endverbatim
*/

#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <AsyncAudioSink.h>

#include "MsgHandler.h"

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

  // Sink that records every sample handed to it so the test can tell
  // whether a (mal-)crafted WAV file's audio was actually played.
class RecordingSink : public AudioSink
{
  public:
    std::vector<float> samples;

    virtual int writeSamples(const float *buf, int count)
    {
      samples.insert(samples.end(), buf, buf + count);
      return count;
    }
    virtual void flushSamples(void) { sourceAllSamplesFlushed(); }
};

void writeLE16(ofstream& f, uint16_t v)
{
  char b[2] = { static_cast<char>(v & 0xff), static_cast<char>((v >> 8) & 0xff) };
  f.write(b, 2);
}

void writeLE32(ofstream& f, uint32_t v)
{
  char b[4] = {
    static_cast<char>(v & 0xff),
    static_cast<char>((v >> 8) & 0xff),
    static_cast<char>((v >> 16) & 0xff),
    static_cast<char>((v >> 24) & 0xff)
  };
  f.write(b, 4);
}

  // Write a WAV file with a deliberately-corrupt outer ChunkSize (0), but a
  // real "fmt " and "data" subchunk, plus one crafted filler subchunk whose
  // declared size makes the unsigned-underflowed rest_size land exactly on
  // zero. Only reachable/observable if the ChunkSize < 4 guard is missing.
void writeMalformedWav(const string& path)
{
  ofstream f(path.c_str(), ios::binary | ios::trunc);

  f.write("RIFF", 4);
  writeLE32(f, 0);            // Malformed ChunkSize: must be >= 4.
  f.write("WAVE", 4);

    // "fmt " subchunk -- 16 bytes of PCM format data (24 bytes total).
  f.write("fmt ", 4);
  writeLE32(f, 16);
  writeLE16(f, 1);            // audio_format = PCM
  writeLE16(f, 1);            // num_channels = mono
  writeLE32(f, INTERNAL_SAMPLE_RATE);
  writeLE32(f, INTERNAL_SAMPLE_RATE * 2); // byte_rate
  writeLE16(f, 2);            // block_align
  writeLE16(f, 16);           // bits_per_sample

    // "data" subchunk -- 4 recognizable 16-bit samples (16 bytes total).
  f.write("data", 4);
  writeLE32(f, 8);
  writeLE16(f, 0x1234);
  writeLE16(f, 0x2345);
  writeLE16(f, 0x3456);
  writeLE16(f, 0x4567);

    // Filler subchunk. Reproduce the exact (buggy) unsigned arithmetic to
    // compute the declared size that zeroes out rest_size.
  uint32_t chunk_size_field = 0;
  uint32_t rest_start = chunk_size_field - 4;  // wraps: ~4294967292
  uint64_t rest_size = rest_start;
  rest_size -= (16 + 8);  // consumed by the "fmt " subchunk
  rest_size -= (8 + 8);   // consumed by the "data" subchunk
  uint32_t filler_size = static_cast<uint32_t>(rest_size - 8);

  f.write("JUNK", 4);
  writeLE32(f, filler_size);
    // No actual filler payload bytes are written; the parser only seeks
    // past them (which succeeds even beyond EOF), it never reads them.

  f.close();
}

void writeValidWav(const string& path)
{
  ofstream f(path.c_str(), ios::binary | ios::trunc);

  f.write("RIFF", 4);
  writeLE32(f, 44);            // Correct ChunkSize: 36 + data size (8)
  f.write("WAVE", 4);

  f.write("fmt ", 4);
  writeLE32(f, 16);
  writeLE16(f, 1);
  writeLE16(f, 1);
  writeLE32(f, INTERNAL_SAMPLE_RATE);
  writeLE32(f, INTERNAL_SAMPLE_RATE * 2);
  writeLE16(f, 2);
  writeLE16(f, 16);

  f.write("data", 4);
  writeLE32(f, 8);
  writeLE16(f, 0x1234);
  writeLE16(f, 0x2345);
  writeLE16(f, 0x3456);
  writeLE16(f, 0x4567);

  f.close();
}

} /* anonymous namespace */


int main(void)
{
  const string malformed_path("/tmp/svxlink_msghandler_wav_underflow_test.wav");
  const string valid_path("/tmp/svxlink_msghandler_wav_valid_test.wav");
  writeMalformedWav(malformed_path);
  writeValidWav(valid_path);

    // A WAV file with an invalid (too small) ChunkSize field must be
    // rejected outright -- its audio must never reach the sink, no matter
    // what any crafted subchunk inside it claims.
  {
    MsgHandler h(INTERNAL_SAMPLE_RATE);
    RecordingSink sink;
    h.registerSink(&sink);

    h.playFile(malformed_path, false);

    check(sink.samples.empty(),
          "WAV file with ChunkSize < 4 is rejected, not played");

    h.clear();
  }

    // A well-formed WAV file with a correct ChunkSize must still play
    // normally (the fix must not reject legitimate files).
  {
    MsgHandler h(INTERNAL_SAMPLE_RATE);
    RecordingSink sink;
    h.registerSink(&sink);

    h.playFile(valid_path, false);

    check(sink.samples.size() == 4, "valid WAV file is played in full");
    if (sink.samples.size() == 4)
    {
      check(sink.samples[0] > 0.14f && sink.samples[0] < 0.15f,
            "valid WAV file samples decode correctly");
    }

    h.clear();
  }

  remove(malformed_path.c_str());
  remove(valid_path.c_str());

  cout << (failures == 0 ? "ALL TESTS PASSED" : "TESTS FAILED") << endl;
  return failures != 0;
} /* main */
