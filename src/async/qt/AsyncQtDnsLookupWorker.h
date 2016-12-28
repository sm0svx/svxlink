/**
@file	 AsyncQtDnsLookupWorker.h
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


#ifndef ASYNC_QT_DNS_LOOKUP_WORKER_INCLUDED
#define ASYNC_QT_DNS_LOOKUP_WORKER_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <sigc++/sigc++.h>

#include <QObject>
#include <QHostInfo>
#undef emit


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
@brief	DNS lookup worker for the Qt variant of the async environment
@author Tobias Blomberg
@date   2003-04-12

This is the DNS lookup worker for the Qt variant of the async environment.
It is an internal class that should only be used from within the async
library.
*/
class QtDnsLookupWorker : public QObject, public DnsLookupWorker
{
  Q_OBJECT
  
  public:
    /**
     * @brief 	Constructor
     * @param 	label The label (hostname) to lookup
     */
    QtDnsLookupWorker(const std::string& label);
  
    /**
     * @brief 	Destructor
     */
    virtual ~QtDnsLookupWorker(void);
  
    /**
     * @brief 	Return the addresses for the host in the query
     * @return	Returns an stl vector which contains all the addresses
     *	      	associated with the hostname in the query.
     *
     * Use this function to retrieve all the IP-addresses associated with
     * the hostname in the query.
     */
    std::vector<IpAddress> addresses(void);
    
  protected:
    
  private:
    int lookup_id;
    QHostInfo host_info;
    
  private slots:
    void onResultsReady(const QHostInfo &info);
    
};  /* class QtDnsLookupWorker */


} /* namespace */

#endif /* ASYNC_QT_DNS_LOOKUP_WORKER_INCLUDED */



/*
 * This file has not been truncated
 */

