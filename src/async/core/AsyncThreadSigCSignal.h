/**
@file   AsyncThreadSigCSignal.h
@brief  A_brief_description_for_this_file
@author Tobias Blomberg / SM0SVX
@date   2019-02-02

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

/** @example AsyncThreadSigCSignal_demo.cpp
An example of how to use the Async::ThreadSigCSignal class
*/


#ifndef ASYNC_THREAD_SIGC_SIGNAL_INCLUDED
#define ASYNC_THREAD_SIGC_SIGNAL_INCLUDED


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
@date   2019-02-02

A_detailed_class_description

\include AsyncThreadSigCSignal_demo.cpp
*/
template <typename T_ret, typename... Args>
class ThreadSigCSignal
{
  public:
    typedef T_ret                                   result_type;
    typedef sigc::slot<T_ret, Args...>              slot_type;
    typedef typename sigc::slot_list<slot_type>     slot_list_type;
    typedef typename sigc::signal<T_ret, Args...>   signal_type;
    typedef typename signal_type::iterator          iterator;
    typedef typename signal_type::const_iterator    const_iterator;

    ThreadSigCSignal(void)
    {
      int pipefd[2];
      if (pipe(pipefd) == -1)
      {
        std::cerr << "*** ERROR: Could not create pipe in "
                     "ThreadSigCSignal: "
                  << strerror(errno) << std::endl;
        abort();
      }
      m_rd_watch = new FdWatch(pipefd[0], FdWatch::FD_WATCH_RD);
      m_rd_watch->activity.connect(
          sigc::mem_fun(this, &ThreadSigCSignal::processQueue));
      m_wr_pipe = pipefd[1];
    }

    ThreadSigCSignal(const ThreadSigCSignal&) = delete;

    ~ThreadSigCSignal()
    {
      m_rd_watch->setEnabled(false);
      close(m_rd_watch->fd());
      delete m_rd_watch;
      m_rd_watch = 0;
      close(m_wr_pipe);
      m_wr_pipe = -1;
    }

    ThreadSigCSignal& operator=(const ThreadSigCSignal&) = delete;

    /**
     * @brief   A_brief_member_function_description
     * @param   param1 Description_of_param1
     * @return  Return_value_of_this_member_function
     */
    iterator connect(const slot_type& slt)
    {
      return m_sig.connect(slt);
    }

    result_type emit(Args... args)
    {
      return queueSignal(false, args...);
    }

    result_type emit_reverse(Args... args)
    {
      return queueSignal(true, args...);
    }

    result_type operator()(Args... args)
    {
      return queueSignal(false, args...);
    }

    slot_type make_slot(void)
    {
      return sigc::mem_fun(*this, &ThreadSigCSignal::emit);
    }

    slot_list_type slots(void)
    {
      return m_sig.slots();
    }

    const slot_list_type slots(void) const
    {
      return m_sig.slots();
    }

  private:
    sigc::signal<T_ret, Args...>  m_sig;
    std::list<sigc::slot<void>>   m_queue;
    std::mutex                    m_queue_mu;
    Async::FdWatch*               m_rd_watch = 0;
    int                           m_wr_pipe = -1;

    result_type queueSignal(bool reverse, Args... args)
    {
      {
        std::lock_guard<std::mutex> lk(m_queue_mu);
        if (reverse)
        {
          m_queue.push_back(sigc::bind(
                sigc::mem_fun(m_sig, &signal_type::emit_reverse), args...));
        }
        else
        {
          m_queue.push_back(sigc::bind(
                sigc::mem_fun(m_sig, &signal_type::emit), args...));
        }
      }
      if (write(m_wr_pipe, "", 1) == -1)
      {
        std::cerr << "*** ERROR: Could not write to pipe in "
                     "Async::ThreadsafeSigCConnector: "
                  << std::strerror(errno) << std::endl;
        abort();
      }
      return result_type();
    }

    void processQueue(FdWatch *w)
    {
      char buf[256];
      ssize_t cnt = read(w->fd(), buf, sizeof(buf));
      if (cnt == -1)
      {
        std::cerr << "*** ERROR: Could not read pipe in "
                     "ThreadSigCSignal::processQueue: "
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
};  /* class ThreadSigCSignal */


} /* namespace */

#endif /* ASYNC_THREAD_SIGC_SIGNAL_INCLUDED */


/*
 * This file has not been truncated
 */
