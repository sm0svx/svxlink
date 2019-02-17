/**
@file   AsyncMutex.cpp
@brief  A mutex for synchronizing threads with the main Async thread
@author Tobias Blomberg / SM0SVX
@date   2019-01-19

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2003-2019 Tobias Blomberg / SM0SVX

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
#include <fcntl.h>
#include <thread>
#include <cassert>
#include <cstring>
#include <iostream>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncApplication.h>
#include <AsyncFdWatch.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "AsyncMutex.h"


/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/

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

std::mutex              Async::Mutex::mu;
std::condition_variable Async::Mutex::lock_available_cond;
std::condition_variable Async::Mutex::no_locks_waiting_cond;
int                     Async::Mutex::pipe_wr = -1;
Async::FdWatch*         Async::Mutex::rd_watch = 0;
std::thread::id         Async::Mutex::main_thread;
std::thread::id         Async::Mutex::lock_owner;
size_t                  Async::Mutex::instance_cnt = 0;
size_t                  Async::Mutex::lock_wait_cnt = 0;


/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/

Mutex::Mutex(void) noexcept
{
  assert((std::this_thread::get_id() == Application::app().threadId()) &&
         "Async::Mutex objects must be created in the Async main thread");
  if (pipe_wr == -1)
  {
    lock_owner = main_thread = Async::Application::app().threadId();
    int pipefd[2];
    if (pipe(pipefd) != 0)
    {
      std::cerr << "*** ERROR: Failed to create pipe: "
                << std::strerror(errno) << std::endl;
      exit(1);
    }
    if (fcntl(pipefd[0], F_SETFL, O_NONBLOCK) == -1)
    {
      std::cerr << "*** ERROR: Failed to set pipe to non-blocking "
                   "in Mutex::Mutex: "
                << std::strerror(errno) << std::endl;
      exit(1);
    }
    pipe_wr = pipefd[1];
    rd_watch = new FdWatch(pipefd[0], FdWatch::FD_WATCH_RD);
    rd_watch->activity.connect(sigc::ptr_fun(&Mutex::lockHandler));

    Async::Application::app().execDone.connect(
        sigc::ptr_fun(&Mutex::mainThreadDone));
  }
  instance_cnt += 1;
} /* Mutex::Mutex */


Mutex::~Mutex(void)
{
  assert((std::this_thread::get_id() == Application::app().threadId()) &&
         "Async::Mutex objects must be destroyed in the Async main thread");
  assert(lock_owner == main_thread);
  if (--instance_cnt == 0)
  {
    int pipe_rd = rd_watch->fd();
    delete rd_watch;
    rd_watch = 0;
    close(pipe_wr);
    pipe_wr = -1;
    close(pipe_rd);
    pipe_rd = -1;
  }
} /* Mutex::~Mutex */


void Mutex::lock(void)
{
  if (Async::Application::app().threadId() == std::this_thread::get_id())
  {
    return;
  }
  std::unique_lock<std::mutex> lk(mu);
  if (write(pipe_wr, "", 1) == -1)
  {
    std::cerr << "*** ERROR: Could not write to pipe in Async::Mutex::lock: "
              << strerror(errno) << std::endl;
    exit(1);
  }
  lock_wait_cnt += 1;
  lock_available_cond.wait(lk, []{ return lock_owner == std::thread::id(); });
  lock_wait_cnt -= 1;
  lock_owner = std::this_thread::get_id();
} /* Mutex::lock */


void Mutex::unlock(void)
{
  if (Async::Application::app().threadId() == std::this_thread::get_id())
  {
    return;
  }
  std::unique_lock<std::mutex> lk(mu);
  assert(lock_owner == std::this_thread::get_id());
  if (lock_wait_cnt > 0)
  {
    lock_owner = std::thread::id();
    lk.unlock();
    lock_available_cond.notify_one();
  }
  else
  {
    lock_owner = main_thread;
    lk.unlock();
    no_locks_waiting_cond.notify_one();
  }
} /* Mutex::unlock */


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

void Mutex::lockHandler(FdWatch *w)
{
  char buf[256];
  ssize_t cnt = -1;
  while ((cnt = read(w->fd(), buf, sizeof(buf))) > 0)
  {
    std::unique_lock<std::mutex> lk(mu);
    assert(lock_owner == main_thread);
    if (lock_wait_cnt > 0)
    {
      lock_owner = std::thread::id();
      lk.unlock();
      lock_available_cond.notify_one();
      lk.lock();
      no_locks_waiting_cond.wait(lk, []{ return lock_owner == main_thread; });
    }
  }
  if (cnt == -1)
  {
    if ((errno != EAGAIN) && (errno != EWOULDBLOCK))
    {
      std::cerr << "*** ERROR: Could not read pipe in "
                   "Async::Mutex::lockHandler: "
                << strerror(errno) << std::endl;
      exit(1);
    }
  }
} /* Mutex::lockHandler */


void Mutex::mainThreadDone(void)
{
  assert((std::this_thread::get_id() == Application::app().threadId()) &&
         "Async::Mutex::mainThreadDone not called from Async main thread");
  {
    std::lock_guard<std::mutex> lk(mu);
    assert(lock_owner == main_thread);
    lock_owner = main_thread = std::thread::id();
  }
  lock_available_cond.notify_one();
} /* Mutex::mainThreadDone */


/*
 * This file has not been truncated
 */
