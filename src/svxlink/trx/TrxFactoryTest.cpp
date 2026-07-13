/**
@file    TrxFactoryTest.cpp
@brief   Unit tests for RxFactory/TxFactory named creation.
@author  Mark Rose
@date    2026-06-06

Checks that createNamedRx/createNamedTx honour the TYPE config variable: a
"NONE" receiver/transmitter and an explicit Dummy type create an object, a
missing TYPE fails, and an unknown TYPE fails.

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

#include "Rx.h"
#include "Tx.h"

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


void test_rx_factory(void)
{
  cout << "test_rx_factory" << endl;
  Config cfg;

  Rx* none = RxFactory::createNamedRx(cfg, "NONE");
  check(none != 0, "RX 'NONE' creates a (dummy) receiver");
  delete none;

  cfg.setValue("Rx1", "TYPE", string("Dummy"));
  Rx* dummy = RxFactory::createNamedRx(cfg, "Rx1");
  check(dummy != 0, "RX TYPE=Dummy creates a receiver");
  delete dummy;

  Rx* no_type = RxFactory::createNamedRx(cfg, "RxNoType");
  check(no_type == 0, "RX with no TYPE set returns null");

  cfg.setValue("RxBogus", "TYPE", string("NoSuchType"));
  Rx* bogus = RxFactory::createNamedRx(cfg, "RxBogus");
  check(bogus == 0, "RX with unknown TYPE returns null");
}

void test_tx_factory(void)
{
  cout << "test_tx_factory" << endl;
  Config cfg;

  Tx* none = TxFactory::createNamedTx(cfg, "NONE");
  check(none != 0, "TX 'NONE' creates a (dummy) transmitter");
  delete none;

  cfg.setValue("Tx1", "TYPE", string("Dummy"));
  Tx* dummy = TxFactory::createNamedTx(cfg, "Tx1");
  check(dummy != 0, "TX TYPE=Dummy creates a transmitter");
  delete dummy;

  Tx* no_type = TxFactory::createNamedTx(cfg, "TxNoType");
  check(no_type == 0, "TX with no TYPE set returns null");

  cfg.setValue("TxBogus", "TYPE", string("NoSuchType"));
  Tx* bogus = TxFactory::createNamedTx(cfg, "TxBogus");
  check(bogus == 0, "TX with unknown TYPE returns null");
}

} /* anonymous namespace */


int main(void)
{
  test_rx_factory();
  test_tx_factory();

  cout << endl;
  if (failures == 0)
  {
    cout << "All Trx factory tests passed" << endl;
    return 0;
  }
  cout << failures << " Trx factory test check(s) FAILED" << endl;
  return 1;
}
