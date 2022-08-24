/**
@file	 AsyncQtApplication.cpp
@brief   The core class for writing asyncronous cpp applications.
@author  Tobias Blomberg
@date	 2003-03-16

This file contains the AsyncQtApplication class which is the core of an
application that use the Async classes in a Qt application.

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

#include <sigc++/sigc++.h>

#include <QSocketNotifier>
#undef emit

#include <cassert>
#include <cstdlib>
#include <algorithm>
#include <iostream>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncDnsLookup.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "AsyncQtDnsLookupWorker.h"
#include "AsyncFdWatch.h"
#include "AsyncTimer.h"
#include "AsyncQtTimer.h"
#include "AsyncQtApplication.h"



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
QtApplication::QtApplication(int &argc, char **argv)
  : QApplication(argc, argv)
{
  
} /* QtApplication::QtApplication */


QtApplication::~QtApplication(void)
{
  clearTasks();
} /* QtApplication::~QtApplication */


void QtApplication::exec(void)
{
  QApplication::exec();
} /* QtApplication::exec */


void QtApplication::quit(void)
{
  QApplication::quit();
} /* QtApplication::quit */



/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/


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






/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/


/*
 *----------------------------------------------------------------------------
 * Method:    
 * Purpose:   
 * Input:     
 * Output:    
 * Author:    
 * Created:   
 * Remarks:   
 * Bugs:      
 *----------------------------------------------------------------------------
 */
void QtApplication::addFdWatch(FdWatch *fd_watch)
{
  QSocketNotifier *notifier = 0;
  
  switch (fd_watch->type())
  {
    case FdWatch::FD_WATCH_RD:
      notifier = new QSocketNotifier(fd_watch->fd(), QSocketNotifier::Read);
      rd_watch_map[fd_watch->fd()] = FdWatchMapItem(fd_watch, notifier);
      QObject::connect(notifier, SIGNAL(activated(int)),
                       this, SLOT(rdFdActivity(int)));
      break;
      
    case FdWatch::FD_WATCH_WR:
      notifier = new QSocketNotifier(fd_watch->fd(), QSocketNotifier::Write);
      wr_watch_map[fd_watch->fd()] = FdWatchMapItem(fd_watch, notifier);
      QObject::connect(notifier, SIGNAL(activated(int)),
                       this, SLOT(wrFdActivity(int)));
      break;
  }  
} /* QtApplication::addFdWatch */


void QtApplication::delFdWatch(FdWatch *fd_watch)
{
  FdWatchMap::iterator iter;
  
  switch (fd_watch->type())
  {
    case FdWatch::FD_WATCH_RD:
      iter = rd_watch_map.find(fd_watch->fd());
      assert(iter != rd_watch_map.end());
      delete iter->second.second;
      rd_watch_map.erase(fd_watch->fd());
      break;
      
    case FdWatch::FD_WATCH_WR:
      iter = wr_watch_map.find(fd_watch->fd());
      assert(iter != wr_watch_map.end());
      delete iter->second.second;
      wr_watch_map.erase(fd_watch->fd());
      break;
  }
  

} /* QtApplication::delFdWatch */


void QtApplication::rdFdActivity(int socket)
{
  FdWatchMap::iterator iter;
  iter = rd_watch_map.find(socket);
  assert(iter != rd_watch_map.end());
  iter->second.first->activity(iter->second.first);
} /* QtApplication::rdFdActivity */


void QtApplication::wrFdActivity(int socket)
{
  FdWatchMap::iterator iter;
  iter = wr_watch_map.find(socket);
  assert(iter != wr_watch_map.end());
  iter->second.first->activity(iter->second.first);
} /* QtApplication::wrFdActivity */


void QtApplication::addTimer(Timer *timer)
{
  AsyncQtTimer *t = new AsyncQtTimer(timer);
  timer_map[timer] = t;  
} /* QtApplication::addTimer */


void QtApplication::delTimer(Timer *timer)
{
  TimerMap::iterator iter;
  iter = timer_map.find(timer);
  assert(iter != timer_map.end());
  delete iter->second;
  timer_map.erase(iter);
} /* QtApplication::delTimer */


DnsLookupWorker *QtApplication::newDnsLookupWorker(const DnsLookup& lookup)
{
  return new QtDnsLookupWorker(lookup);
} /* QtApplication::newDnsLookupWorker */


/*
 * This file has not been truncated
 */

