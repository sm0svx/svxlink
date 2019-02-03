/**
@file	 AsyncMutex.h
@brief   A_brief_description_for_this_file
@author  Tobias Blomberg / SM0SVX
@date	 2019-01-19

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

#include <mutex>
#include <condition_variable>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncFdWatch.h>


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
@brief	A_brief_class_description
@author Tobias Blomberg / SM0SVX
@date   2019-01-19

A_detailed_class_description

\include AsyncMutex_demo.cpp
*/
class Mutex
{
  public:
    /**
     * @brief Default constructor
     */
    Mutex(void) noexcept
      : m_main_thread(std::this_thread::get_id()), m_lock_owner(m_main_thread)
    {
      //std::cout << "Mutex::Mutex" << std::endl;
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
      m_rd_watch->activity.connect(sigc::mem_fun(*this, &Mutex::unlockHandler));

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
      //std::cout << "Mutex::~Mutex" << std::endl;
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
     * @brief 	A_brief_member_function_description
     * @param 	param1 Description_of_param1
     * @return	Return_value_of_this_member_function
     */
    void lock(void)
    {
      assert(m_main_thread != std::this_thread::get_id());
      if (write(m_pipe_wr, "", 1) == -1)
      {
        std::cerr << "*** ERROR: Could not write to pipe in Async::Mutex::lock: "
                  << strerror(errno) << std::endl;
        exit(1);
      }
      std::unique_lock<std::mutex> lk(m_mu);
      //std::cout << "### Mutex::lock: ENTER" << std::endl;
      m_cond.wait(lk,
          [this]{ return m_lock_owner == std::thread::id(); });
      m_lock_owner = std::this_thread::get_id();
      //std::cout << "### Mutex::lock: EXIT" << std::endl;
    }

    //bool try_lock() { return m_mu.try_lock(); }

    void unlock(void)
    {
      assert(m_main_thread != std::this_thread::get_id());
      std::unique_lock<std::mutex> lk(m_mu);
      //std::cout << "### Mutex::unlock: ENTER" << std::endl;
      assert(m_lock_owner == std::this_thread::get_id());
      m_lock_owner = m_main_thread;
      //std::cout << "### Mutex::unlock: EXIT" << std::endl;
      lk.unlock();
      m_cond.notify_one();
    }

  private:
    std::mutex                    m_mu;
    //std::condition_variable       m_lock_cond;
    std::condition_variable       m_cond;
    int                           m_pipe_wr = -1;
    Async::FdWatch*               m_rd_watch = 0;
    std::thread::id               m_main_thread;
    std::thread::id               m_lock_owner;

    void unlockHandler(FdWatch *w)
    {
      //std::cout << "### Mutex::unlockHandler: ENTER" << std::endl;
      ssize_t cnt = -1;
      char ch;
      while ((cnt = read(w->fd(), &ch, 1)) > 0)
      {
        std::unique_lock<std::mutex> lk(m_mu);
        if (m_lock_owner == m_main_thread)
        {
          m_lock_owner = std::thread::id();
          lk.unlock();
          m_cond.notify_one();
          lk.lock();
        }
        m_cond.wait(lk,
            [this]{ return m_lock_owner == m_main_thread; });
      }
      if (cnt == -1)
      {
        if ((errno != EAGAIN) && (errno != EWOULDBLOCK))
        {
          std::cerr << "*** ERROR: Could not read pipe in "
                       "Async::Mutex::unlockHandler: "
                    << strerror(errno) << std::endl;
          exit(1);
        }
      }
      //std::cout << "### Mutex::unlockHandler: EXIT" << std::endl;
    }

    void mainThreadDone(void)
    {
      {
        std::lock_guard<std::mutex> lk(m_mu);
        //std::cout << "mainThreadDone: EXIT" << std::endl;
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
