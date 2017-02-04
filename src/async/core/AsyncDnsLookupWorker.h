/**
@file	 AsyncDnsLookupWorker.h
@brief   Contains a base class for implementing a DNS lookup worker object
@author  Tobias Blomberg
@date	 2003-04-12

This file contains a class that is used as a base class for a DNS lookup
worker. That is an object that do the actual work when a DNS query have
been created using a Async::DnsQuery class. This worker class is only
used internally by the async library.

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



#ifndef ASYNC_DNS_LOOKUP_WORKER_INCLUDED
#define ASYNC_DNS_LOOKUP_WORKER_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <sigc++/sigc++.h>

#include <string>
#include <vector>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncIpAddress.h>


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
@brief	DNS lookup worker base class
@author Tobias Blomberg
@date   2003-04-12

This is the DNS lookup worker base class. It is an internal class that should
only be used from within the async library.
*/
class DnsLookupWorker
{
  public:
    /**
     * @brief 	Constructor
     */
    DnsLookupWorker(void) {}
  
    /**
     * @brief 	Destructor
     */
    virtual ~DnsLookupWorker(void) {}
    
    /**
     * @brief   Called by the DnsLookup class to start the lookup
     * @return  Return \em true on success or else \em false
     */
    virtual bool doLookup(void) { return true; }
    
    /**
     * @brief 	Return the addresses for the host in the query
     * @return	Returns an stl vector which contains all the addresses
     *	      	associated with the hostname in the query.
     *
     * Use this function to retrieve all the IP-addresses associated with
     * the hostname in the query.
     * This is a pure virtual member function which must be reimplemented
     * in the derived classes.
     */
    virtual std::vector<IpAddress> addresses(void) = 0;
    
    /**
     * @brief 	A signal to indicate that the query has been completed
     */
    sigc::signal<void> resultsReady;
    
  protected:
    
  private:
    
};  /* class DnsLookupWorker */


} /* namespace */

#endif /* ASYNC_DNS_LOOKUP_WORKER_INCLUDED */



/*
 * This file has not been truncated
 */

