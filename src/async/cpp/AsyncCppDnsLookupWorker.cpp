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

#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


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

#include "AsyncTimer.h"
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
CppDnsLookupWorker::CppDnsLookupWorker(const string &label)
  : label(label)
{
  timer = new Timer(0);
  timer->expired.connect(slot(this, &CppDnsLookupWorker::onTimeout));
  
} /* CppDnsLookupWorker::CppDnsLookupWorker */


CppDnsLookupWorker::~CppDnsLookupWorker(void)
{
  delete timer;
} /* CppDnsLookupWorker::~CppDnsLookupWorker */



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
void CppDnsLookupWorker::onTimeout(Timer *timer)
{
  struct hostent *he = gethostbyname(label.c_str());
  if (he != 0)
  {
    struct in_addr *h_addr;
    h_addr = (struct in_addr *)he->h_addr_list[0];
    while (h_addr->s_addr != INADDR_ANY)
    {
      the_addresses.push_back(IpAddress(*h_addr));
      ++h_addr;
    }
  }
  
  resultsReady();
  
  delete timer;
  timer = 0;
  
} /* CppDnsLookupWorker::onTimeout */






/*
 * This file has not been truncated
 */

