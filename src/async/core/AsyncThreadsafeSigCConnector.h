/**
@file	 AsyncThreadsafeSigCConnnetor.h
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

/** @example AsyncThreadsafeSigCConnector_demo.cpp
An example of how to use the Async::ThreadsafeSigCConnector class
*/


#ifndef ASYNC_THREADSAFE_SIGC_CONNECTOR_INCLUDED
#define ASYNC_THREADSAFE_SIGC_CONNECTOR_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <iostream>
#include <cassert>
#include <mutex>
#include <sigc++/sigc++.h>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncMutex.h>


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

\include AsyncThreadsafeSigCConnector_demo.cpp
*/
template <typename RetT, typename... Args>
class ThreadsafeSigCConnector
{
  public:
    /**
     * @brief   Constructor
     * @param   sig The signal to connect
     * @param   slt The slot to connect the signal to
     */
    ThreadsafeSigCConnector(sigc::signal<RetT, Args...>& sig,
                            const sigc::slot<RetT, Args...>& slt)
      : m_slt(slt)
    {
      //std::cout << "ThreadsafeSigCConnector" << std::endl;
      mutex();
      assert(!m_slt.empty());
      sig.connect(sigc::mem_fun(*this, &ThreadsafeSigCConnector::sigHandler));
    }

    ThreadsafeSigCConnector(const ThreadsafeSigCConnector&) = delete;

    ~ThreadsafeSigCConnector()
    {
      //std::cout << "~ThreadsafeSigCConnector" << std::endl;
    }

    ThreadsafeSigCConnector& operator=(const ThreadsafeSigCConnector&) = delete;

    /**
     * @brief   A_brief_member_function_description
     * @param   param1 Description_of_param1
     * @return  Return_value_of_this_member_function
     */

  private:
    sigc::slot<RetT, Args...> m_slt;

    static Async::Mutex& mutex(void)
    {
      static Async::Mutex *mu = 0;
      if (mu == 0)
      {
        mu = new Async::Mutex;
        Async::Application::app().destroy.connect([]{
          //std::cout << "Mutex::lambda: destroy" << std::endl;
          delete mu;
          mu = 0;
        });
      }
      return *mu;
    }

    RetT sigHandler(Args... args)
    {
      std::lock_guard<Async::Mutex> lk(mutex());
      //std::cout << "ThreadsafeSigCConnection::sigHandler" << std::endl;
      assert(!m_slt.empty());
      return m_slt(args...);
    }
};  /* class ThreadsafeSigCConnector */


} /* namespace */

#endif /* ASYNC_THREADSAFE_SIGC_CONNECTOR_INCLUDED */


/*
 * This file has not been truncated
 */
