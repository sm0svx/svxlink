/**
@file   AsyncThreadSigCSignal.h
@brief  A threadsafe SigC signal
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
@brief  A threadsafe SigC signal
@author Tobias Blomberg / SM0SVX
@date   2019-02-02

This class is implemented as close as possible to mimic a sigc::signal but with
thread safety in mind. When the signal is emitted, instead of calling it
directly, it will be added to a queue. The queue is then processed from the
Async main thread in a safe manner so that all connected slots are called from
the main thread.

If the signal is emitted from the main Async thread it will still be queued so
this may be used if a signal emission need to be delayed so that is is called
from the main Async loop.

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

    /**
     * @brief   Default constructor
     */
    ThreadSigCSignal(void) {}

    /**
     * @brief   The copy constructor is deleted
     */
    ThreadSigCSignal(const ThreadSigCSignal&) = delete;

    /**
     * @brief   The assignment operator is deleted
     */
    ThreadSigCSignal& operator=(const ThreadSigCSignal&) = delete;

    /**
     * @brief   Connect this signal to the given slot
     * @param   slt The slot to connect to
     * @return  Returns a sigc::connection object
     */
    iterator connect(const slot_type& slt)
    {
      return m_sig.connect(slt);
    }

    /**
     * @brief   Queue the emission of this signal
     * @param   arg Zero or more arguments depending on the signal declaration
     * @return  Always return the default value of the specified return type
     *
     * This function will add a signal emission to the emission queue. Since
     * the emission is delayed there is no way of returning the return value
     * from the slot(s). This function will instead return the default value of
     * the specified return type.
     */
    result_type emit(Args... args)
    {
      return queueSignal(false, args...);
    }

    /**
     * @brief   Queue the emission of this signal in reverse order
     * @param   arg Zero or more arguments depending on the signal declaration
     * @return  Always return the default value of the specified return type
     *
     * This function will add a signal emission to the emission queue. Since
     * the emission is delayed there is no way of returning the return value
     * from the slot(s). This function will instead return the default value of
     * the specified return type.
     * The connected slots will be called in the reverse order in comparison to
     * the order they were added.
     */
    result_type emit_reverse(Args... args)
    {
      return queueSignal(true, args...);
    }

    /**
     * @brief   Queue the emission of this signal
     * @param   arg Zero or more arguments depending on the signal declaration
     * @return  Always return the default value of the specified return type
     *
     * This function will add a signal emission to the emission queue. Since
     * the emission is delayed there is no way of returning the return value
     * from the slot(s). This function will instead return the default value of
     * the specified return type.
     */
    result_type operator()(Args... args)
    {
      return queueSignal(false, args...);
    }

    /**
     * @brief   Return a slot that emits this signal
     * @return  A new slot is returned
     */
    slot_type make_slot(void)
    {
      return sigc::mem_fun(*this, &ThreadSigCSignal::emit);
    }

    /**
     * @brief   Return a list of the connected slots
     * @return  The slot list is returned
     */
    slot_list_type slots(void)
    {
      return m_sig.slots();
    }

    /**
     * @brief   Return a list of the connected slots
     * @return  The slot list is returned
     */
    const slot_list_type slots(void) const
    {
      return m_sig.slots();
    }

  private:
    sigc::signal<T_ret, Args...>  m_sig;

    result_type queueSignal(bool reverse, Args... args)
    {
      if (reverse)
      {
        Application::app().runTask(sigc::bind(
              sigc::mem_fun(m_sig, &signal_type::emit_reverse), args...));
      }
      else
      {
        Application::app().runTask(sigc::bind(
              sigc::mem_fun(m_sig, &signal_type::emit), args...));
      }
      return result_type();
    }
};  /* class ThreadSigCSignal */


} /* namespace */

#endif /* ASYNC_THREAD_SIGC_SIGNAL_INCLUDED */

/*
 * This file has not been truncated
 */
