/**
@file    ModuleFrnTestLinkStubs.cpp
@brief   Link-only stand-ins for symbols ModuleFrnTest never executes
@author  Mark Rose
@date    2026-07-11

ModuleFrnTest.cpp constructs and destroys a ModuleFrn without ever
calling initialize(), so it never exercises any Logic:: or LinkManager
functionality. However, Module.cpp (compiled in as part of the module
under test, since ModuleFrn derives from Module) contains a handful of
member functions -- reachable only from code paths this test never
runs -- that make *direct* (non-virtual) calls into Logic:: methods and
touch the LinkManager singleton pointer. Because those functions are
part of the same translation unit that is linked into this test binary,
the linker requires those symbols to be resolvable even though they are
never invoked at runtime by this test.

Pulling in the real Logic.cpp/LinkManager.cpp to satisfy this would drag
in the full logic-core/event-handling/link-management subsystem, which
needs a live Async event loop and application wiring -- exactly what this
small unit test intentionally avoids. These stubs instead provide trivial
definitions for just the non-virtual symbols Module.cpp references, so
the parts of ModuleFrn actually under test (the constructor/destructor
pointer lifecycle) can be linked and run standalone. None of these bodies
are ever executed by ModuleFrnTest.

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2026 Tobias Blomberg / SM0SVX

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.
\endverbatim
*/

#include "Logic.h"
#include "LinkManager.h"

using namespace std;


  // Non-virtual Logic:: member functions that Module.cpp calls directly
  // (not through a vtable). Never invoked by this test -- ModuleFrn is
  // destroyed before initialize() is ever called -- but the symbols must
  // exist for the link to succeed.
void Logic::setEventVariable(const string&, const string&) {}
void Logic::sendDtmf(const string&) {}
Module *Logic::findModule(int) { return 0; }
bool Logic::isWritingMessage(void) { return false; }

  // Storage for the LinkManager singleton pointer. Module::activate()/
  // deactivate() check LinkManager::hasInstance() (inline, reads this
  // pointer) before touching the singleton; leaving it null means that
  // guard is always false and the singleton is never dereferenced.
LinkManager *LinkManager::_instance = 0;

void LinkManager::setLogicMute(const LogicBase*, bool) {}
