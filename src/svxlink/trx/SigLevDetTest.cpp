/**
@file    SigLevDetTest.cpp
@brief   Unit tests for signal level detectors (SigLevDetConst).
@author  Mark Rose
@date    2026-06-06

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

#include <AsyncConfig.h>

#include "SigLevDetConst.h"

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


void test_const_configured(void)
{
  cout << "test_const_configured" << endl;
  Config cfg;
  cfg.setValue("Rx", "SIGLEV_CONST", string("42"));
  SigLevDetConst det;
  check(det.initialize(cfg, "Rx", 16000), "initializes with SIGLEV_CONST");
  check(fabs(det.lastSiglev() - 42.0f) < 0.001f, "lastSiglev returns 42");
  check(fabs(det.siglevIntegrated() - 42.0f) < 0.001f,
        "siglevIntegrated returns 42");
}

void test_const_default_zero(void)
{
  cout << "test_const_default_zero" << endl;
  Config cfg;                       // SIGLEV_CONST not set
  SigLevDetConst det;
  check(det.initialize(cfg, "Rx", 16000), "initializes without SIGLEV_CONST");
  check(fabs(det.lastSiglev()) < 0.001f, "default level is 0");
}

} /* anonymous namespace */


int main(void)
{
  test_const_configured();
  test_const_default_zero();

  cout << endl;
  if (failures == 0)
  {
    cout << "All SigLevDet tests passed" << endl;
    return 0;
  }
  cout << failures << " SigLevDet test check(s) FAILED" << endl;
  return 1;
}
