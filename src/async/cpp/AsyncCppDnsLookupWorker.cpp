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

/**
 * System Includes
 */
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <errno.h>
#include <cstdlib>
#include <cstdio>
#include <cassert>
#include <cstring>
#include <algorithm>

/**
 * Project Includes
 */
#include <AsyncTimer.h>
#include <AsyncFdWatch.h>

/**
 * Local Includes
 */
#include "AsyncCppDnsLookupWorker.h"

/**
 * Namespaces to use
 */
using namespace std;
using namespace Async;

/**
 * Defines & typedefs
 */

/**
 * Local class definitions
 */

/**
 * Prototypes
 */

/**
 * Exported Global Variables
 */

/**
 * Local Global Variables
 */

/**
 * Public member functions
 */
CppDnsLookupWorker::CppDnsLookupWorker(const string &label)
<<<<<<< HEAD
  : label(label), worker(0), notifier_rd(-1), notifier_wr(-1),
    notifier_watch(0), done(false), result(0)
{
} /** CppDnsLookupWorker::CppDnsLookupWorker */
=======
  : label(label), worker_thread(0), notifier_rd(-1), notifier_wr(-1),
    notifier_watch(0), done(false), result(0)
{
} /* CppDnsLookupWorker::CppDnsLookupWorker */

>>>>>>> refs/remotes/sm0svx/master

CppDnsLookupWorker::~CppDnsLookupWorker(void)
{
  if (worker_thread != 0)
  {
    if (!done)
    {
      int ret = pthread_cancel(worker_thread);
      if (ret != 0)
      {
        cerr << "*** WARNING: pthread_cancel: " << strerror(ret) << endl;
      }
    }
<<<<<<< HEAD

    void *ud;
    ret = pthread_join(worker, &ud);
=======
 
    int ret = pthread_join(worker_thread, NULL);
>>>>>>> refs/remotes/sm0svx/master
    if (ret != 0)
    {
      cerr << "*** WARNING: pthread_join: " << strerror(ret) << endl;
    }
  }
<<<<<<< HEAD

=======
  
>>>>>>> refs/remotes/sm0svx/master
  if (result != 0)
  {
    freeaddrinfo(result);
  }
<<<<<<< HEAD

=======
  
>>>>>>> refs/remotes/sm0svx/master
  delete notifier_watch;
  if (notifier_rd != -1)
  {
    close(notifier_rd);
  }
  if (notifier_wr != -1)
  {
    close(notifier_wr);
  }
<<<<<<< HEAD
} /** CppDnsLookupWorker::~CppDnsLookupWorker */

bool CppDnsLookupWorker::doLookup(void)
{

=======
} /* CppDnsLookupWorker::~CppDnsLookupWorker */


bool CppDnsLookupWorker::doLookup(void)
{
>>>>>>> refs/remotes/sm0svx/master
  int fd[2];
  if (pipe(fd) != 0)
  {
    cerr << "*** ERROR: Could not create pipe: " << strerror(errno) << endl;
    return false;
  }
  notifier_rd = fd[0];
  notifier_wr = fd[1];
  notifier_watch = new FdWatch(notifier_rd, FdWatch::FD_WATCH_RD);
  notifier_watch->activity.connect(
      	  mem_fun(*this, &CppDnsLookupWorker::notificationReceived));
<<<<<<< HEAD
  int ret = pthread_create(&worker, NULL, workerFunc, this);
=======
  int ret = pthread_create(&worker_thread, NULL, workerFunc, this);
>>>>>>> refs/remotes/sm0svx/master
  if (ret != 0)
  {
    cerr << "*** ERROR: pthread_create: " << strerror(ret) << endl;
    return false;
  }
<<<<<<< HEAD
  /**
  if (pthread_detach(worker) != 0)
  {
    perror("pthread_detach");
    return false;
  }
  */
=======
>>>>>>> refs/remotes/sm0svx/master

  return true;
  
} /** CppDnsLookupWorker::doLookup */

/**
 * Protected member functions
 */

/**
 * Private member functions
 */
/**
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

<<<<<<< HEAD
  int ret = getaddrinfo(worker->label.c_str(), NULL, NULL, &worker->result);
  if (ret != 0)
  {
    cerr << "getaddrinfo" << gai_strerror(ret) << endl;
=======
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  int ret = getaddrinfo(worker->label.c_str(), NULL, &hints, &worker->result);
  if (ret != 0)
  {
    cerr << "*** WARNING: Could not look up host \"" << worker->label
         << "\": " << gai_strerror(ret) << endl;
>>>>>>> refs/remotes/sm0svx/master
  }
  
  ret = write(worker->notifier_wr, "D", 1);
  assert(ret == 1);
  
  worker->done = true;
<<<<<<< HEAD
  
   return NULL;
=======
  return NULL;
>>>>>>> refs/remotes/sm0svx/master
  
} /** CppDnsLookupWorker::workerFunc */

/**
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

<<<<<<< HEAD
=======
  int ret = pthread_join(worker_thread, NULL);
  if (ret != 0)
  {
    cerr << "*** WARNING: pthread_join: " << strerror(ret) << endl;
  }
  worker_thread = 0;

>>>>>>> refs/remotes/sm0svx/master
  if (result != 0)
  {
    struct addrinfo *entry;
    for (entry = result; entry != 0; entry = entry->ai_next)
    {
<<<<<<< HEAD
      struct in_addr addr = ((struct sockaddr_in*)entry->ai_addr)->sin_addr;
      the_addresses.push_back(IpAddress(addr));;
    }
  }
  
  resultsReady();

} /** CppDnsLookupWorker::notificationReceived */
=======
      //printf("ai_family=%d ai_socktype=%d ai_protocol=%d\n",
      //       entry->ai_family, entry->ai_socktype, entry->ai_protocol);
      IpAddress ip_addr(
          reinterpret_cast<struct sockaddr_in*>(entry->ai_addr)->sin_addr);
      if (find(the_addresses.begin(), the_addresses.end(), ip_addr) ==
          the_addresses.end())
      {
        the_addresses.push_back(ip_addr);
      }
    }
  }
  resultsReady();
} /* CppDnsLookupWorker::notificationReceived */
>>>>>>> refs/remotes/sm0svx/master

/**
 * This file has not been truncated
 */

