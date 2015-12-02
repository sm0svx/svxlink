/**
@file	 AsyncTimer.h
@brief   Contains a single shot or periodic timer that emits a signal on timeout
@author  Tobias Blomberg
@date	 2003-03-26

This file contains a class for creating a timer. Timers are a core
component in the event driven world. See usage instructions in the
class documentation.

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2003  Tobias Blomberg

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

/** @example AsyncTimer_demo.cpp
An example of how to use the Async::Timer class
*/



#ifndef ASYNC_TIMER_INCLUDED
#define ASYNC_TIMER_INCLUDED


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
@brief	A class that produces timer events
@author Tobias Blomberg
@date   2003-03-26

This class is used to create timer objects. These objects will emit a signal
when the specified time has elapsed. An example of how to use it is shown below.

\include AsyncTimer_demo.cpp
*/
class Timer : public sigc::trackable
{
  public:
    /**
     * @brief The type of the timer
     */
    typedef enum
    {
      TYPE_ONESHOT,   ///< A timer that expires once
      TYPE_PERIODIC   ///< A timer that restarts itself every time it expires
    } Type;
  
    /**
     * @brief 	Constructor
     * @param   timeout_ms  The timeout value in milliseconds
     * @param   type        The type of timer to use (see @ref Type)
     * @param   enabled     Set to \em false if the timer should be disabled
     *
     * If no arguments are given (default constructor) a timer that expires
     * immediately will be created. Such a timer can for example be used to
     * delay the execution of some function until all active callbacks have
     * returned.
     * If a negative timeout value is given, the timer will be disabled. The
     * timer must not be enabled until a timeout value equal or greater than
     * zero has been set.
     */
    Timer(int timeout_ms = 0, Type type = TYPE_ONESHOT, bool enabled=true);
    
    /**
     * @brief 	Destructor
     */
    ~Timer(void);
    
    /**
     * @brief 	Return the type of this timer
     * @return	Returns the type of this timer
     */
    Type type(void) const { return m_type; }
  
    /**
     * @brief 	Set (change) the timeout value
     * @param 	timeout_ms The new timeout value in milliseconds
     *
     * Use this function to set a new timeout value on an existing timer.
     * The timer will be reset so the timer will expire when the new timeout
     * time has elapsed. If the timer is disabled, this function will set the
     * new timeout value but it will not enable the timer.
     * If a negative timeout value is given, the timer will be disabled. The
     * timer must not be enabled until a timeout value equal or greater than
     * zero has been set.
     */
    void setTimeout(int timeout_ms);
  
    /**
     * @brief 	Return the setting of the timeout value
     * @return	Returns the timeout value in milliseconds
     */
    int timeout(void) const { return m_timeout_ms; }
  
    /**
     * @brief 	Enable or disable the timer
     * @param 	do_enable Set to \em true to enable the timer or \em false to
     *	      	      	  disable it
     *
     * This function will enable or disable an existing timer. A timer that has
     * a negative timeout value set must not be enabled.
     */
    void setEnable(bool do_enable);
  
    /**
     * @brief 	Check if the timer is enabled
     * @return	Returns \em true if the timer is enabled or \em false if it is
     *	      	disabled
     */
    bool isEnabled(void) const { return m_is_enabled; }
  
    /**
     * @brief 	Reset (restart) the timer
     *
     * This function is used to reset the timer. After reset it will take
     * \em timeout milliseconds before the timer expires, where \em timeout
     * is the previously set timeout value.
     * If the timer is disabled, this function will do nothing.
     */
    void reset(void);
    
    /**
     * @brief 	A signal that is emitted when the timer expires
     * @param 	timer A pointer to the timer that has expired
     *
     * This signal is emitted when the timer expires. It is perfectly legal
     * to delete the timer in the connected slot if it is known to be the
     * only connected slot.
     */
    sigc::signal<void, Timer *> expired;
    
    
  protected:
    
  private:
    Type  m_type;
    int   m_timeout_ms;
    bool  m_is_enabled;
  
};  /* class Timer */


} /* namespace */

#endif /* ASYNC_TIMER_INCLUDED */



/*
 * This file has not been truncated
 */

