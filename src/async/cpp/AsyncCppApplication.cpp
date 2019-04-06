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
 * Copyright (C) 2003-2019 Tobias Blomberg
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

#include <sys/select.h>
#include <signal.h>
#include <unistd.h>

#include <cstdlib>
#include <cstdio>
#include <cerrno>
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

# define clock_timeradd(a, b, result)                                         \
  do {                                                                        \
    (result)->tv_sec = (a)->tv_sec + (b)->tv_sec;                             \
    (result)->tv_nsec = (a)->tv_nsec + (b)->tv_nsec;                          \
    if ((result)->tv_nsec >= 1000000000)                                      \
      {                                                                       \
        ++(result)->tv_sec;                                                   \
        (result)->tv_nsec -= 1000000000;                                      \
      }                                                                       \
  } while (0)

# define clock_timersub(a, b, result)                                         \
  do {                                                                        \
    (result)->tv_sec = (a)->tv_sec - (b)->tv_sec;                             \
    (result)->tv_nsec = (a)->tv_nsec - (b)->tv_nsec;                          \
    if ((result)->tv_nsec < 0) {                                              \
      --(result)->tv_sec;                                                     \
      (result)->tv_nsec += 1000000000;                                        \
    }                                                                         \
  } while (0)




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

int CppApplication::sighandler_pipe[2];


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
  : do_quit(false), max_desc(0), unix_signal_recv(-1), unix_signal_recv_cnt(0)
{
  FD_ZERO(&rd_set);
  FD_ZERO(&wr_set);
  sighandler_pipe[0] = sighandler_pipe[1] = -1;
} /* CppApplication::CppApplication */


CppApplication::~CppApplication(void)
{
  clearTasks();
} /* CppApplication::~CppApplication */


void CppApplication::exec(void)
{
  if (pipe(sighandler_pipe) == -1)
  {
    perror("pipe");
    exit(1);
  }

  FdWatch watch(sighandler_pipe[0], FdWatch::FD_WATCH_RD);
  watch.activity.connect(
      hide(mem_fun(*this, &CppApplication::handleUnixSignal)));

  for (UnixSignalMap::const_iterator it = unix_signals.begin();
       it != unix_signals.end();
       ++it)
  {
    struct sigaction act;
    act.sa_handler = unixSignalHandler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    if (sigaction((*it).first, &act, NULL) == -1)
    {
      perror("sigaction");
      exit(1);
    }
  }
  
  while (!do_quit)
  {
    struct timespec *timeout_ptr = 0;
    struct timespec timeout;
    TimerMap::iterator titer = timer_map.begin();
    while (titer != timer_map.end())
    {
      if (titer->second != 0)
      {
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	clock_timersub(&titer->first, &ts, &timeout);
	if (timeout.tv_sec < 0)
	{
	  timeout.tv_sec = 0;
	  timeout.tv_nsec = 0;
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
    int dcnt = pselect(max_desc, &local_rd_set, &local_wr_set, NULL,
	timeout_ptr, NULL);
    if (dcnt == -1)
    {
      if ((errno == EINTR) || (errno == EAGAIN))
      {
      	continue;
      }
      else
      {
        perror("pselect");
        exit(1);
      }
    }
    
    if ((timeout_ptr != 0)
        && ((dcnt == 0)
            || ((timeout_ptr->tv_sec == 0) && (timeout_ptr->tv_nsec == 0))
           )
       )
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
    while ((dcnt > 0) && (witer != rd_watch_map.end()))
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
    while ((dcnt > 0) && (witer != wr_watch_map.end()))
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

  for (UnixSignalMap::const_iterator it = unix_signals.begin();
       it != unix_signals.end();
       ++it)
  {
    if (sigaction((*it).first, &(*it).second, NULL) == -1)
    {
      perror("sigaction");
      exit(1);
    }
  }

  close(sighandler_pipe[1]);
  close(sighandler_pipe[0]);
  sighandler_pipe[0] = sighandler_pipe[1] = -1;
} /* CppApplication::exec */


void CppApplication::quit(void)
{
  do_quit = true;
} /* CppApplication::quit */


void CppApplication::catchUnixSignal(int signum)
{
  UnixSignalMap::iterator it = unix_signals.find(signum);
  if (it != unix_signals.end())
  {
    uncatchUnixSignal(signum);
  }

    // Store the old signal handler
  if (sigaction(signum, NULL, &unix_signals[signum]) == -1)
  {
    perror("sigaction");
    exit(1);
  }

    // Add signal handler if we're in the exec function
  if (sighandler_pipe[0] != -1)
  {
    struct sigaction act;
    act.sa_handler = unixSignalHandler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    if (sigaction(signum, &act, NULL) == -1)
    {
      perror("sigaction");
      exit(1);
    }
  }
} /* CppApplication::catchUnixSignal */


void CppApplication::uncatchUnixSignal(int signum)
{
  UnixSignalMap::iterator it = unix_signals.find(signum);
  if (it == unix_signals.end())
  {
    return;
  }
  if (sigaction(signum, &unix_signals[signum], NULL) == -1)
  {
    perror("sigaction");
    exit(1);
  }
} /* CppApplication::uncatchUnixSignal */



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

void CppApplication::unixSignalHandler(int signum)
{
  int cnt = write(sighandler_pipe[1], &signum, sizeof(signum));
  assert(cnt == sizeof(signum));
} /* CppApplication::unixSignalHandler */


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
    max_desc = -1;

    WatchMap::reverse_iterator riter;
    for (riter = rd_watch_map.rbegin(); riter != rd_watch_map.rend(); ++riter)
    {
      if ((riter->second != 0) && (riter->first > max_desc))
      {
        max_desc = riter->first;
        break;
      }
    }
    
    for (riter = wr_watch_map.rbegin(); riter != wr_watch_map.rend(); ++riter)
    {
      if ((riter->second != 0) && (riter->first > max_desc))
      {
        max_desc = riter->first;
        break;
      }
    }
    
    ++max_desc;
  }
} /* CppApplication::delFdWatch */


void CppApplication::addTimer(Timer *timer)
{
  struct timespec current;
  clock_gettime(CLOCK_MONOTONIC, &current);
  addTimerP(timer, current);
} /* CppApplication::addTimer */


void CppApplication::addTimerP(Timer *timer, const struct timespec& current)
{
  struct timespec add;
  struct timespec expiration;
  int timeout = timer->timeout();
  add.tv_sec = timeout / 1000;
  timeout -= add.tv_sec * 1000;
  add.tv_nsec = timeout * 1000000;
  clock_timeradd(&current, &add, &expiration);
  
  timer_map.insert(pair<struct timespec, Timer *>(expiration, timer));
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


void CppApplication::handleUnixSignal(void)
{
  char *ptr = reinterpret_cast<char*>(&unix_signal_recv) + unix_signal_recv_cnt;
  int cnt = read(sighandler_pipe[0], ptr,
                 sizeof(unix_signal_recv) - unix_signal_recv_cnt);
  assert(cnt > 0);
  unix_signal_recv_cnt += cnt;
  assert(unix_signal_recv_cnt <= sizeof(unix_signal_recv));
  if (unix_signal_recv_cnt == sizeof(unix_signal_recv))
  {
    unixSignalCaught(unix_signal_recv);
    unix_signal_recv_cnt = 0;
    unix_signal_recv = -1;
  }
} /* CppApplication::handleUnixSignal */



/*
 * This file has not been truncated
 */

