/**
@file    MultiTxTest.cpp
@brief   Unit test for the MultiTx child Tx leak on init failure
@author  Mark Rose
@date    2026-07-11

MultiTx::initialize() creates a child Tx via TxFactory::createNamedTx() for
each entry in TRANSMITTERS and then calls tx->initialize() on it. If that
call fails, the child was never added to the txs list, so it was never
reachable from ~MultiTx() and leaked.

This test drives that exact path using a real "Local" type transmitter that
is deliberately misconfigured (no AUDIO_DEV) so that LocalTx::initialize()
fails immediately without touching any audio hardware. It verifies the
leak is gone by instrumenting the global allocator: after the failed
initialize() call, the only allocation that should still be outstanding is
the AudioSplitter that MultiTx keeps alive -- not the failed child Tx.

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2026 Tobias Blomberg / SM0SVX

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.
\endverbatim
*/

#include <cstdlib>
#include <cstdio>
#include <atomic>
#include <iostream>
#include <string>
#include <new>

#include <AsyncConfig.h>
#include <AsyncAudioSplitter.h>

#include "MultiTx.h"

using namespace std;
using namespace Async;

namespace {

  // Track every heap allocation/deallocation made by the process so that
  // leaks can be detected without a sanitizer: a leaked object simply never
  // decrements this counter.
std::atomic<long> live_allocs{0};

} /* anonymous namespace */

void* operator new(std::size_t sz)
{
  void *p = std::malloc(sz ? sz : 1);
  if (p == 0)
  {
    throw std::bad_alloc();
  }
  ++live_allocs;
  return p;
}

void operator delete(void *p) noexcept
{
  if (p != 0)
  {
    --live_allocs;
  }
  std::free(p);
}

void operator delete(void *p, std::size_t) noexcept
{
  if (p != 0)
  {
    --live_allocs;
  }
  std::free(p);
}

void* operator new[](std::size_t sz)
{
  return ::operator new(sz);
}

void operator delete[](void *p) noexcept
{
  ::operator delete(p);
}

void operator delete[](void *p, std::size_t sz) noexcept
{
  ::operator delete(p, sz);
}


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

} /* anonymous namespace */


int main(void)
{
    // Measure how many allocations a lone AudioSplitter accounts for. This
    // is exactly the allocation that MultiTx::initialize() legitimately
    // keeps alive after a failed child Tx creation, so we use it as the
    // expected outstanding-allocation baseline instead of a hard-coded
    // number.
  long before_splitter = live_allocs.load();
  AudioSplitter *reference_splitter = new AudioSplitter;
  long splitter_cost = live_allocs.load() - before_splitter;
  delete reference_splitter;
  check(splitter_cost > 0, "measuring AudioSplitter allocation cost");

  Config cfg;
  cfg.setValue("Tx1", "TYPE", string("Local"));
    // Deliberately leave AUDIO_DEV (and everything after it) unset so that
    // LocalTx::initialize() fails immediately, before touching any audio
    // hardware, while still returning a real, freshly allocated LocalTx
    // instance from TxFactory::createNamedTx().
  cfg.setValue("Multi1", "TRANSMITTERS", string("Tx1"));

  MultiTx *multi_tx = new MultiTx(cfg, "Multi1");

  long before_init = live_allocs.load();
  bool result = multi_tx->initialize();
  long after_init = live_allocs.load();

  check(!result, "MultiTx::initialize() fails when a child Tx fails to init");

  long outstanding = after_init - before_init;
  cout << "  info  outstanding allocations after failed initialize(): "
       << outstanding << " (expected " << splitter_cost << ")" << endl;
  check(outstanding == splitter_cost,
        "failed child Tx does not leak (only the AudioSplitter remains "
        "allocated)");

  delete multi_tx;

  cout << endl;
  if (failures == 0)
  {
    cout << "All MultiTx tests passed" << endl;
    return 0;
  }
  cout << failures << " MultiTx test check(s) FAILED" << endl;
  return 1;
} /* main */
