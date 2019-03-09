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

#include <thread>
#include <cassert>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncApplication.h>


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
std::thread::id         Async::Mutex::main_thread;
std::thread::id         Async::Mutex::lock_owner;
size_t                  Async::Mutex::lock_wait_cnt = 0;
bool                    Async::Mutex::lock_handler_pending = false;


/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/

Mutex::Mutex(void) noexcept
{
  assert((std::this_thread::get_id() == Application::app().threadId()) &&
         "Async::Mutex objects must be created in the Async main thread");

  if (main_thread == std::thread::id())
  {
    lock_owner = main_thread = Async::Application::app().threadId();
    Async::Application::app().execDone.connect(
        sigc::ptr_fun(&Mutex::mainThreadDone));
  }
} /* Mutex::Mutex */


Mutex::~Mutex(void)
{
  assert((std::this_thread::get_id() == Application::app().threadId()) &&
         "Async::Mutex objects must be destroyed in the Async main thread");
  assert(lock_owner == main_thread);
} /* Mutex::~Mutex */


void Mutex::lock(void)
{
  if (Async::Application::app().threadId() == std::this_thread::get_id())
  {
    return;
  }
  std::unique_lock<std::mutex> lk(mu);
  if (!lock_handler_pending)
  {
    lock_handler_pending = true;
    Application::app().runTask(&Mutex::lockHandler);
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

void Mutex::lockHandler(void)
{
  std::unique_lock<std::mutex> lk(mu);
  assert(lock_owner == main_thread);
  while (lock_wait_cnt > 0)
  {
    lock_owner = std::thread::id();
    lk.unlock();
    lock_available_cond.notify_one();
    lk.lock();
    no_locks_waiting_cond.wait(lk, []{ return lock_owner == main_thread; });
  }
  lock_handler_pending = false;
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
