/**
 * @file    AsyncCppApplication.cpp
 * @brief   The core class for writing asyncronous cpp applications.
 * @author  Tobias Blomberg
 * @date    2003-03-16
 *
 * This file contains the AsyncCppApplication class which is the core of an
 * application that use the Async classes in a non-GUI application.
 *
 * \verbatim
 * Async - A library for programming event driven applications
 * Copyright (C) 2003  Tobias Blomberg
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * \endverbatim
 */




/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <assert.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/errno.h>

#include <cstdlib>
#include <cstdio>
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

#include "AsyncCppDnsLookupWorker.h"
#include "AsyncFdWatch.h"
#include "AsyncTimer.h"
#include "AsyncCppApplication.h"



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
CppApplication::CppApplication(void)
  : do_quit(false), max_desc(0)
{
  FD_ZERO(&rd_set);
  FD_ZERO(&wr_set);
  
} /* CppApplication::CppApplication */


CppApplication::~CppApplication(void)
{
  
} /* CppApplication::~CppApplication */


void CppApplication::exec(void)
{
  while (!do_quit)
  {
    struct timeval *timeout_ptr = 0;
    struct timeval timeout;
    TimerMap::iterator titer = timer_map.begin();
    while (titer != timer_map.end())
    {
      if (titer->second != 0)
      {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	timersub(&titer->first, &tv, &timeout);
	if (timeout.tv_sec < 0)
	{
	  timeout.tv_sec = 0;
	  timeout.tv_usec = 0;
	}
	timeout_ptr = &timeout;
	break;
      }
      else
      {
	timer_map.erase(titer);
      }
      titer = timer_map.begin();
    }
    
    fd_set local_rd_set = rd_set;
    fd_set local_wr_set = wr_set;
    int dcnt = select(max_desc, &local_rd_set, &local_wr_set, NULL,
	timeout_ptr);
    if (dcnt == -1)
    {
      if (errno == EINTR)
      {
      	continue;
      }
      else
      {
      	perror("select");
      	exit(1);
      }
    }
    
    if ((timeout_ptr != 0) && (timeout_ptr->tv_sec == 0) &&
	(timeout_ptr->tv_usec == 0))
    {
      titer->second->expired(titer->second);
      if ((titer->second != 0) &&
	  (titer->second->type() == Timer::TYPE_PERIODIC))
      {
	addTimerP(titer->second, titer->first);
      }
      timer_map.erase(titer);
    }
    
    WatchMap::iterator witer, next_witer;
    
      /* Check for activity on the read watch file descriptors */
    witer=rd_watch_map.begin();
    while (witer != rd_watch_map.end())
    {
      next_witer = witer;
      ++next_witer;
      if (FD_ISSET(witer->first, &local_rd_set))
      {
	if (witer->second != 0)
	{
	  witer->second->activity(witer->second);
	}
	else
	{
	  rd_watch_map.erase(witer);
	}
	--dcnt;
      }
      witer = next_witer;
    }
    
      /* Check for activity on the write watch file descriptors */
    witer=wr_watch_map.begin();
    while (witer != wr_watch_map.end())
    {
      next_witer = witer;
      ++next_witer;
      if (FD_ISSET(witer->first, &local_wr_set))
      {
	if (witer->second != 0)
	{
	  witer->second->activity(witer->second);
	}
	else
	{
	  wr_watch_map.erase(witer);
	}
	--dcnt;
      }
      witer = next_witer;
    }
    
    assert(dcnt == 0);
  }
} /* CppApplication::exec */


void CppApplication::quit(void)
{
  do_quit = true;
} /* CppApplication::quit */



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
void CppApplication::addFdWatch(FdWatch *fd_watch)
{
  int fd = fd_watch->fd();
  //printf("Adding watch for fd=%d (max_desc=%d)\n", fd, max_desc);
  
  WatchMap *watch_map = 0;
  switch (fd_watch->type())
  {
    case FdWatch::FD_WATCH_RD:
      FD_SET(fd, &rd_set);
      watch_map = &rd_watch_map;
      break;

    case FdWatch::FD_WATCH_WR:
      FD_SET(fd, &wr_set);
      watch_map = &wr_watch_map;
      break;
  }
  assert(watch_map != 0);

  WatchMap::iterator iter = watch_map->find(fd);
  assert((iter == watch_map->end()) || (iter->second == 0));
  
  if (fd+1 > max_desc)
  {
    max_desc = fd+1;
  }

  (*watch_map)[fd] = fd_watch;
  
} /* CppApplication::addFdWatch */


void CppApplication::delFdWatch(FdWatch *fd_watch)
{
  int fd = fd_watch->fd();
  WatchMap *watch_map = 0;
  switch (fd_watch->type())
  {
    case FdWatch::FD_WATCH_RD:
      FD_CLR(fd, &rd_set);
      watch_map = &rd_watch_map;
      break;
      
    case FdWatch::FD_WATCH_WR:
      FD_CLR(fd, &wr_set);
      watch_map = &wr_watch_map;
      break;
  }
  assert(watch_map != 0);
  
  WatchMap::iterator iter = watch_map->find(fd);
  assert((iter != watch_map->end()) && (iter->second != 0));
  iter->second = 0;
  
  if (fd+1 == max_desc)
  {
    max_desc = 0;
    WatchMap::reverse_iterator riter;
    riter = rd_watch_map.rbegin();
    if ((riter != rd_watch_map.rend()) && (riter->first > max_desc))
    {
      max_desc = riter->first;
    }
    
    riter = wr_watch_map.rbegin();
    if ((riter != wr_watch_map.rend()) && (riter->first > max_desc))
    {
      max_desc = riter->first;
    }
    
    ++max_desc;
  }
  
} /* CppApplication::delFdWatch */


void CppApplication::addTimer(Timer *timer)
{
  struct timeval current;
  gettimeofday(&current, NULL);
  addTimerP(timer, current);
} /* CppApplication::addTimer */


void CppApplication::addTimerP(Timer *timer, const struct timeval& current)
{
  struct timeval add;
  struct timeval expiration;
  int timeout = timer->timeout();
  add.tv_sec = timeout / 1000;
  timeout -= add.tv_sec * 1000;
  add.tv_usec = timeout * 1000;
  timeradd(&current, &add, &expiration);
  
  timer_map.insert(pair<struct timeval, Timer *>(expiration, timer));
} /* CppApplication::addTimerP */


void CppApplication::delTimer(Timer *timer)
{
  TimerMap::iterator iter;
  for (iter=timer_map.begin(); iter!=timer_map.end(); ++iter)
  {
    if (iter->second == timer)
    {
      //timer_map.erase(iter);
      iter->second = 0;
      break;
    }
  }
} /* CppApplication::delTimer */


DnsLookupWorker *CppApplication::newDnsLookupWorker(const string& label)
{
  return new CppDnsLookupWorker(label);
} /* CppApplication::newDnsLookupWorker */




/*
 * This file has not been truncated
 */

