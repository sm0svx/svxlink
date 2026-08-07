/**
@file    AsyncExecTest.cpp
@brief   Unit test for Async::Exec fd-leak and exec-safety fixes
@author  Mark Rose
@date    2026-07-11

Exercises three defects fixed together in Async::Exec:

 1. run() used to proceed with an empty command line, leading the child
    process to execv() a NULL path and the parent to read args[0] on an
    empty vector. run() must now reject an empty command line and return
    false without forking.

 2. closeStdin() used to close(stdin_fd) without resetting stdin_fd to -1,
    so a second call (or the destructor) would close the same fd number
    again. Once a subsequent open/pipe/dup recycles that fd number, this
    second close() silently closes an unrelated file descriptor. This test
    calls closeStdin() twice and requires the second call to be a no-op
    that still reports success, proving stdin_fd was reset.

 3. run() used to leak the stdin (and stdout) pipe file descriptors it had
    already opened whenever a later pipe() call failed. This test lowers
    RLIMIT_NOFILE so that the stdin pipe succeeds but the stdout pipe fails,
    then verifies the fd budget freed up by the failed run() is available
    again -- i.e. the stdin pipe's fds were closed rather than leaked.

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
#include <unistd.h>
#include <sys/resource.h>

#include <iostream>
#include <string>

#include <AsyncCppApplication.h>
#include <AsyncExec.h>

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

  // Return the fd number that the next open() call would hand out. Per
  // POSIX, open() always returns the lowest-numbered fd not currently open
  // for the process, so this is deterministic.
int nextFreeFd(void)
{
  int fd = open("/dev/null", O_RDONLY);
  if (fd >= 0)
  {
    close(fd);
  }
  return fd;
}

} /* anonymous namespace */


int main(void)
{
  CppApplication app;

    // --- Defect 1: empty command line must be rejected, not forked ---
  {
    Exec e("");
    check(!e.run(), "run() rejects an empty command line");
  }

    // --- Defect 2: closeStdin() must be idempotent ---
  {
    Exec e("/no/such/command-for-async-exec-test");
    check(e.run(), "run() succeeds (fork only, exec failure is async)");
    check(e.closeStdin(), "first closeStdin() closes the pipe");
    check(e.closeStdin(),
          "second closeStdin() is a no-op that still reports success "
          "(stdin_fd was reset to -1)");
    e.kill(SIGKILL);
  }

    // --- Defect 3: a failed run() must not leak the fds it already
    //     opened before the failure ---
  {
    struct rlimit orig_rl;
    check(getrlimit(RLIMIT_NOFILE, &orig_rl) == 0, "getrlimit succeeds");

    int free_fd = nextFreeFd();
    check(free_fd >= 0, "could determine next free fd");

      // Allow exactly two more fds to be opened (one pipe's worth). The
      // stdin pipe in run() will consume them; the following stdout pipe
      // will then fail with EMFILE.
    struct rlimit limited_rl = orig_rl;
    limited_rl.rlim_cur = free_fd + 2;
    check(setrlimit(RLIMIT_NOFILE, &limited_rl) == 0,
          "setrlimit lowers the fd budget");

    Exec e("/no/such/command-for-async-exec-test-2");
    bool run_result = e.run();

      // Restore the fd budget before making any more assertions so the
      // test process itself doesn't get wedged if something goes wrong.
    setrlimit(RLIMIT_NOFILE, &orig_rl);

    check(!run_result,
          "run() fails when the second pipe() call hits the fd limit");

      // If the stdin pipe's fds were leaked, "free_fd" and "free_fd+1" are
      // still in use, so the next allocation would be free_fd+2 or higher.
      // With the fix, run() closed them on the failure path, so the next
      // free fd is back to what it was before run() was called.
    int free_fd_after = nextFreeFd();
    check(free_fd_after == free_fd,
          "failed run() releases the pipe fds it had already opened "
          "(no fd leak)");
  }

  cout << (failures == 0 ? "ALL TESTS PASSED" : "TESTS FAILED") << endl;
  return failures != 0;
} /* main */
