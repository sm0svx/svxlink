/**
@file    DtmfEncoderTest.cpp
@brief   Round-trip test: DtmfEncoder output decoded by DtmfDecoder.
@author  Mark Rose
@date    2026-06-06

Generates a clean (noise-free) DTMF tone sequence for the whole digit alphabet
with DtmfEncoder, feeds it straight into the INTERNAL DtmfDecoder, and asserts
the decoded digits match what was sent. This validates the encoder (which has
no test of its own) and the decoder on a clean signal. (DtmfDecoderTest covers
the noisy/stress case.)

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2026 Tobias Blomberg / SM0SVX

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.
\endverbatim
*/

#include <iostream>
#include <string>

#include <AsyncConfig.h>

#include "DtmfDecoder.h"
#include "DtmfEncoder.h"

using namespace std;
using namespace Async;

namespace {

int failures = 0;
string received;

void check(bool cond, const string& msg)
{
  cout << (cond ? "  ok   " : "  FAIL ") << msg << endl;
  if (!cond)
  {
    ++failures;
  }
}

void on_digit(char ch, int /*duration*/)
{
  received += ch;
}

// Encode `digits` with a fresh encoder->decoder pipe and return the decoded
// string.
string round_trip(const string& digits)
{
  Config cfg;
  cfg.setValue("Test", "DTMF_DEC_TYPE", string("INTERNAL"));

  DtmfEncoder enc;
  enc.setDigitDuration(80);
  enc.setDigitSpacing(80);
  enc.setDigitPower(-10);

  DtmfDecoder* dec = DtmfDecoder::create(0, cfg, "Test");
  if ((dec == 0) || !dec->initialize())
  {
    cout << "*** ERROR: could not create/initialize DTMF decoder" << endl;
    return "";
  }
  dec->digitDeactivated.connect(sigc::ptr_fun(on_digit));
  enc.registerSink(dec, true);

  received.clear();
  // A low-power leading digit settles the detector before the real sequence.
  int p = enc.digitPower();
  enc.setDigitPower(-100);
  enc.send("0");
  enc.setDigitPower(p);
  enc.send(digits);
  return received;
}


void test_full_alphabet_round_trip(void)
{
  cout << "test_full_alphabet_round_trip" << endl;
  const string sent = "0123456789ABCD*#";
  string got = round_trip(sent);
  check(got == sent, "decoded matches sent (sent='" + sent + "' got='" + got + "')");
}

void test_repeated_digits(void)
{
  cout << "test_repeated_digits" << endl;
  const string sent = "11227700";
  string got = round_trip(sent);
  check(got == sent, "repeated digits round-trip (sent='" + sent +
        "' got='" + got + "')");
}

} /* anonymous namespace */


int main(void)
{
  test_full_alphabet_round_trip();
  test_repeated_digits();

  cout << endl;
  if (failures == 0)
  {
    cout << "All DtmfEncoder round-trip tests passed" << endl;
    return 0;
  }
  cout << failures << " DtmfEncoder test check(s) FAILED" << endl;
  return 1;
}
