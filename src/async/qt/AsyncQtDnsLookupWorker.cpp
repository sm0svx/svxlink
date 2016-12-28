/**
@file	 AsyncQtDnsLookupWorker.cpp
@brief   Execute DNS queries in the Qt environment
@author  Tobias Blomberg
@date	 2003-04-12

This file contains a class for executing DNS quries in the Qt variant of
the async environment. This class should never be used directly. It is
used by Async::QtApplication to execute DNS queries.

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2003-2010 Tobias Blomberg

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

#include "AsyncQtDnsLookupWorker.h"



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
QtDnsLookupWorker::QtDnsLookupWorker(const string& label)
  : lookup_id(-1)
{
  lookup_id = QHostInfo::lookupHost(label.c_str(), this,
				    SLOT(onResultsReady(QHostInfo)));
} /* QtDnsLookupWorker::QtDnsLookupWorker */


QtDnsLookupWorker::~QtDnsLookupWorker(void)
{
  if (lookup_id != -1)
  {
    QHostInfo::abortHostLookup(lookup_id);
  }
} /* QtDnsLookupWorker::~QtDnsLookupWorker */


vector<IpAddress> QtDnsLookupWorker::addresses(void)
{
  vector<IpAddress> addr_list;
  
  QList<QHostAddress> list = host_info.addresses();
  QList<QHostAddress>::Iterator it = list.begin();
  while(it != list.end())
  {
    if ((*it).protocol() == QAbstractSocket::IPv4Protocol)
    {
      addr_list.push_back(IpAddress((*it).toString().toStdString()));
    }
    ++it;
  }
  
  return addr_list;
  
} /* QtDnsLookupWorker::addresses */



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

void QtDnsLookupWorker::onResultsReady(const QHostInfo &info)
{
  lookup_id = -1;
  host_info = info;
  resultsReady();
} /* QtDnsLookupWorker::onResultsReady */



/*
 * This file has not been truncated
 */

