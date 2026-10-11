/**
@file    ModuleFrnTest.cpp
@brief   Unit test for ModuleFrn init-failure pointer lifecycle
@author  Mark Rose
@date    2026-07-11

ModuleFrn::initialize() only assigns audio_fifo, audio_splitter and
audio_valve after the base Module::initialize() call succeeds. If
initialization fails (or is never called) before those members are
assigned, the ModuleFrn destructor still unconditionally calls
moduleCleanup(), which used to dereference audio_fifo unconditionally.
Because audio_fifo was not part of the constructor initializer list, it
held whatever garbage bits were already present in the object's storage,
so moduleCleanup() dereferenced an uninitialized/garbage pointer and
crashed.

This test places a ModuleFrn instance into memory that has been poisoned
with a non-canonical bit pattern *before* construction -- exactly the
kind of "garbage" an uninitialized pointer member would contain -- then
destroys the object without ever calling initialize(). With the fix,
the constructor explicitly nulls audio_fifo and moduleCleanup() guards
every audio pipeline pointer with a null check, so destruction completes
cleanly. Without the fix, moduleCleanup() dereferences the poisoned
audio_fifo pointer and the process crashes (a non-canonical x86_64
address reliably faults on first access), which CTest observes as a
failing (non-zero/signal-terminated) test.

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2026 Tobias Blomberg / SM0SVX

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.
\endverbatim
*/

#include <cstring>
#include <iostream>
#include <string>

#include "ModuleFrn.h"

using namespace std;

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
    // Poison the storage for a ModuleFrn instance with a non-canonical
    // x86_64 bit pattern before construction. This stands in for the
    // uninitialized garbage that a pointer member would contain if it is
    // never assigned by the constructor -- exactly the pre-fix bug for
    // audio_fifo. Any real dereference of this pattern as a pointer
    // reliably faults.
  alignas(ModuleFrn) unsigned char storage[sizeof(ModuleFrn)];
  memset(storage, 0xAB, sizeof(storage));

    // Construct in place. dl_handle and logic are both unused by the
    // constructor and by moduleCleanup(), so passing null is safe as
    // long as initialize() (which does dereference logic()) is never
    // called -- exactly the failure mode this test targets: initialize()
    // fails (or, as here, is never reached) before the audio pipeline
    // members are assigned.
  ModuleFrn *mod = new (storage) ModuleFrn(0, 0, "frn");

    // Destroy without ever calling initialize(). This exercises
    // moduleCleanup() with audio_fifo (and friends) left at whatever the
    // constructor set them to. Pre-fix, audio_fifo is left as the
    // poisoned garbage above and moduleCleanup() crashes dereferencing
    // it. Post-fix, audio_fifo is explicitly nulled and every dereference
    // in moduleCleanup() is null-guarded, so this returns cleanly.
  mod->~ModuleFrn();

  check(true, "destructor completed without dereferencing an "
              "uninitialized audio_fifo pointer");

  cout << (failures == 0 ? "ALL TESTS PASSED" : "TESTS FAILED") << endl;
  return failures != 0;
} /* main */
