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
Copyright (C) 2003-2022 Tobias Blomberg

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

class DnsLookup;


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
     * @param 	dns The lookup object
     */
    QtDnsLookupWorker(const DnsLookup& dns);
  
    /**
     * @brief 	Destructor
     */
    virtual ~QtDnsLookupWorker(void);

    /**
     * @brief   Move assignment operator
     * @param   other The other object to move data from
     * @return  Returns this object
     */
    virtual DnsLookupWorker& operator=(DnsLookupWorker&& other_base);

  protected:
    /**
     * @brief   Called by the DnsLookupWorker class to start the lookup
     * @return  Return \em true on success or else \em false
     */
    virtual bool doLookup(void);

    /**
     * @brief   Called by the DnsLookupWorker class to abort a pending lookup
     * @return  Return \em true on success or else \em false
     */
    virtual void abortLookup(void);

  private:
    int                     m_lookup_id = -1;

  private slots:
    void onResultsReady(const QHostInfo &info);
    
};  /* class QtDnsLookupWorker */


} /* namespace */

#endif /* ASYNC_QT_DNS_LOOKUP_WORKER_INCLUDED */



/*
 * This file has not been truncated
 */

