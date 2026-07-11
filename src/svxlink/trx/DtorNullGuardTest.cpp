/**
@file    DtorNullGuardTest.cpp
@brief   Unit test for null-pointer guards in destructors after failed init
@author  Mark Rose
@date    2026-07-11

Several classes cache a pointer that is only assigned by a separate
initialize()/instance() step and stays 0 when that step is never called or
fails. Their destructors used to dereference that pointer unconditionally:

  - NetRx::~NetRx()  called tcp_con->deleteInstance() with tcp_con==0.
  - NetTx::~NetTx()  called tcp_con->deleteInstance() with tcp_con==0.
  - PttPty::~PttPty() called pty->destroy() with pty==0.
  - RefCountingPty::instance() stored a NULL entry in its name->instance map
    when open() failed, so a later instance() call for the same name found
    the map entry and dereferenced the null pty via pty->m_refs += 1.

All four are exercised below. Without the fix, each of the first three
crashes with a null-pointer dereference in the destructor, and the fourth
crashes on the second instance() call for a name that fails to open.

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

#include "Rx.h"
#include "Tx.h"
#include "NetRx.h"
#include "NetTx.h"
#include "PttPty.h"
#include "RefCountingPty.h"

using namespace std;
using namespace Async;

namespace {

int failures = 0;

void check(bool cond, const string& msg)
{
  cout << (cond ? "  ok   " : "  FAIL ") << msg << endl;
  cout.flush();
  if (!cond)
  {
    ++failures;
  }
}


void test_netrx_dtor_null_guard(void)
{
  cout << "test_netrx_dtor_null_guard" << endl;
  Config cfg;
  {
      // initialize() is deliberately not called, so tcp_con stays 0.
      // The destructor must not dereference it.
    NetRx rx(cfg, "Rx1");
  }
  check(true, "NetRx destructor survives with tcp_con==0");
}


void test_nettx_dtor_null_guard(void)
{
  cout << "test_nettx_dtor_null_guard" << endl;
  Config cfg;
  {
      // initialize() is deliberately not called, so tcp_con stays 0.
    NetTx tx(cfg, "Tx1");
  }
  check(true, "NetTx destructor survives with tcp_con==0");
}


void test_pttpty_dtor_null_guard(void)
{
  cout << "test_pttpty_dtor_null_guard" << endl;
  {
      // initialize() is deliberately not called, so pty stays 0.
    PttPty ptt;
  }
  check(true, "PttPty destructor survives with pty==0");
}


void test_refcountingpty_no_null_entry_on_failed_open(void)
{
  cout << "test_refcountingpty_no_null_entry_on_failed_open" << endl;

    // A slave-link path in a directory that does not exist makes the
    // symlink() call in Async::Pty::open() fail, so RefCountingPty::open()
    // returns false.
  const string bad_name("/no/such/directory/badpty");

  RefCountingPty *first = RefCountingPty::instance(bad_name);
  check(first == 0, "instance() returns null when open() fails");

    // Before the fix, the failed first call left a NULL entry in the
    // ptys() map under bad_name. This second call would then find that
    // map entry and dereference the null pointer (pty->m_refs += 1)
    // instead of retrying open(). With the fix, no entry was stored, so
    // this call retries and cleanly fails again instead of crashing.
  RefCountingPty *second = RefCountingPty::instance(bad_name);
  check(second == 0, "second instance() call for the same failed name "
                      "also returns null instead of crashing");
}

} /* anonymous namespace */


int main(void)
{
    // Some of the classes under test (RefCountingPty, via Async::Pty) use
    // an Async::Timer/FdWatch, which need an Async::Application instance
    // to exist. The event loop is never run.
  CppApplication app;

  test_netrx_dtor_null_guard();
  test_nettx_dtor_null_guard();
  test_pttpty_dtor_null_guard();
  test_refcountingpty_no_null_entry_on_failed_open();

  cout << (failures == 0 ? "ALL TESTS PASSED" : "TESTS FAILED") << endl;
  return failures != 0;
} /* main */
