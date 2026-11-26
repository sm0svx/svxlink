/**
@file    VoterMuteTest.cpp
@brief   Unit test for muting the Voter while several receivers are open
@author  Mark Rose
@date    2026-11-26

When the voter is muted (MUTE_ALL) it mutes all its receivers, and a
receiver with an open squelch closes it right away, like LocalRxBase and
NetRx do. The squelch close events are queued by the voter and handled
after all receivers have been muted. When the best receiver was listed
before another open receiver, the close event for the best receiver
cleared the best receiver and the close event for the next one failed
assert(is_open) in Voter::Top::satSquelchOpen(), aborting the process.

This test drives a real Voter with two in-process Rx stand-ins and mutes it
while both are open, first while the voter is in the voting delay and then
after an active receiver has been selected. It also checks that voting
still works afterwards.

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
#include <map>
#include <string>

#include <AsyncCppApplication.h>
#include <AsyncConfig.h>
#include <AsyncTimer.h>

#include "Rx.h"
#include "Voter.h"

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

  // A minimal Rx stand-in. Its squelch is driven by the test and, like
  // LocalRxBase and NetRx, it closes an open squelch synchronously when it
  // is muted.
class TestRx : public Rx
{
  public:
    TestRx(Config &cfg, const string &name)
      : Rx(cfg, name), m_id(name[name.size()-1]) {}
    virtual void reset(void) {}
    virtual float signalStrength(void) const { return m_siglev; }
    virtual char sqlRxId(void) const { return m_id; }
    virtual void setMuteState(MuteState new_mute_state)
    {
      Rx::setMuteState(new_mute_state);
      if ((new_mute_state == MUTE_ALL) && squelchIsOpen())
      {
        m_siglev = 0.0f;
        setSquelchState(false, "MUTED");
      }
    }
    void openSquelch(float siglev)
    {
      m_siglev = siglev;
      setSquelchState(true);
    }
    void closeSquelch(void)
    {
      m_siglev = 0.0f;
      setSquelchState(false);
    }

  private:
    char  m_id;
    float m_siglev = 0.0f;
};

class TestRxFactory : public RxFactory
{
  public:
    TestRxFactory(void) : RxFactory("VoterMuteTestRx") {}
    map<string, TestRx*> rxs;

  protected:
    virtual Rx *createRx(Config &cfg, const string &name)
    {
      TestRx *rx = new TestRx(cfg, name);
      rxs[name] = rx;
      return rx;
    }
};

} /* anonymous namespace */


int main(void)
{
    // A local rather than a namespace-scope global, since the factory
    // registers itself in RxFactory's factories map, which is itself a
    // dynamically-initialized global.
  TestRxFactory test_rx_factory;

  CppApplication app;
  Config cfg;

  const string vname("Voter1");
  cfg.setValue("RxA", "TYPE", string("VoterMuteTestRx"));
  cfg.setValue("RxB", "TYPE", string("VoterMuteTestRx"));
  cfg.setValue(vname, "RECEIVERS", string("RxA,RxB"));
  cfg.setValue(vname, "VOTING_DELAY", string("1000"));
    // No audio buffer: the test receivers have no audio to buffer
  cfg.setValue(vname, "BUFFER_LENGTH", string("0"));

  Voter voter(cfg, vname);
  check(voter.initialize(), "Voter initializes successfully");
  TestRx *rxa = test_rx_factory.rxs["RxA"];
  TestRx *rxb = test_rx_factory.rxs["RxB"];
  check((rxa != 0) && (rxb != 0), "Voter created the test receivers");
  if ((rxa == 0) || (rxb == 0))
  {
    return 1;
  }

  voter.setMuteState(Rx::MUTE_NONE);
  check(rxa->muteState() == Rx::MUTE_NONE, "RxA unmuted");
  check(rxb->muteState() == Rx::MUTE_NONE, "RxB unmuted");

    // Both receivers open with weak signals, so the voter is in the voting
    // delay with RxA, the first one in RECEIVERS, as the best receiver.
    // Muting must not abort.
  rxa->openSquelch(40.0f);
  rxb->openSquelch(20.0f);
  voter.setMuteState(Rx::MUTE_ALL);
  check(true, "mute during the voting delay with two open receivers");
  check(!rxa->squelchIsOpen() && !rxb->squelchIsOpen(),
        "both receivers closed by the mute");

    // The voter still votes: a strong signal on RxA selects it at once.
  voter.setMuteState(Rx::MUTE_NONE);
  rxb->openSquelch(20.0f);
  rxa->openSquelch(80.0f);
  check(voter.sqlRxId() == 'A', "RxA selected as the active receiver");

    // Mute again with RxA active and RxB also open
  voter.setMuteState(Rx::MUTE_ALL);
  check(true, "mute with an active receiver and another open receiver");
  check(voter.sqlRxId() == Rx::ID_UNKNOWN, "no active receiver when muted");

    // And once more with RxB as the strongest receiver
  voter.setMuteState(Rx::MUTE_NONE);
  rxa->openSquelch(20.0f);
  rxb->openSquelch(80.0f);
  check(voter.sqlRxId() == 'B', "RxB selected as the active receiver");
  rxb->closeSquelch();
  rxa->closeSquelch();
  check(true, "squelch close of both receivers after unmute");

  cout << (failures == 0 ? "ALL TESTS PASSED" : "TESTS FAILED") << endl;
  return failures != 0;
} /* main */
