/**
@file	 AsyncDnsLookup.h
@brief   Contains a class for executing DNS queries
@author  Tobias Blomberg
@date	 2003-04-12

This file contains a class for executing DNS queries asynchronously. When
the answer arrives, a signal will be emitted. See the class documentation
for usage instructions.

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

/** @example  AsyncDnsLookup_demo.cpp
An example of how to use the Async::DnsLookup class
*/



#ifndef ASYNC_DNS_LOOKUP_INCLUDED
#define ASYNC_DNS_LOOKUP_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <sigc++/sigc++.h>

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

class DnsLookupWorker;


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
@brief	A class for performing asynchronous DNS lookups
@author Tobias Blomberg
@date   2003-04-12

Use this class to make DNS lookups. Right now it only supports looking up
hostnames to find out what IP-addresses it maps to. An example usage can be seen
below.

\include AsyncDnsLookup_demo.cpp
*/
class DnsLookup : public sigc::trackable
{
  public:
    /**
     * @brief 	Constructor
     * @param 	label The label (hostname) to lookup
     */
    DnsLookup(const std::string& label);
  
    /**
     * @brief 	Destructor
     */
    ~DnsLookup(void);
    
    /**
     * @brief  Return the associated label
     * @return Returns the label associated with this DNS lookup
     */
    const std::string &label(void) const { return m_label; }

    /**
     * @brief  Check if the DNS lookup is done or not
     * @return Returns \em true if results are ready or \em false if not
     */
    bool resultsAreReady(void) { return m_results_ready; }

    /**
     * @brief 	Return the addresses for the host in the query
     * @return	Return a stl vector which contains all the addresses
     *	      	associated with the hostname in the query.
     * @pre   	The result is not available before the resultsReay signal
     *	      	has been emitted
     *
     * Use this function to retrieve all the IP-addresses associated with
     * the hostname in the query. If the list is empty, the query has failed.
     */
    std::vector<IpAddress> addresses(void);
    
    /**
     * @brief 	A signal to indicate that the query has been completed
     * @param 	dns A reference to the DNS object associated with the query
     */
    sigc::signal<void, DnsLookup&> resultsReady;
    
    
  protected:
    
  private:
    DnsLookupWorker * m_worker;
    std::string       m_label;
    bool              m_results_ready;
    
    void onResultsReady(void);

};  /* class DnsLookup */


} /* namespace */

#endif /* ASYNC_DNS_LOOKUP_INCLUDED */



/*
 * This file has not been truncated
 */

