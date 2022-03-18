/**
@file	 AsyncTimer.cpp
@brief   Contains a single shot or periodic timer that emits a signal on timeout
@author  Tobias Blomberg
@date	 2003-03-26

This file contains a class for creating a timer. Timers are a core
component in the event driven world. See usage instructions in the
class documentation.

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2003-2022 Tobias Blomberg

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

#include <cassert>


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

#include "AsyncApplication.h"
#include "AsyncTimer.h"



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


Timer::Timer(int timeout_ms, Type type, bool enabled)
  : m_type(type), m_timeout_ms(timeout_ms), m_is_enabled(false)
{
  setEnable(enabled && (timeout_ms >= 0));
} /* Timer::Timer */


Timer::~Timer(void)
{
  //expired.clear();
  setEnable(false);
} /* Timer::~Timer */


void Timer::setTimeout(int timeout_ms)
{
  m_timeout_ms = timeout_ms;
  if (m_timeout_ms >= 0)
  {
    reset();
  }
  else
  {
    setEnable(false);
  }
} /* Timer::setTimeout */


void Timer::setEnable(bool do_enable)
{
  assert((m_timeout_ms >= 0) || !do_enable);
  if (do_enable && !m_is_enabled)
  {
    Application::app().addTimer(this);
    m_is_enabled = true;
  }
  else if (!do_enable && m_is_enabled)
  {
    Application::app().delTimer(this);
    m_is_enabled = false;
  }
} /* Timer::setEnable */


void Timer::reset(void)
{
  if (m_is_enabled)
  {
    assert(m_timeout_ms >= 0);
    Application::app().delTimer(this);
    Application::app().addTimer(this);
  }
} /* Timer::reset */



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




/*
 * This file has not been truncated
 */

