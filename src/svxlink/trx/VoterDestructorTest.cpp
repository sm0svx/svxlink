/**
@file    VoterDestructorTest.cpp
@brief   Regression test for destroying a Voter while its squelch is open
@author  Mark Rose
@date    2026-10-16

The Voter destructor deleted the satellite receivers before its state
machine was shut down. When the voter was destroyed while its squelch was
open, the state machine shutdown ran SquelchOpen::exit(), which reads from
the already deleted active receiver.

This test opens the squelch of a single satellite receiver, waits until the
voter has selected it and opened its own squelch, and then destroys the
voter. With the bug, the process crashes (or AddressSanitizer reports a
heap-use-after-free) in the destructor. It also checks that a voter that
failed to initialize can still be destroyed.

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

  // A minimal Rx stand-in whose squelch state can be flipped directly by
  // the test, with no real receiver hardware, audio device or squelch
  // detector involved.
class TestRx : public Rx
{
  public:
    TestRx(Config &cfg, const string &name) : Rx(cfg, name) {}
    virtual void reset(void) {}
    virtual void resumeOutput(void) {}
    virtual void allSamplesFlushed(void) {}
    void openSquelch(void) { setSquelchState(true); }
};

class TestRxFactory : public RxFactory
{
  public:
    TestRxFactory(void) : RxFactory("VoterDestructorTestRx") {}
    TestRx *last_created = 0;

  protected:
    virtual Rx *createRx(Config &cfg, const string &name)
    {
      last_created = new TestRx(cfg, name);
      return last_created;
    }
};

} /* anonymous namespace */


int main(void)
{
    // A local rather than a namespace-scope global since the RxFactory
    // factories map is itself a dynamically initialized global
  TestRxFactory test_rx_factory;

  CppApplication app;
  Config cfg;

  const string vname("Voter1");
  const string rname("Rx1");

  cfg.setValue(rname, "TYPE", string("VoterDestructorTestRx"));
  cfg.setValue(vname, "RECEIVERS", rname);
  cfg.setValue(vname, "VOTING_DELAY", string("10"));

  Voter *voter = new Voter(cfg, vname);
  check(voter->initialize(), "Voter initializes successfully");
  check(test_rx_factory.last_created != 0,
        "Voter created the test receiver");

  voter->setMuteState(Rx::MUTE_NONE);
  test_rx_factory.last_created->openSquelch();

    // Let the voting delay expire so that the voter opens its squelch
  Timer t(200);
  t.expired.connect([&](Timer *) { app.quit(); });
  app.exec();
  check(voter->squelchIsOpen(), "Voter squelch is open");

  delete voter;
  check(true, "Voter destroyed with an open squelch without crashing");

    // A voter that failed to initialize must also be destroyable
  Voter *bad_voter = new Voter(cfg, "Voter2");
  check(!bad_voter->initialize(), "Voter without RECEIVERS fails to initialize");
  delete bad_voter;
  check(true, "Voter that failed to initialize destroyed without crashing");

  cout << (failures == 0 ? "ALL TESTS PASSED" : "TESTS FAILED") << endl;
  return failures != 0;
} /* main */
