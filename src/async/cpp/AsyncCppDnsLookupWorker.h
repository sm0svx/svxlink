/**
@file	 AsyncCppDnsLookupWorker.h
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



#ifndef ASYNC_CPP_DNS_LOOKUP_WORKER_INCLUDED
#define ASYNC_CPP_DNS_LOOKUP_WORKER_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <sigc++/sigc++.h>
#include <pthread.h>
#include <netdb.h>

#include <string>
#include <vector>


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

#include "../core/AsyncDnsLookupWorker.h"



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

class Timer;
class FdWatch;
  
  
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
@brief	DNS lookup worker for the Cpp variant of the async environment
@author Tobias Blomberg
@date   2003-03-23

This is the DNS lookup worker for the Cpp variant of the async environment.
It is an internal class that should only be used from within the async
library.
*/
class CppDnsLookupWorker : public DnsLookupWorker, public sigc::trackable
{
  public:
    /**
     * @brief 	Constructor
     * @param 	label The label (hostname) to lookup
     */
    CppDnsLookupWorker(const std::string& label);
  
    /**
     * @brief 	Destructor
     */
    ~CppDnsLookupWorker(void);
  
    /**
     * @brief   Called by the DnsLookup class to start the lookup
     * @return  Return \em true on success or else \em false
     */
    virtual bool doLookup(void);

    /**
     * @brief 	Return the addresses for the host in the query
     * @return	Returns an stl vector which contains all the addresses
     *	      	associated with the hostname in the query.
     *
     * Use this function to retrieve all the IP-addresses associated with
     * the hostname in the query.
     */
    virtual std::vector<IpAddress> addresses(void) { return the_addresses; }
    
    
  protected:
    
  private:
    std::string	      	    label;
    std::vector<IpAddress>  the_addresses;
    pthread_t 	      	    worker_thread;
    int       	      	    notifier_rd;
    int       	      	    notifier_wr;
    Async::FdWatch    	    *notifier_watch;
    bool      	      	    done;
    struct addrinfo    	    *result;
  
    static void *workerFunc(void *);

    void onTimeout(Timer *t);
    void notificationReceived(FdWatch *w);

};  /* class CppDnsLookupWorker */


} /* namespace */

#endif /* ASYNC_CPP_DNS_LOOKUP_WORKER_INCLUDED */



/*
 * This file has not been truncated
 */

