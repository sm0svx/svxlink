/**
@file	 AsyncThreadSigCAsyncConnector.h
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

/** @example AsyncThreadSigCAsyncConnector_demo.cpp
An example of how to use the Async::ThreadSigCAsyncConnector class
*/


#ifndef ASYNC_THREAD_SIGC_ASYNC_CONNECTOR
#define ASYNC_THREAD_SIGC_ASYNC_CONNECTOR


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <unistd.h>
#include <iostream>
#include <cassert>
#include <cstring>
#include <mutex>
#include <list>
#include <sigc++/sigc++.h>


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
@brief  A_brief_class_description
@author Tobias Blomberg / SM0SVX
@date   2019-01-19

A_detailed_class_description

\include AsyncThreadSigCAsyncConnector_demo.cpp
*/
template <typename... Args>
class ThreadSigCAsyncConnector
{
  public:
    /**
     * @brief   Constructor
     * @param   sig The signal to connect
     * @param   slt The slot to connect the signal to
     */
    ThreadSigCAsyncConnector(sigc::signal<void, Args...>& sig,
                            const sigc::slot<void, Args...>& slt)
      : m_slt(slt)
    {
      //std::cout << "ThreadSigCAsyncConnector" << std::endl;
      assert(!m_slt.empty());
      sig.connect(sigc::mem_fun(*this, &ThreadSigCAsyncConnector::sigHandler));

      int pipefd[2];
      if (pipe(pipefd) == -1)
      {
        std::cerr << "*** ERROR: Could not create pipe in "
                     "ThreadSigCAsyncConnector: "
                  << strerror(errno) << std::endl;
        abort();
      }
      m_rd_watch = new FdWatch(pipefd[0], FdWatch::FD_WATCH_RD);
      m_rd_watch->activity.connect(
          sigc::mem_fun(this, &ThreadSigCAsyncConnector::processQueue));
      m_wr_pipe = pipefd[1];
    }

    ThreadSigCAsyncConnector(const ThreadSigCAsyncConnector&) = delete;

    ~ThreadSigCAsyncConnector()
    {
      //std::cout << "~ThreadSigCAsyncConnector" << std::endl;
      m_rd_watch->setEnabled(false);
      close(m_rd_watch->fd());
      delete m_rd_watch;
      m_rd_watch = 0;
      close(m_wr_pipe);
      m_wr_pipe = -1;
    }

    ThreadSigCAsyncConnector& operator=(const ThreadSigCAsyncConnector&) = delete;

    /**
     * @brief   A_brief_member_function_description
     * @param   param1 Description_of_param1
     * @return  Return_value_of_this_member_function
     */

  private:
    sigc::slot<void, Args...>   m_slt;
    std::list<sigc::slot<void>> m_queue;
    std::mutex                  m_queue_mu;
    Async::FdWatch*             m_rd_watch = 0;
    int                         m_wr_pipe = -1;

    void sigHandler(Args... args)
    {
      std::lock_guard<std::mutex> lk(m_queue_mu);
      //std::cout << "ThreadsafeSigCConnection::asyncSigHandler" << std::endl;
      m_queue.push_back(sigc::bind(m_slt, args...));
      if (write(m_wr_pipe, "", 1) == -1)
      {
        std::cerr << "*** ERROR: Could not write to pipe in "
                     "Async::ThreadsafeSigCConnector: "
                  << std::strerror(errno) << std::endl;
        abort();
      }
    }

    void processQueue(FdWatch *w)
    {
      char buf[256];
      ssize_t cnt = read(w->fd(), buf, sizeof(buf));
      if (cnt == -1)
      {
        std::cerr << "*** ERROR: Could not read pipe in "
                     "ThreadSigCAsyncConnector::processQueue: "
                  << std::strerror(errno) << std::endl;
        abort();
      }
      std::lock_guard<std::mutex> lk(m_queue_mu);
      assert(static_cast<ssize_t>(m_queue.size()) >= cnt);
      for (ssize_t i=0; i<cnt; ++i)
      {
        m_queue.front()();
        m_queue.pop_front();
      }
    }
};  /* class ThreadSigCAsyncConnector */


} /* namespace */

#endif /* ASYNC_THREAD_SIGC_ASYNC_CONNECTOR */


/*
 * This file has not been truncated
 */
