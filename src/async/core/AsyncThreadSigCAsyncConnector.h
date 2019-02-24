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

#include <iostream>
#include <cassert>
#include <mutex>
#include <deque>
#include <sigc++/sigc++.h>


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
     * @param   synchronous Set to \em true to call the signal directly
     */
    ThreadSigCAsyncConnector(sigc::signal<void, Args...>& sig,
                             const sigc::slot<void, Args...>& slt,
                             bool synchronous=false)
      : m_slt(slt), m_synchronous(synchronous)
    {
      assert(!m_slt.empty());
      sig.connect(sigc::mem_fun(*this, &ThreadSigCAsyncConnector::sigHandler));
    }

    ThreadSigCAsyncConnector(const ThreadSigCAsyncConnector&) = delete;

    ~ThreadSigCAsyncConnector(void) {}

    ThreadSigCAsyncConnector& operator=(const ThreadSigCAsyncConnector&) = delete;

    /**
     * @brief   A_brief_member_function_description
     * @param   param1 Description_of_param1
     * @return  Return_value_of_this_member_function
     */

  private:
    sigc::slot<void, Args...>     m_slt;
    std::deque<sigc::slot<void>>  m_queue;
    std::mutex                    m_queue_mu;
    bool                          m_synchronous = false;

    void sigHandler(Args... args)
    {
      std::lock_guard<std::mutex> lk(m_queue_mu);
      m_queue.push_back(sigc::bind(m_slt, args...));
      Application::app().runTask(sigc::mem_fun(*this, &ThreadSigCAsyncConnector::processQueue));
    }

    void processQueue(void)
    {
      std::unique_lock<std::mutex> lk(m_queue_mu);
      while (!m_queue.empty())
      {
        lk.unlock();
        m_queue.front()();
        lk.lock();
        m_queue.pop_front();
      }
    }
};  /* class ThreadSigCAsyncConnector */


} /* namespace */

#endif /* ASYNC_THREAD_SIGC_ASYNC_CONNECTOR */


/*
 * This file has not been truncated
 */
