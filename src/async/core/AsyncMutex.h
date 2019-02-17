/**
@file   AsyncMutex.h
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

/** @example AsyncMutex_demo.cpp
An example of how to use the Async::Mutex class
*/

#ifndef ASYNC_MUTEX_INCLUDED
#define ASYNC_MUTEX_INCLUDED

/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <iostream>
#include <mutex>
#include <condition_variable>
#include <cassert>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncFdWatch.h>
#include <AsyncApplication.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Forward declarations
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Namespace
 *
 ****************************************************************************/

namespace Async
{


/****************************************************************************
 *
 * Forward declarations of classes inside of the declared namespace
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Defines & typedefs
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Exported Global Variables
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Class definitions
 *
 ****************************************************************************/

/**
@brief  Synchronize execution from a thread with the main Async thread
@author Tobias Blomberg / SM0SVX
@date   2019-01-19

This class is used to synchronize the execution of a thread with the main Async
thread so that it is safe to access shared resources. When a thread holds the
lock the main Async thread is guaranteed to be in a safe place. All other
threads trying to get the same lock will be blocked until the thread holding
the lock release it.

As always with locks, try to hold the lock the shortest time possible since it
will block the main Async thread while the lock is held. That for example means
that no timer handlers nor file descriptor watch handlers will be called

Note that locking this mutex is almost always expensive since the main Async
thread own the lock if no other thread owns it. The main thread must then get
to a safe place before the mutex can be locked by another thread. Only use this
mutex if synchronization with the main Async thread is needed. If the purpose
is to only synchronize between other threads, use a standard mutex. A standard
mutex is also sufficient if synchronizing access to some data structure that
you have full control over.

\include AsyncMutex_demo.cpp
*/
class Mutex
{
  public:
    /**
     * @brief Default constructor
     */
    Mutex(void) noexcept
      : m_main_thread(Async::Application::app().threadId()),
        m_lock_owner(m_main_thread)
    {
      assert((std::this_thread::get_id() == m_main_thread) &&
             "Async::Mutex objects must be created in the Async main thread");
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
      m_pipe_wr = pipefd[1];
      m_rd_watch = new FdWatch(pipefd[0], FdWatch::FD_WATCH_RD);
      m_rd_watch->activity.connect(sigc::mem_fun(*this, &Mutex::lockHandler));

      Async::Application::app().execDone.connect(
          sigc::mem_fun(*this, &Mutex::mainThreadDone));
    }

    /**
     * @brief Copy constructor is deleted
     */
    Mutex(const Mutex&) = delete;

    /**
     * @brief Destructor
     *
     * The behavior is undefined if the mutex is owned by any thread or if any
     * thread terminates while holding any ownership of the mutex.
     */
    ~Mutex(void)
    {
      assert((m_main_thread == std::this_thread::get_id())
          || (m_main_thread == std::thread::id()));
      assert(m_lock_owner == m_main_thread);
      int pipe_rd = m_rd_watch->fd();
      delete m_rd_watch;
      m_rd_watch = 0;
      close(m_pipe_wr);
      m_pipe_wr = -1;
      close(pipe_rd);
      pipe_rd = -1;
    }

    Mutex& operator=(const Mutex&) = delete;

    /**
     * @brief   Lock the mutex
     *
     * Calling this function will lock the mutex if it's not already locked by
     * another thread. If the lock is held by another thread this thread will
     * block until the lock can be acquired.
     * NOTE: Never call this function from the main Async thread.
     */
    void lock(void)
    {
      if (Async::Application::app().threadId() == std::this_thread::get_id())
      {
        return;
      }
      if (write(m_pipe_wr, "", 1) == -1)
      {
        std::cerr << "*** ERROR: Could not write to pipe in Async::Mutex::lock: "
                  << strerror(errno) << std::endl;
        exit(1);
      }
      std::unique_lock<std::mutex> lk(m_mu);
      m_cond.wait(lk,
          [this]{ return m_lock_owner == std::thread::id(); });
      m_lock_owner = std::this_thread::get_id();
    }

    //bool try_lock() { return m_mu.try_lock(); }

    /**
     * @brief   Unlock the mutex
     *
     * Unlock a previously locked mutex. It is not allowed to call this
     * function if the calling thread does not own the lock.
     * NOTE: Never call this function from the main Async thread.
     */
    void unlock(void)
    {
      if (Async::Application::app().threadId() == std::this_thread::get_id())
      {
        return;
      }
      std::unique_lock<std::mutex> lk(m_mu);
      assert(m_lock_owner == std::this_thread::get_id());
      m_lock_owner = m_main_thread;
      lk.unlock();
      m_cond.notify_one();
    }

  private:
    std::mutex                    m_mu;
    std::condition_variable       m_cond;
    int                           m_pipe_wr = -1;
    Async::FdWatch*               m_rd_watch = 0;
    std::thread::id               m_main_thread;
    std::thread::id               m_lock_owner;

    void lockHandler(FdWatch *w)
    {
      ssize_t cnt = -1;
      char buf[256];
      while ((cnt > 0) || ((cnt = read(w->fd(), buf, sizeof(buf))) > 0))
      {
        std::unique_lock<std::mutex> lk(m_mu);
        assert(m_lock_owner == m_main_thread);
        m_lock_owner = std::thread::id();
        lk.unlock();
        m_cond.notify_one();
        lk.lock();
        m_cond.wait(lk, [this]{ return m_lock_owner == m_main_thread; });
        cnt -= 1;
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
    }

    void mainThreadDone(void)
    {
      {
        std::lock_guard<std::mutex> lk(m_mu);
        assert(m_lock_owner == m_main_thread);
        m_lock_owner = m_main_thread = std::thread::id();
      }
      m_cond.notify_one();
    }

};  /* class Mutex */


} /* namespace */

#endif /* ASYNC_MUTEX_INCLUDED */

/*
 * This file has not been truncated
 */
