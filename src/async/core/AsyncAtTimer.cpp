/**
@file	 AsyncAtTimer.cpp
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



/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <sys/time.h>
#include <iostream>
#include <cstdio>


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

#include "AsyncAtTimer.h"


/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/

using namespace std;
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



/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/

AtTimer::AtTimer(void)
{
  timerclear(&m_expire_at);
  m_timer.expired.connect(mem_fun(*this, &AtTimer::onTimerExpired));
} /* AtTimer::AtTimer */


AtTimer::AtTimer(struct tm &tm, bool do_start)
{
  timerclear(&m_expire_at);
  m_timer.expired.connect(mem_fun(*this, &AtTimer::onTimerExpired));
  setTimeout(tm);
  if (do_start)
  {
    start();
  }
} /* AtTimer::AtTimer */


AtTimer::~AtTimer(void)
{

} /* AtTimer::~AtTimer */


bool AtTimer::setTimeout(time_t t)
{
  m_expire_at.tv_sec = t;
  if (m_timer.isEnabled())
  {
    return start();
  }
  return true;
} /* AtTimer::setTimeout */


bool AtTimer::setTimeout(struct tm &tm)
{
  time_t t = mktime(&tm);
  if (t == -1)
  {
    cerr << "mktime[AtTimer::setTimeout]: Could not set the timeout due to "
            "an invalid time format\n";
    return false;
  }
  return setTimeout(t);
} /* AtTimer::setTimeout */


void AtTimer::setExpireOffset(int offset_ms)
{
  m_expire_offset = offset_ms;
} /* AtTimer::setExpireOffset */


bool AtTimer::start(void)
{
  int msec = msecToTimeout();
  if (msec == -1)
  {
    return false;
  }
  m_timer.setTimeout(msec);
  m_timer.setEnable(true);
  return true;
} /* AtTimer::start */


void AtTimer::stop(void)
{
  m_timer.setEnable(false);
} /* AtTimer::stop */



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

/**
 * @brief   This function will calculate what to set the timer to
 * @return  Returns the number of milliseconds to set the timer to
 *
 * This function will calculate how many milliseconds there is left to the
 * wanted timeout time. If the time is larger than one minute it will be
 * chopped up in 59 second chunks so as to not loose precision.
 * One second before the wanted timeout time occurrs, a last timer is
 * schedules so as to obtain good precision for the final timeout.
 */
int AtTimer::msecToTimeout(void)
{
  struct timeval now;
  if (gettimeofday(&now, 0) == -1)
  {
    perror("gettimeofday[AtTimer::msecToTimeout]");
    return -1;
  }

  struct timeval diff;
  timersub(&m_expire_at, &now, &diff);

  long long msec = static_cast<long long>(diff.tv_sec) * 1000
                   + diff.tv_usec / 1000 + m_expire_offset + 1;
  if (msec < 0)
  {
    msec = 0;
  }
  else if (msec > 60000)
  {
    msec = 59000;
  }
  else if (msec > 1500)
  {
    msec -= 1000;
  }

  return static_cast<int>(msec);
} /* AtTimer::msecToTimeout */


/**
 * @brief Called by the timer when it expires
 * @param t The timer object
 *
 * This function will be called by the timer when it expires. If the specified
 * time of day have not been reached, the timer will be restarted.
 */
void AtTimer::onTimerExpired(Timer *t)
{
  int msec = msecToTimeout();
  if (msec > 0)
  {
    m_timer.setTimeout(msec);
  }
  else
  {
    expired(this);
  }
} /* AtTimer::onTimerExpired */



/*
 * This file has not been truncated
 */

