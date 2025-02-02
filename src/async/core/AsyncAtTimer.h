/**
@file	 AsyncAtTimer.h
@brief   A timer that times out at a specified absolute time
@author  Tobias Blomberg / SM0SVX
@date	 2013-04-06

This class is used to get a timeout at a specified absolute time. That is,
you can specify a time of day, like 2013-04-06 12:43:00, when you would
like the timer to expire.

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2003-2013 Tobias Blomberg / SM0SVX

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

/** @example AsyncAtTimer_demo.cpp
An example of how to use the AsyncAtTimer class
*/


#ifndef ASYNC_AT_TIMER_INCLUDED
#define ASYNC_AT_TIMER_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <time.h>
#include <sys/time.h>
#include <sigc++/sigc++.h>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncTimer.h>


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
@brief	A timer that times out at a specified absolute time
@author Tobias Blomberg / SM0SVX
@date   2013-04-06

This class is used to get a timeout at a specified absolute time. That is,
you can specify a time of day, like 2013-04-06 12:43:00, when you would
like the timer to expire.

This class use the gettimeofday() function as its time reference. If reading
time using another function, like time(), in the expire callback, you can
not be sure to get the same time value. The gettimeofday() and time()
functions may return different values for the second. The offset usually seem
to be small (~10ms) but this has not been tested very much. One way to get
around the problem, if it's not possible to use the gettimeofday() function,
is to set an offset using the setExpireOffset() method. An offset of 100ms
will probably do.

\include AsyncAtTimer_demo.cpp
*/
class AtTimer : public sigc::trackable
{
  public:
    /**
     * @brief   Default constructor
     *
     * After default construction the timer will be disabled.
     */
    AtTimer(void);

    /**
     * @brief 	Constuctor
     * @param 	tm When the timer should expire in local time
     * @param   do_start Set to \em true (default) if the timer should start
     *                   upon creation
     */
    explicit AtTimer(struct tm &tm, bool do_start=true);
  
    /**
     * @brief 	Destructor
     */
    ~AtTimer(void);
  
    /**
     * @brief 	Set the timeout time
     * @param 	t When the timer should expire in seconds since the epoch
     * @return	Returns \em true on success or else \em false
     */
    bool setTimeout(time_t t);

    /**
     * @brief 	Set the timeout time
     * @param 	tm When the timer should expire in broken down local time
     * @return	Returns \em true on success or else \em false
     */
    bool setTimeout(struct tm &tm);

    /**
     * @brief   Set the expire offset
     * @param   offset_ms The expire offset in milliseconds
     *
     * Use this function to set an offset for the timer expiration. For
     * example, if the offset is set to 100ms, the timer will expire 100ms
     * after the time of day specification. It is also possible to set the
     * offset to a negative value.
     */
    void setExpireOffset(int offset_ms);

    /**
     * @brief   Start the timer
     * @return  Returns \em true on sucess or else \em false
     */
    bool start(void);

    /**
     * @brief   Stop the timer
     */
    void stop(void);
    
    /**
     * @brief 	A signal that is emitted when the timer expires
     * @param 	timer A pointer to the timer that has expired
     *
     * This signal is emitted when the timer expires. It is perfectly legal
     * to delete the timer in the connected slot if it is known to be the
     * only connected slot.
     */
    sigc::signal<void, AtTimer *> expired;

  protected:
    
  private:
    Timer           m_timer         {-1};
    struct timeval  m_expire_at;
    int             m_expire_offset {0};

    AtTimer(const AtTimer&);
    AtTimer& operator=(const AtTimer&);
    int msecToTimeout(void);
    void onTimerExpired(Timer *t);
    
};  /* class AtTimer */


} /* namespace */

#endif /* ASYNC_AT_TIMER_INCLUDED */



/*
 * This file has not been truncated
 */

