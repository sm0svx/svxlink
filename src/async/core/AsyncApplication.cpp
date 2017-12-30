/**
@file	 AsyncApplication.cpp
@brief   The core class for writing asyncronous applications
@author  Tobias Blomberg
@date	 2003-03-16

This file contains the AsyncApplication class which is the core of an
application that use the Async classes.

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2003-2015 Tobias Blomberg

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

#include <sys/types.h>
#include <sys/select.h>
#include <stdlib.h>

#include <cassert>
#include <algorithm>


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

#include "AsyncFdWatch.h"
#include "AsyncTimer.h"
#include "AsyncApplication.h"



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

Application *Application::app_ptr = 0;


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


Application &Application::app(void)
{
  assert(app_ptr != 0);
  return *app_ptr;
} /* Application::app */


/*
 *------------------------------------------------------------------------
 * Method:    
 * Purpose:   
 * Input:     
 * Output:    
 * Author:    
 * Created:   
 * Remarks:   
 * Bugs:      
 *------------------------------------------------------------------------
 */
Application::Application(void)
{
  assert(app_ptr == 0);
  app_ptr = this;  
  task_timer = new Async::Timer(0, Timer::TYPE_ONESHOT, false);
  task_timer->expired.connect(
      sigc::hide(mem_fun(*this, &Application::taskTimerExpired)));
} /* Application::Application */


Application::~Application(void)
{
  delete task_timer;
  task_timer = 0;
} /* Application::~Application */


void Application::runTask(sigc::slot<void> task)
{
  task_list.push_back(task);
  task_timer->setEnable(true);
} /* Application::runTask */



/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/

void Application::clearTasks(void)
{
  task_list.clear();
  task_timer->setEnable(false);
} /* Application::clearTasks */


/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/

void Application::taskTimerExpired(void)
{
  SlotList::iterator it;
  for (it=task_list.begin(); it!=task_list.end(); ++it)
  {
    (*it)();
  }
  clearTasks();
} /* Application::taskTimerExpired */



/*
 * This file has not been truncated
 */
