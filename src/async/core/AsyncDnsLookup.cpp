/**
@file	 AsyncDnsLookup.cpp
@brief   Contains a class for executing DNS queries
@author  Tobias Blomberg
@date	 2003-04-12

This file contains a class for executing DNS queries asynchronously. When
the answer arrives, a signal will be emitted. See the class documentation
for usage instructions.

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2003-2013  Tobias Blomberg

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

#include <cassert>


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

#include "AsyncApplication.h"
#include "AsyncDnsLookupWorker.h"
#include "AsyncDnsLookup.h"



/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/

using namespace std;
using namespace sigc;
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
DnsLookup::DnsLookup(const string& label)
  : m_worker(0), m_label(label), m_results_ready(false)
{
  m_worker = Application::app().newDnsLookupWorker(label);
  m_worker->resultsReady.connect(mem_fun(*this, &DnsLookup::onResultsReady));
  m_worker->doLookup();
} /* DnsLookup::DnsLookup */


DnsLookup::~DnsLookup(void)
{
  delete m_worker;
} /* DnsLookup::~DnsLookup */


vector<IpAddress> DnsLookup::addresses(void)
{
  return m_worker->addresses();
} /* DnsLookup::addresses */



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
void DnsLookup::onResultsReady(void)
{
  m_results_ready = true;
  resultsReady(*this);
} /* DnsLookup::onResultsReady */



/*
 * This file has not been truncated
 */
