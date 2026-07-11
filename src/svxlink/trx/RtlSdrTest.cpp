/**
@file    RtlSdrTest.cpp
@brief   Unit test for RtlSdr block-size computation
@author  Mark Rose
@date    2026-07-10

RtlSdr::setSampleRate() derives block_size = 10*2*rate/1000. A sample rate below
50 Hz made that integer expression zero, and a zero block size spins the RtlUsb
sample-buffer loop forever. This test verifies block_size is clamped to a
non-zero value for a degenerate rate while a normal rate is unaffected.

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
#include <iostream>
#include <string>

#include "RtlSdr.h"

using namespace std;

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

  // Concrete RtlSdr with all hardware hooks stubbed out.
class TestRtlSdr : public RtlSdr
{
  public:
    virtual bool isReady(void) const { return true; }
    virtual const std::string displayName(void) const { return "test"; }

      // blockSize() is protected; expose it for the test.
    size_t testBlockSize(void) const { return blockSize(); }

  protected:
    virtual void handleSetTunerIfGain(uint16_t, int16_t) {}
    virtual void handleSetCenterFq(uint32_t) {}
    virtual void handleSetSampleRate(uint32_t) {}
    virtual void handleSetGainMode(uint32_t) {}
    virtual void handleSetGain(int32_t) {}
    virtual void handleSetFqCorr(int) {}
    virtual void handleEnableTestMode(bool) {}
    virtual void handleEnableDigitalAgc(bool) {}
};

} /* anonymous namespace */


int main(void)
{
  TestRtlSdr rtl;

    // A sample rate under 50 Hz makes 10*2*rate/1000 evaluate to 0. A zero
    // block size makes the RtlUsb SampleBuffer copy loop never terminate.
  rtl.setSampleRate(40);
  check(rtl.testBlockSize() >= 1,
        "degenerate sample rate does not yield a zero block size");

    // A normal sample rate still produces the expected 10 ms block.
  rtl.setSampleRate(960000);
  check(rtl.testBlockSize() == 10u * 2u * 960000u / 1000u,
        "normal sample rate block size unchanged");
  check(rtl.testBlockSize() > 0, "normal sample rate block size positive");

  cout << (failures == 0 ? "ALL TESTS PASSED" : "TESTS FAILED") << endl;
  return failures != 0;
} /* main */
