/**
@file    DtmfDigitHandlerTest.cpp
@brief   Unit tests for DtmfDigitHandler (DTMF command accumulation).
@author  Mark Rose
@date    2026-06-06

DtmfDigitHandler accumulates received DTMF digits into a command string,
completing on '#', with a few special digits (A enables anti-flutter, D resets
to "D", H inserts a literal '#', B repeats the previous digit under
anti-flutter) and a 20-digit cap. These tests drive digitReceived() directly
and inspect command()/the commandComplete signal. The 10-second inactivity
timeout is not exercised (it requires the event loop); a CppApplication is
created only so the internal Async::Timer can be constructed.

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2026 Tobias Blomberg / SM0SVX

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.
\endverbatim
*/

#include <AsyncCppApplication.h>

#include <iostream>
#include <string>

#include "DtmfDigitHandler.h"

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

// Records commandComplete emissions and the command string at completion time
// (DtmfDigitHandler resets the buffer right after emitting, so it must be read
// from within the signal).
class Recorder : public sigc::trackable
{
  public:
    explicit Recorder(DtmfDigitHandler& dh) : m_dh(dh)
    {
      dh.commandComplete.connect(sigc::mem_fun(*this, &Recorder::onComplete));
    }
    void onComplete(void)
    {
      ++completions;
      last_command = m_dh.command();
    }
    int completions = 0;
    string last_command;
  private:
    DtmfDigitHandler& m_dh;
};

void feed(DtmfDigitHandler& dh, const string& digits)
{
  for (char d : digits)
  {
    dh.digitReceived(d);
  }
}


void test_basic_accumulation(void)
{
  cout << "test_basic_accumulation" << endl;
  DtmfDigitHandler dh;
  feed(dh, "123");
  check(dh.command() == "123", "digits accumulate to '123'");
}

void test_hash_completes_and_resets(void)
{
  cout << "test_hash_completes_and_resets" << endl;
  DtmfDigitHandler dh;
  Recorder rec(dh);
  feed(dh, "12#");
  check(rec.completions == 1, "commandComplete emitted once on '#'");
  check(rec.last_command == "12", "command was '12' at completion");
  check(dh.command() == "", "buffer cleared after completion");
}

void test_star_rules(void)
{
  cout << "test_star_rules" << endl;
  DtmfDigitHandler dh;
  feed(dh, "**");
  check(dh.command() == "*", "leading '*' kept once, duplicate '*' ignored");
}

void test_max_length(void)
{
  cout << "test_max_length" << endl;
  DtmfDigitHandler dh;
  feed(dh, string(25, '1'));
  check(dh.command().size() == 20, "buffer capped at 20 digits");
}

void test_d_resets_to_D(void)
{
  cout << "test_d_resets_to_D" << endl;
  DtmfDigitHandler dh;
  feed(dh, "12D");
  check(dh.command() == "D", "'D' resets the buffer to 'D'");
}

void test_h_inserts_hash(void)
{
  cout << "test_h_inserts_hash" << endl;
  DtmfDigitHandler dh;
  feed(dh, "1H2");
  check(dh.command() == "1#2", "'H' inserts a literal '#'");
}

void test_anti_flutter_dedup(void)
{
  cout << "test_anti_flutter_dedup" << endl;
  DtmfDigitHandler dh;
  feed(dh, "A");
  check(dh.antiFlutterActive(), "'A' enables anti-flutter");
  feed(dh, "1123");                 // consecutive duplicates collapsed
  check(dh.command() == "123", "consecutive duplicate digits collapsed");
  feed(dh, "B");                    // back-up repeats the previous digit
  check(dh.command() == "1233", "'B' repeats the previous digit");
}

void test_anti_flutter_c_completes(void)
{
  cout << "test_anti_flutter_c_completes" << endl;
  DtmfDigitHandler dh;
  Recorder rec(dh);
  feed(dh, "A12C");
  check(rec.completions == 1, "'C' completes the command under anti-flutter");
  check(rec.last_command == "12", "command was '12' at completion");
}

void test_force_command_complete(void)
{
  cout << "test_force_command_complete" << endl;
  DtmfDigitHandler dh;
  Recorder rec(dh);
  feed(dh, "12");
  dh.forceCommandComplete();
  check(rec.completions == 1 && rec.last_command == "12",
        "forceCommandComplete emits with the buffered command");
  dh.forceCommandComplete();
  check(rec.completions == 1, "force on empty buffer emits nothing");
}

void test_reset(void)
{
  cout << "test_reset" << endl;
  DtmfDigitHandler dh;
  feed(dh, "A12");
  dh.reset();
  check(dh.command() == "" && !dh.antiFlutterActive(),
        "reset clears buffer and anti-flutter");
}

} /* anonymous namespace */


int main(void)
{
  CppApplication app;   // needed so DtmfDigitHandler's Async::Timer constructs

  test_basic_accumulation();
  test_hash_completes_and_resets();
  test_star_rules();
  test_max_length();
  test_d_resets_to_D();
  test_h_inserts_hash();
  test_anti_flutter_dedup();
  test_anti_flutter_c_completes();
  test_force_command_complete();
  test_reset();

  cout << endl;
  if (failures == 0)
  {
    cout << "All DtmfDigitHandler tests passed" << endl;
    return 0;
  }
  cout << failures << " DtmfDigitHandler test check(s) FAILED" << endl;
  return 1;
}
