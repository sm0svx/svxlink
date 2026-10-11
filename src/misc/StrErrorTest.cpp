/**
@file    StrErrorTest.cpp
@brief   Unit test for SvxLink::strError() explicit errno handling
@author  Mark Rose
@date    2026-07-11

strError(int errnum) is supposed to report the message for the errno value
passed in by the caller. It used to ignore that argument and instead call
strerror_r() with the *global* errno, so a caller that saved an errno value
(e.g. after a failed syscall, once other code may have run and changed the
global errno) got the wrong message back. This test sets the global errno to
one value and asks strError() about a different value, verifying the message
returned matches the requested errno, not the global one.

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2026 Tobias Blomberg / SM0SVX

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.
\endverbatim
*/

#include <cerrno>
#include <cstring>
#include <iostream>
#include <string>

#include "common.h"

using namespace std;
using namespace SvxLink;

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
  string enoent_msg(strerror(ENOENT));
  string eacces_msg(strerror(EACCES));
  check(enoent_msg != eacces_msg,
        "sanity: ENOENT and EACCES have different messages on this system");

    // Leave a different errno value in the global variable than the one we
    // ask strError() about, mimicking a caller that saved an errno from an
    // earlier failed syscall while other code (which may touch errno) ran
    // in between.
  errno = EACCES;
  string requested = strError(ENOENT);
  check(requested == enoent_msg,
        "strError(ENOENT) reports ENOENT's message while global errno is "
        "EACCES");
  check(requested != eacces_msg,
        "strError(ENOENT) does not report the global errno's message");

    // And the reverse, to make sure it's not just coincidentally right.
  errno = ENOENT;
  string requested2 = strError(EACCES);
  check(requested2 == eacces_msg,
        "strError(EACCES) reports EACCES's message while global errno is "
        "ENOENT");
  check(requested2 != enoent_msg,
        "strError(EACCES) does not report the global errno's message");

  cout << endl;
  if (failures == 0)
  {
    cout << "All strError() tests passed" << endl;
    return 0;
  }
  cout << failures << " strError() test check(s) FAILED" << endl;
  return 1;
} /* main */
