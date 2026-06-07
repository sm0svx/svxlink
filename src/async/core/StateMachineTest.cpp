/**
@file    StateMachineTest.cpp
@brief   Unit tests for the Async::StateMachine hierarchical state machine.
@author  Mark Rose
@date    2026-06-06

Builds a small hierarchical state machine and verifies start-up into the
default substate, event-driven transitions, hierarchical isActive() queries,
and context access/mutation from a state handler.

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

#include "AsyncStateMachine.h"

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

struct Context { int a = 0; };

struct StateTop;
struct StateDisconnected;
struct StateConnected;
struct StateConnectedA;
struct StateConnectedB;

struct StateTop : Async::StateTopBase<Context, StateTop>::Type
{
  static constexpr auto NAME = "Top";
  void init(void) { setState<StateDisconnected>(); }
  virtual void eventA(void) {}
  virtual void eventB(void) {}
};

struct StateDisconnected : Async::StateBase<StateTop, StateDisconnected>
{
  static constexpr auto NAME = "Disconnected";
  void eventA(void) override
  {
    ctx().a = 24;
    setState<StateConnected>();
  }
};

struct StateConnected : Async::StateBase<StateTop, StateConnected>
{
  static constexpr auto NAME = "Connected";
  void init(void) { setState<StateConnectedA>(); }
  void eventB(void) override { setState<StateConnectedB>(); }
};

struct StateConnectedA : Async::StateBase<StateConnected, StateConnectedA>
{
  static constexpr auto NAME = "ConnectedA";
};

struct StateConnectedB : Async::StateBase<StateConnected, StateConnectedB>
{
  static constexpr auto NAME = "ConnectedB";
};


void test_state_machine(void)
{
  cout << "test_state_machine" << endl;
  Context ctx;
  ctx.a = 42;
  Async::StateMachine<Context, StateTop> sm(&ctx);

  sm.start();
  check(sm.isActive<StateDisconnected>(),
        "starts in the default substate (Disconnected)");
  check(!sm.isActive<StateConnected>(), "not in Connected at start");

  sm.state().eventA();
  check(ctx.a == 24, "event handler mutated the context");
  check(!sm.isActive<StateDisconnected>(), "left Disconnected after eventA");
  check(sm.isActive<StateConnectedA>(),
        "entered Connected's default substate ConnectedA");

  // eventB is defined on the superstate StateConnected, not on ConnectedA.
  // Dispatching it while in ConnectedA exercises hierarchical event handling
  // (a substate inherits its superstate's event handlers).
  sm.state().eventB();
  check(sm.isActive<StateConnectedB>(),
        "superstate event handler (eventB) ran from substate -> ConnectedB");

  sm.setState<StateDisconnected>();
  check(sm.isActive<StateDisconnected>(),
        "explicit setState returned to Disconnected");
  check(!sm.isActive<StateConnected>(), "no longer in Connected");
}

} /* anonymous namespace */


int main(void)
{
  test_state_machine();

  cout << endl;
  if (failures == 0)
  {
    cout << "All StateMachine tests passed" << endl;
    return 0;
  }
  cout << failures << " StateMachine test check(s) FAILED" << endl;
  return 1;
}
