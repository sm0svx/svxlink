/**
@file	 AsyncCppDnsLookupWorker.cpp
@brief   Contains a class to execute DNS queries in the pure C++ environment
@author  Tobias Blomberg
@date	 2003-04-17

This file contains a class for executing DNS quries in the Cpp variant of
the async environment. This class should never be used directly. It is
used by Async::CppApplication to execute DNS queries.

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





/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <errno.h>
#include <cstdlib>
#include <cstdio>
#include <cassert>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncTimer.h>
#include <AsyncFdWatch.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "AsyncCppDnsLookupWorker.h"



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


CppDnsLookupWorker::CppDnsLookupWorker(const string &label)
  : label(label), worker(0), notifier_rd(-1), notifier_wr(-1),
    notifier_watch(0), done(false), result(0), buf(0)
{
  int ret = pthread_mutex_init(&mutex, NULL);
  if (ret != 0)
  {
    cerr << "pthread_mutex_init: error " << ret << endl;
  }
} /* CppDnsLookupWorker::CppDnsLookupWorker */


CppDnsLookupWorker::~CppDnsLookupWorker(void)
{
  if (worker != 0)
  {
    int ret;
    
    if (!done)
    {
      ret = pthread_cancel(worker);
      if (ret != 0)
      {
	cerr << "pthread_cancel: error " << ret << endl;
      }
    }
   
    void *ud;
    ret = pthread_join(worker, &ud);
    if (ret != 0)
    {
      cerr << "pthread_join: error " << ret << endl;
    }
  }
  
  free(buf);
  buf = 0;
  
  delete notifier_watch;
  if (notifier_rd != -1)
  {
    close(notifier_rd);
  }
  if (notifier_wr != -1)
  {
    close(notifier_wr);
  }
  
  int ret = pthread_mutex_destroy(&mutex);
  if (ret != 0)
  {
    cerr << "pthread_mutex_destroy: error " << ret << endl;
  }
} /* CppDnsLookupWorker::~CppDnsLookupWorker */


bool CppDnsLookupWorker::doLookup(void)
{
  int ret = pthread_mutex_lock(&mutex);
  if (ret != 0)
  {
    cerr << "pthread_mutex_lock: error " << ret << endl;
  }
  
  int fd[2];
  if (pipe(fd) != 0)
  {
    perror("pipe");
    return false;
  }
  notifier_rd = fd[0];
  notifier_wr = fd[1];
  notifier_watch = new FdWatch(notifier_rd, FdWatch::FD_WATCH_RD);
  notifier_watch->activity.connect(
      	  mem_fun(*this, &CppDnsLookupWorker::notificationReceived));
  ret = pthread_create(&worker, NULL, workerFunc, this);
  if (ret != 0)
  {
    cerr << "pthread_create: error " << ret << endl;
    return false;
  }
  /*
  if (pthread_detach(worker) != 0)
  {
    perror("pthread_detach");
    return false;
  }
  */

  ret = pthread_mutex_unlock(&mutex);
  if (ret != 0)
  {
    cerr << "pthread_mutex_unlock: error " << ret << endl;
  }
    
  return true;
  
} /* CppDnsLookupWorker::doLookup */




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
 *----------------------------------------------------------------------------
 * Method:    CppDnsLookupWorker::workerFunc
 * Purpose:   This is the function that do the actual DNS lookup. It is
 *    	      started as a separate thread since gethostbyname is a
 *    	      blocking function.
 * Input:     w - Thread user data. This is the actual CppDnsLookupWorker
 *    	      	  object
 * Output:    the_addresses will be filled in with all IP addresses
 *    	      associated with the name.
 * Author:    Tobias Blomberg
 * Created:   2005-04-12
 * Remarks:   
 * Bugs:      
 *----------------------------------------------------------------------------
 */
void *CppDnsLookupWorker::workerFunc(void *w)
{
  CppDnsLookupWorker *worker = reinterpret_cast<CppDnsLookupWorker *>(w);

  int ret = pthread_mutex_lock(&worker->mutex);
  if (ret != 0)
  {
    cerr << "pthread_mutex_lock: error " << ret << endl;
  }
  
  int bufsize = 512;
  do
  {
    worker->buf = (char *)realloc(worker->buf, bufsize);
    int h_errnop;
    ret = gethostbyname_r(worker->label.c_str(), &worker->he_buf, worker->buf,
      	      	      	  bufsize, &worker->result, &h_errnop);
    bufsize *= 2;
  } while (ret == ERANGE);
  
  if ((ret != 0) || (worker->result == 0))
  {
    free(worker->buf);
    worker->buf = 0;
    worker->result = 0;
  }
  
  ret = write(worker->notifier_wr, "D", 1);
  assert(ret == 1);
  
  worker->done = true;
  
  ret = pthread_mutex_unlock(&worker->mutex);
  if (ret != 0)
  {
    cerr << "pthread_mutex_unlock: error " << ret << endl;
  }
    
  return NULL;
  
} /* CppDnsLookupWorker::workerFunc */


/*
 *----------------------------------------------------------------------------
 * Method:    CppDnsLookupWorker::notificationReceived
 * Purpose:   When the DNS lookup thread is done, this function will be
 *    	      called to notify the user of the result.
 * Input:     w - The file watch object (notification pipe)
 * Output:    None
 * Author:    Tobias Blomberg
 * Created:   2005-04-12
 * Remarks:   
 * Bugs:      
 *----------------------------------------------------------------------------
 */
void CppDnsLookupWorker::notificationReceived(FdWatch *w)
{
  w->setEnabled(false);

  int ret = pthread_mutex_lock(&mutex);
  if (ret != 0)
  {
    cerr << "pthread_mutex_lock: error " << ret << endl;
  }

  if (result != 0)
  {
    for (int i=0; result->h_addr_list[i] != NULL; ++i)
    {
      struct in_addr *addr;      
      addr = reinterpret_cast<struct in_addr *>(result->h_addr_list[i]);
      the_addresses.push_back(IpAddress(*addr));
    }
  }
  
  ret = pthread_mutex_unlock(&mutex);
  if (ret != 0)
  {
    cerr << "pthread_mutex_unlock: error " << ret << endl;
  }
  
  resultsReady();

} /* CppDnsLookupWorker::notificationReceived */



/*
 * This file has not been truncated
 */

