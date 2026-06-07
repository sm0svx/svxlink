/**
@file    SquelchTest.cpp
@brief   Unit tests for squelch detectors (SquelchOpen, SquelchVox).
@author  Mark Rose
@date    2026-06-06

Feeds audio samples through squelch detectors and checks the open/close state.
SquelchOpen should always report open; SquelchVox should open when the audio
level rises above VOX_THRESH and close again on silence.

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
#include <AsyncConfig.h>

#include "SquelchOpen.h"
#include "SquelchVox.h"

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

void feed_tone(Squelch& sql, float amp, int n)
{
  vector<float> buf(n);
  for (int i = 0; i < n; ++i)
  {
    buf[i] = amp * sinf(2.0f * M_PI * 1000.0f * i / 16000.0f);
  }
  sql.writeSamples(buf.data(), n);
}

void feed_silence(Squelch& sql, int n)
{
  vector<float> buf(n, 0.0f);
  sql.writeSamples(buf.data(), n);
}


void test_squelch_open_always_open(void)
{
  cout << "test_squelch_open_always_open" << endl;
  Config cfg;
  SquelchOpen sql;
  check(sql.initialize(cfg, "Rx"), "SquelchOpen initializes");
  feed_silence(sql, 160);
  check(sql.isOpen(), "SquelchOpen reports open even on silence");
}

void test_vox_opens_and_closes(void)
{
  cout << "test_vox_opens_and_closes" << endl;
  Config cfg;
  cfg.setValue("Rx", "VOX_FILTER_DEPTH", string("10"));
  cfg.setValue("Rx", "VOX_THRESH", string("300"));
  cfg.setValue("Rx", "SQL_HANGTIME", string("0"));
  cfg.setValue("Rx", "SQL_START_DELAY", string("0"));
  cfg.setValue("Rx", "SQL_DELAY", string("0"));
  SquelchVox sql;
  check(sql.initialize(cfg, "Rx"), "SquelchVox initializes");
  check(!sql.isOpen(), "closed before any audio");

  feed_tone(sql, 0.3f, 480);              // well above threshold
  check(sql.isOpen(), "opens on a tone above VOX_THRESH");

  feed_silence(sql, 480);
  check(!sql.isOpen(), "closes again on silence");
}

void test_vox_ignores_quiet_signal(void)
{
  cout << "test_vox_ignores_quiet_signal" << endl;
  Config cfg;
  cfg.setValue("Rx", "VOX_FILTER_DEPTH", string("10"));
  cfg.setValue("Rx", "VOX_THRESH", string("3000"));   // high threshold
  cfg.setValue("Rx", "SQL_HANGTIME", string("0"));
  cfg.setValue("Rx", "SQL_START_DELAY", string("0"));
  cfg.setValue("Rx", "SQL_DELAY", string("0"));
  SquelchVox sql;
  sql.initialize(cfg, "Rx");
  feed_tone(sql, 0.05f, 480);             // weak signal, below threshold
  check(!sql.isOpen(), "stays closed for a signal below VOX_THRESH");
}

// Records squelchOpen signal transitions.
class SqlRecorder : public sigc::trackable
{
  public:
    void onSquelchOpen(bool is_open)
    {
      if (is_open) ++opens; else ++closes;
    }
    int opens = 0;
    int closes = 0;
};

void test_vox_emits_signal(void)
{
  cout << "test_vox_emits_signal" << endl;
  Config cfg;
  cfg.setValue("Rx", "VOX_FILTER_DEPTH", string("10"));
  cfg.setValue("Rx", "VOX_THRESH", string("300"));
  cfg.setValue("Rx", "SQL_HANGTIME", string("0"));
  cfg.setValue("Rx", "SQL_START_DELAY", string("0"));
  cfg.setValue("Rx", "SQL_DELAY", string("0"));
  SquelchVox sql;
  sql.initialize(cfg, "Rx");
  SqlRecorder rec;
  sql.squelchOpen.connect(sigc::mem_fun(rec, &SqlRecorder::onSquelchOpen));
  feed_tone(sql, 0.3f, 480);
  feed_silence(sql, 480);
  check(rec.opens == 1 && rec.closes == 1,
        "squelchOpen signal fired once open, once closed");
}

} /* anonymous namespace */


int main(void)
{
  CppApplication app;   // squelch base may construct an Async::Timer

  test_squelch_open_always_open();
  test_vox_opens_and_closes();
  test_vox_ignores_quiet_signal();
  test_vox_emits_signal();

  cout << endl;
  if (failures == 0)
  {
    cout << "All Squelch tests passed" << endl;
    return 0;
  }
  cout << failures << " Squelch test check(s) FAILED" << endl;
  return 1;
}
