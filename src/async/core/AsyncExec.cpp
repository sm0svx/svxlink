/**
@file	 AsyncExec.cpp
@brief   Execute external commands
@author  Tobias Blomberg / SM0SVX
@date	 2013-10-26

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2003-2013 Tobias Blomberg / SM0SVX

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
\endverbatim
*/



/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <sys/types.h>

#include <cstring>
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <sstream>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncTimer.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "AsyncExec.h"



/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/

using namespace std;
using namespace Async;



/****************************************************************************
 *
 * Defines & typedefs
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Local class definitions
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Prototypes
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Exported Global Variables
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Local Global Variables
 *
 ****************************************************************************/

Exec::ExecMap     Exec::execs;
int               Exec::sigchld_pipe[2] = {-1, -1};
Async::FdWatch    *Exec::sigchld_watch = 0;
struct sigaction  Exec::old_sigact;


/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/

Exec::Exec(const std::string &cmd)
  : pid(-1), stdout_watch(0), stderr_watch(0), stdin_fd(-1),
    status(0), nice_value(0), timeout_timer(0), pending_term(false)
{
  setCommandLine(cmd);

    // Set up SIGCHLD signal handling on first usage of the class
  if (sigchld_watch == 0)
  {
    int ret = pipe(sigchld_pipe);
    if (ret == -1)
    {
      cerr << "*** ERROR: Could not set up SIGCHLD pipe for Async::Exec: "
        << strerror(errno) << endl;
      exit(1);
    }
    sigchld_watch = new FdWatch(sigchld_pipe[0], FdWatch::FD_WATCH_RD);
    sigchld_watch->activity.connect(
        sigc::hide(sigc::ptr_fun(Exec::sigchldReceived)));

    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_sigaction = &handleSigChld;
    act.sa_flags = SA_RESTART|SA_NOCLDSTOP|SA_SIGINFO;
    if (sigaction(SIGCHLD, &act, &old_sigact) == -1)
    {
      cout << "*** ERROR: Could not set up SIGCHLD signal handler\n";
      exit(1);
    }
    /*
    cout << "### handler=" << old_sigact.sa_handler
        //<< " mask=" << old_sigact.sa_mask
      << " flags=" << old_sigact.sa_flags
      << " sigaction=" << old_sigact.sa_sigaction
      << endl;
    */
  }
} /* Exec::Exec */


Exec::~Exec(void)
{
  ExecMap::iterator it = execs.find(pid);
  if (it != execs.end())
  {
    execs.erase(pid);
  }

  if (stdin_fd != -1)
  {
    close(stdin_fd);
  }

  if (stdout_watch != 0)
  {
    close(stdout_watch->fd());
    delete stdout_watch;
  }

  if (stderr_watch != 0)
  {
    close(stderr_watch->fd());
    delete stderr_watch;
  }

  delete timeout_timer;
} /* Exec::~Exec */


void Exec::setCommandLine(const std::string &cmd)
{
  args.clear();
  stringstream ss(cmd);
  string part;
  while (ss >> part)
  {
    args.push_back(part);
  }
} /* Exec::setCommandLine */


void Exec::appendArgument(const std::string &arg)
{
  args.push_back(arg);
} /* Exec::appendArgument */


void Exec::clearEnvironment(void)
{
  clear_env = true;
  env.clear();
} /* Exec::clearEnvironment */


void Exec::addEnvironmentVar(const std::string& name, const std::string& val)
{
  env.push_back(name + "=" + val);
} /* Exec::addEnvironmentVar */


void Exec::addEnvironmentVars(const Environment& env)
{
  for (const auto& e : env)
  {
    addEnvironmentVar(e.first, e.second);
  }
} /* Exec::addEnvironmentVars */


bool Exec::nice(int inc)
{
  nice_value += inc;
  if (pid > 0)
  {
    if (setpriority(PRIO_PROCESS, pid, nice_value) == -1)
    {
      cerr << "*** WARNING: Could not set \"nice\" value for process "
           << args[0] << ": " << strerror(errno) << endl;
      return false;
    }
  }
  return true;
} /* Exec::nice */


void Exec::setTimeout(int time_s)
{
  delete timeout_timer;
  timeout_timer = new Timer(1000 * time_s);
  timeout_timer->expired.connect(hide(mem_fun(*this, &Exec::handleTimeout)));
  timeout_timer->setEnable(pid > 0);
} /* Exec::setTimeout */


bool Exec::run(void)
{
    // Create pipe file descriptor pair for handling stdin to subprocess
  int in_filedes[2];
  int ret = pipe(in_filedes);
  if (ret == -1)
  {
    cerr << "*** ERROR: Could not set up stdin pipe for subprocess "
         << args[0] << ": " << strerror(errno) << endl;
    return false;
  }

    // Create pipe file descriptor pair for handling stdout from subprocess
  int out_filedes[2];
  ret = pipe(out_filedes);
  if (ret == -1)
  {
    cerr << "*** ERROR: Could not set up stdout pipe for subprocess "
         << args[0] << ": " << strerror(errno) << endl;
    return false;
  }

    // Create pipe file descriptor pair for handling stderr from subprocess
  int err_filedes[2];
  ret = pipe(err_filedes);
  if (ret == -1)
  {
    cerr << "*** ERROR: Could not set up stderr pipe for subprocess "
         << args[0] << ": " << strerror(errno) << endl;
    return false;
  }

  pid = fork();
  if (pid == -1)
  {
    cerr << "*** ERROR: The fork system call failed for subprocess "
         << args[0] << ": " << strerror(errno) << endl;
    return false;
  }

    // If pid is non-zero we're in the parent process
  if (pid != 0)
  {
      // Set up priority for child if specified
    if (nice_value != 0)
    {
      nice(0);
    }

      // Add the new child to the global map
    execs[pid] = this;

      // Set up handling for subprocess stdin
    close(in_filedes[0]);
    stdin_fd = in_filedes[1];

      // Set up handling for subprocess stdout
    close(out_filedes[1]);
    stdout_watch = new FdWatch(out_filedes[0], FdWatch::FD_WATCH_RD);
    stdout_watch->activity.connect(mem_fun(*this, &Exec::stdoutActivity));

      // Set up handling for subprocess stderr
    close(err_filedes[1]);
    stderr_watch = new FdWatch(err_filedes[0], FdWatch::FD_WATCH_RD);
    stderr_watch->activity.connect(mem_fun(*this, &Exec::stderrActivity));

    if (timeout_timer != 0)
    {
      timeout_timer->setEnable(true);
    }

    return true;
  }

  /* From this point we are in the child process */

    // Set up stdin pipe
  close(in_filedes[1]);
  dup2(in_filedes[0], STDIN_FILENO);
  close(in_filedes[0]);

    // Set up stdout pipe
  close(out_filedes[0]);
  dup2(out_filedes[1], STDOUT_FILENO);
  close(out_filedes[1]);

    // Set up stderr pipe
  close(err_filedes[0]);
  dup2(err_filedes[1], STDERR_FILENO);
  close(err_filedes[1]);

  char *cmd[args.size()+1];
  for (size_t i=0; i<args.size(); ++i)
  {
    cmd[i] = strdup(args[i].c_str());
  }
  cmd[args.size()] = 0;

  if (clear_env)
  {
    clearenv();
  }
  for (size_t i=0; i<env.size(); ++i)
  {
    putenv(strdup(env[i].c_str()));
  }

  ret = execv(cmd[0], cmd);
  assert (ret == -1);

  cerr << "*** ERROR: Failed to exec " << args[0]
       << ": " << strerror(errno) << endl;

  exit(255);
} /* Exec::run */


bool Exec::writeStdin(const char *buf, int cnt)
{
  int ret = write(stdin_fd, buf, cnt);
  if (ret < 0)
  {
    cerr << "*** ERROR: Could not write to stdin pipe for subprocess "
         << args[0] << ": " << strerror(errno) << endl;
    return false;
  }
  return true;
} /* Exec::writeStdin */


bool Exec::writeStdin(const std::string &str)
{
  if (str.empty())
  {
    return true;
  }
  return writeStdin(str.c_str(), str.size());
} /* Exec::writeStdin */


bool Exec::kill(int sig)
{
  if (pid <= 0)
  {
    return false;
  }

  if (::kill(pid, sig) == -1)
  {
    cerr << "*** ERROR: Could not send signal " << sig << " to process "
         << args[0] << ": " << strerror(errno) << endl;
    return false;
  }
  return true;
} /* Exec::kill */


bool Exec::closeStdin(void)
{
  return (close(stdin_fd) == 0);
} /* Exec::closeStdin */


bool Exec::ifExited(void) const
{
  return WIFEXITED(status);
} /* Exec::ifExited */


bool Exec::ifSignaled(void) const
{
  return WIFSIGNALED(status);
} /* Exec::ifSignaled */


int Exec::exitStatus(void) const
{
  return WEXITSTATUS(status);
} /* Exec::exitStatus */


int Exec::termSig(void) const
{
  return WTERMSIG(status);
} /* Exec::termSig */



/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/

void Exec::stdoutActivity(Async::FdWatch *w)
{
  char buf[4096];
  int cnt = read(w->fd(), buf, sizeof(buf)-1);
  if (cnt < 0)
  {
    cerr << "*** ERROR: Could not read subprocess stdout pipe: "
         << strerror(errno) << endl;
    return;
  }
  else if (cnt == 0)
  {
    w->setEnabled(false);
    stdoutClosed();
    return;
  }
  buf[cnt] = '\0';
  stdoutData(buf, cnt);
} /* Exec::stdoutActivity */


void Exec::stderrActivity(Async::FdWatch *w)
{
  char buf[4096];
  int cnt = read(w->fd(), buf, sizeof(buf)-1);
  if (cnt < 0)
  {
    cerr << "*** ERROR: Could not read subprocess stderr pipe: "
         << strerror(errno) << endl;
    return;
  }
  else if (cnt == 0)
  {
    w->setEnabled(false);
    stderrClosed();
    return;
  }
  buf[cnt] = '\0';
  stderrData(buf, cnt);
} /* Exec::stderrActivity */


void Exec::handleSigChld(int signal_number, siginfo_t *info, void *context)
{
    // Tell the SIGCHLD handler that we have received a signal
  if (write(sigchld_pipe[1], "C", 1) == -1)
  {
    cerr << "*** ERROR: Could not write SIGCHLD notification to pipe\n";
  }

    // Call the old signal handler if there was one registered
  if ((old_sigact.sa_flags & SA_SIGINFO) && (old_sigact.sa_sigaction != 0))
  {
    old_sigact.sa_sigaction(signal_number, info, context);
  }
  else if (old_sigact.sa_handler != 0)
  {
    old_sigact.sa_handler(signal_number);
  }
} /* Exec::handleSigChld */


void Exec::subprocessExited(void)
{
  execs.erase(pid);
  pid = -1;
  delete timeout_timer;
  timeout_timer = 0;
  exited();
} /* Exec::subprocessExited */


void Exec::handleTimeout(void)
{
  if (!pending_term)
  {
    cerr << "*** WARNING: The process " << args[0] << " have been running for "
            "too long. Sending the SIGTERM signal to it\n";
    kill(SIGTERM);
    pending_term = true;
    timeout_timer->setTimeout(10000);
  }
  else
  {
    cerr << "*** WARNING: The process " << args[0] << " could not be "
            "terminated using the SIGTERM signal. Sending the SIGKILL "
            "signal to it\n";
    kill(SIGKILL);
  }
} /* Exec::handleTimeout */


void Exec::sigchldReceived(void)
{
  char buf[1];
  int err = read(sigchld_pipe[0], buf, 1);
  if (err == -1)
  {
    cout << "*** WARNING: Error while reading SIGCHLD pipe: "
         << strerror(errno) << endl;
  }
  else if (err != 1)
  {
    cout << "*** WARNING: Unexpected read of size " << err 
         << " from SIGCHLD pipe\n";
  }

  //cout << "### sigchldReceived\n";
  ExecMap::iterator it = execs.begin();
  while (it != execs.end())
  {
    ExecMap::iterator next = it;
    ++next;
    pid_t pid = it->first;
    Exec *exec = it->second;
    int status = 0;
    pid_t ret = waitpid(pid, &status, WNOHANG);
    if (ret == -1)
    {
      cout << "*** ERROR: Could not poll status of process " << exec->command()
           << ": " << strerror(errno) << endl;
    }
    //cout << "pid=" << pid << " ret=" << ret << " status=" << status << endl;
    exec->status = status;
    if (ret == pid)
    {
      exec->subprocessExited();
    }
    it = next;
  }
} /* Exec::sigchldReceived */



/*
 * This file has not been truncated
 */

