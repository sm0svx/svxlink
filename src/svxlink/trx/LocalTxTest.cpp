/**
@file    LocalTxTest.cpp
@brief   Unit test for PTT-stuck-keyed fixes in LocalTx
@author  Mark Rose
@date    2026-07-11

Exercises two related LocalTx defects:

 1. ~LocalTx() called transmit(false) and then went straight on to delete
    ptt/ptt_hangtimer. If a PTT_HANGTIME was pending (i.e. transmit(false) had
    already been called once and the hangtimer armed), transmit(false) is a
    no-op on the second call (do_transmit == isTransmitting()) and even a
    fresh call to setPtt(false, true) just (re)arms the hangtimer instead of
    deasserting PTT -- so the object was destroyed with the PTT hardware left
    keyed and the hangtimer that would eventually have turned it off deleted
    before it could fire.

 2. LocalTx::transmit(true) asserted PTT even when the transmit audio device
    failed to open (the early "is_transmitting = false; return;" was
    commented out), keying the transmitter with no audio flowing.

Both bugs are observed here through a small in-process Ptt implementation
("Test") that just records every setTxOn() call, so no real PTT hardware is
needed. A "udp:" AUDIO_DEV is used for the audio device so no real sound
card is needed either -- opening it in write mode is just creating a UDP
socket. LocalTx::initialize() itself validates the device by opening and
closing it once, so to reproduce a transmit-time-only open failure (as
opposed to a bad device spec, which initialize() would already reject), the
test temporarily lowers RLIMIT_NOFILE so that the socket() call performed by
LocalTx::transmit(true) fails with EMFILE, then restores the original limit.

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2026 Tobias Blomberg / SM0SVX

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.
\endverbatim
*/

#include <fcntl.h>
#include <sys/resource.h>
#include <unistd.h>

#include <iostream>
#include <string>
#include <vector>

#include <AsyncCppApplication.h>
#include <AsyncConfig.h>

#include "LocalTx.h"
#include "Ptt.h"

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

  // A Ptt implementation that just logs every setTxOn() call so that the
  // test can observe whether (and with what argument) LocalTx asserted or
  // deasserted PTT, without touching any real hardware.
class TestPtt : public Ptt
{
  public:
    struct Factory : public PttFactory<TestPtt>
    {
      Factory(void) : PttFactory<TestPtt>("Test") {}
    };

    virtual bool initialize(Config&, const string) { return true; }

    virtual bool setTxOn(bool tx_on)
    {
      calls.push_back(tx_on);
      return true;
    }

    static vector<bool> calls;
};
vector<bool> TestPtt::calls;

} /* anonymous namespace */


int main(void)
{
    // Registered as a local rather than a namespace-scope global: the
    // FactoryBase<Ptt>::factories map that Factory's constructor inserts
    // into is itself a dynamically-initialized global (a template static
    // data member), and its initialization order relative to another
    // namespace-scope global in this translation unit is unspecified. A
    // local variable sidesteps that static-initialization-order hazard,
    // since all namespace-scope globals are guaranteed to be fully
    // constructed before main() runs.
  TestPtt::Factory test_ptt_factory;

  CppApplication app;
  Config cfg;

  //--------------------------------------------------------------------
  // Scenario 1: PTT stuck keyed on destruct while a hangtime is pending.
  //--------------------------------------------------------------------
  {
    const string name("Tx1");
    cfg.setValue(name, "AUDIO_DEV", "udp:127.0.0.1:12345");
    cfg.setValue(name, "AUDIO_CHANNEL", "0");
    cfg.setValue(name, "PTT_TYPE", "Test");
    cfg.setValue(name, "PTT_HANGTIME", "100");

    LocalTx *tx = new LocalTx(cfg, name);
    check(tx->initialize(), "Tx1 initializes successfully");

    TestPtt::calls.clear();

      // Key up: audio device (a UDP socket) opens fine, so PTT is asserted.
    tx->setTxCtrlMode(Tx::TX_ON);
    check(!TestPtt::calls.empty() && TestPtt::calls.back(),
          "PTT asserted after TX_ON");

      // Key down: with PTT_HANGTIME configured, transmit(false) arms the
      // hangtimer instead of deasserting PTT immediately -- this is the
      // pending-hangtime state that ~LocalTx() must not leave keyed.
    TestPtt::calls.clear();
    tx->setTxCtrlMode(Tx::TX_OFF);
    check(TestPtt::calls.empty(),
          "PTT not yet deasserted while hangtime is pending (precondition)");

      // Destroying the Tx while the hangtimer is still pending must not
      // leave the hardware PTT keyed.
    delete tx;
    check(!TestPtt::calls.empty() && !TestPtt::calls.back(),
          "PTT forced off by destructor while a hangtime was pending");
  }

  //--------------------------------------------------------------------
  // Scenario 2: PTT must not be asserted when the audio device fails to
  // open.
  //--------------------------------------------------------------------
  {
    const string name("Tx2");
    cfg.setValue(name, "AUDIO_DEV", "udp:127.0.0.1:12346");
    cfg.setValue(name, "AUDIO_CHANNEL", "0");
    cfg.setValue(name, "PTT_TYPE", "Test");

    LocalTx *tx = new LocalTx(cfg, name);
      // initialize() validates AUDIO_DEV by opening and closing it once, so
      // a bad device spec would be rejected right here -- that is not the
      // bug under test.
    check(tx->initialize(), "Tx2 initializes successfully");

    TestPtt::calls.clear();

      // Find out which fd number would be allocated next, then lower
      // RLIMIT_NOFILE to disallow it, so that the socket() call inside
      // LocalTx::transmit(true) deterministically fails with EMFILE --
      // simulating a transmit-time audio-device-open failure without
      // needing real, unreliable hardware.
    int probe_fd = open("/dev/null", O_RDONLY);
    check(probe_fd >= 0, "probing the next available fd number succeeds");
    close(probe_fd);

    struct rlimit rl;
    check(getrlimit(RLIMIT_NOFILE, &rl) == 0, "getrlimit(RLIMIT_NOFILE) succeeds");
    rlim_t orig_cur = rl.rlim_cur;
    rl.rlim_cur = probe_fd;
    check(setrlimit(RLIMIT_NOFILE, &rl) == 0,
          "setrlimit lowers the fd limit below the next fd number");

    tx->setTxCtrlMode(Tx::TX_ON);

    rl.rlim_cur = orig_cur;
    setrlimit(RLIMIT_NOFILE, &rl);   // restore before doing anything else

    check(TestPtt::calls.empty(),
          "PTT not asserted when the audio device fails to open");
    check(!tx->isTransmitting(),
          "isTransmitting() false after a failed audio-device open");

    delete tx;
  }

  cout << (failures == 0 ? "ALL TESTS PASSED" : "TESTS FAILED") << endl;
  return failures != 0;
} /* main */
