/**
@file    CmdParserTest.cpp
@brief   Unit tests for the CmdParser DTMF command dispatcher.
@author  Mark Rose
@date    2026-06-06

CmdParser matches a received command string against registered commands using
longest-prefix matching (unless a command requires an exact match), and invokes
the matching Command with the remaining characters as the sub command. These
tests exercise that logic directly; CmdParser has no Async/event-loop deps.

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

#include "CmdParser.h"

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

// A Command that records how many times it was invoked and the last sub
// command it received.
class RecCmd : public Command
{
  public:
    RecCmd(CmdParser* parser, const string& cmd, bool exact = false)
      : Command(parser, cmd)
    {
      if (exact)
      {
        setExactMatch(true);
      }
    }
    void operator()(const string& subcmd) override
    {
      ++calls;
      last_subcmd = subcmd;
    }
    int calls = 0;
    string last_subcmd;
};


void test_exact_dispatch_and_subcmd(void)
{
  cout << "test_exact_dispatch_and_subcmd" << endl;
  CmdParser p;
  RecCmd c12(&p, "12");
  check(c12.addToParser(), "add command '12'");

  check(p.processCmd("12"), "processCmd('12') matches");
  check(c12.calls == 1 && c12.last_subcmd == "", "invoked once, empty subcmd");

  check(p.processCmd("123"), "processCmd('123') matches '12'");
  check(c12.calls == 2 && c12.last_subcmd == "3", "subcmd '3' extracted");
}

void test_longest_prefix_wins(void)
{
  cout << "test_longest_prefix_wins" << endl;
  CmdParser p;
  RecCmd c1(&p, "1");
  RecCmd c12(&p, "12");
  c1.addToParser();
  c12.addToParser();

  check(p.processCmd("123"), "processCmd('123') matches");
  check(c12.calls == 1 && c1.calls == 0, "longest prefix '12' wins over '1'");
  check(c12.last_subcmd == "3", "subcmd '3'");

  check(p.processCmd("1"), "processCmd('1') matches");
  check(c1.calls == 1, "'1' dispatches to the short command");
}

void test_exact_match_blocks_subcmd(void)
{
  cout << "test_exact_match_blocks_subcmd" << endl;
  CmdParser p;
  RecCmd c12(&p, "12", /*exact=*/true);
  c12.addToParser();

  check(p.processCmd("12"), "exact command matches exact string");
  check(c12.calls == 1, "invoked for exact match");
  check(!p.processCmd("123"), "exact command does NOT match with trailing chars");
  check(c12.calls == 1, "not invoked for non-exact input");
}

void test_unknown_command(void)
{
  cout << "test_unknown_command" << endl;
  CmdParser p;
  RecCmd c12(&p, "12");
  c12.addToParser();
  check(!p.processCmd("99"), "unknown command returns false");
  check(c12.calls == 0, "no command invoked");
}

void test_add_remove_lifecycle(void)
{
  cout << "test_add_remove_lifecycle" << endl;
  CmdParser p;
  RecCmd c12(&p, "12");
  check(c12.addToParser(), "first add succeeds");
  RecCmd dup(&p, "12");
  check(!dup.addToParser(), "duplicate command string rejected");

  check(c12.removeFromParser(), "remove succeeds");
  check(!p.processCmd("12"), "removed command no longer matches");
  check(!c12.removeFromParser(), "removing again returns false");
}

// Records handleCmd signal emissions.
class SignalRecorder : public sigc::trackable
{
  public:
    void onHandle(Command*, const string& subcmd)
    {
      ++calls;
      last_subcmd = subcmd;
    }
    int calls = 0;
    string last_subcmd;
};

void test_handle_cmd_signal(void)
{
  cout << "test_handle_cmd_signal" << endl;
  CmdParser p;
  // A plain Command (operator() not overridden) emits handleCmd.
  Command c(&p, "55");
  c.addToParser();
  SignalRecorder rec;
  c.handleCmd.connect(sigc::mem_fun(rec, &SignalRecorder::onHandle));
  check(p.processCmd("557"), "processCmd('557')");
  check(rec.calls == 1 && rec.last_subcmd == "7",
        "handleCmd signal fired with subcmd '7'");
}

} /* anonymous namespace */


int main(void)
{
  test_exact_dispatch_and_subcmd();
  test_longest_prefix_wins();
  test_exact_match_blocks_subcmd();
  test_unknown_command();
  test_add_remove_lifecycle();
  test_handle_cmd_signal();

  cout << endl;
  if (failures == 0)
  {
    cout << "All CmdParser tests passed" << endl;
    return 0;
  }
  cout << failures << " CmdParser test check(s) FAILED" << endl;
  return 1;
}
