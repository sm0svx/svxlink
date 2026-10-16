/**
@file    VoterTest.cpp
@brief   Unit test for the Voter unsigned-underflow voting-delay timer fix
@author  Mark Rose
@date    2026-07-11

Voter::VotingDelay::init() used to compute the voting-delay timer value as
max(votingDelay() - srx->sqlOpenDelay(), 0U). Since both operands are
unsigned, a receiver configured with a per-rx SQL_OPEN_DELAY (the ":N" suffix
on a RECEIVERS entry) larger than the voter's VOTING_DELAY makes the
subtraction wrap around to a huge unsigned value before max() ever gets a
chance to clamp it.

That huge unsigned value is then narrowed to Async::Timer::setTimeout()'s
signed int parameter. Because the wrapped value is (2^32 - epsilon) for a
small epsilon, the narrowing conversion reproduces the bit pattern of the
small negative number -epsilon, so setTimeout() takes its "disable" branch
(a negative timeout disables the timer). Voter::Top::startTimer()
unconditionally follows setTimeout() with setEnable(true) though, and
Async::Timer::setEnable(true) asserts that the timeout is non-negative
before allowing a timer to be (re-)enabled -- so the process aborts on a
failed assertion the moment a satellite receiver's squelch opens while it is
in this configuration.

This test drives a real Voter through Idle -> VotingDelay with exactly such
a configuration (VOTING_DELAY=0, a receiver with SQL_OPEN_DELAY=2000) using
a minimal in-process Rx stand-in whose squelch can be opened directly, with
no real receiver hardware or audio device needed. With the fix, the
timer delay is clamped to zero (a sane, immediate-expiry value) and
satSquelchOpen() returns normally; with the bug, the process aborts on the
assertion described above.

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
    void openSquelch(void) { setSquelchState(true); }
};

class TestRxFactory : public RxFactory
{
  public:
    TestRxFactory(void) : RxFactory("VoterTestRx") {}
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
    // A local rather than namespace-scope global for the same
    // static-initialization-order reasons documented in LocalTxTest.cpp:
    // RxFactory's factories map is itself a dynamically-initialized global.
  TestRxFactory test_rx_factory;

  CppApplication app;
  Config cfg;

  const string vname("Voter1");
  const string rname("Rx1");

  cfg.setValue(rname, "TYPE", string("VoterTestRx"));

    // VOTING_DELAY (0) is smaller than the per-receiver SQL_OPEN_DELAY
    // (2000) given after the colon -- exactly the configuration that
    // underflows votingDelay() - srx->sqlOpenDelay() in unsigned arithmetic.
  cfg.setValue(vname, "RECEIVERS", string(rname + ":2000"));
  cfg.setValue(vname, "VOTING_DELAY", string("0"));

  Voter voter(cfg, vname);
  check(voter.initialize(), "Voter initializes successfully");
  check(test_rx_factory.last_created != 0,
        "Voter created the test receiver");

    // Move the voter from its initial Muted state to Idle so that a
    // squelch-open transitions it into VotingDelay, exactly like a real
    // squelch opening on a live receiver would.
  voter.setMuteState(Rx::MUTE_NONE);

    // Open the satellite receiver's squelch. Idle::satSquelchOpen() will
    // route this into VotingDelay::init(), which computes the timer delay
    // from VOTING_DELAY and SQL_OPEN_DELAY. With the unsigned-underflow bug,
    // this call aborts the process on a failed assertion inside
    // Async::Timer::setEnable(); with the fix, it returns normally.
  test_rx_factory.last_created->openSquelch();

  check(true, "satSquelchOpen() with SQL_OPEN_DELAY > VOTING_DELAY "
              "did not crash the process");

  cout << (failures == 0 ? "ALL TESTS PASSED" : "TESTS FAILED") << endl;
  return failures != 0;
} /* main */
