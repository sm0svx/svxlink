/**
@file	 AsyncCppDnsLookupWorker.h
@brief   Contains a class to execute DNS queries in the Posix environment
@author  Tobias Blomberg
@date	 2003-04-17

This file contains a class for executing DNS quries in the Cpp variant of
the async environment. This class should never be used directly. It is
used by Async::CppApplication to execute DNS queries.

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2003-2023 Tobias Blomberg

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
#include <arpa/nameser.h>

#include <string>
#include <sstream>
#include <future>
#include <netdb.h>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncFdWatch.h>


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
 * Forward declarations of classes inside of the declared namespace
 *
 ****************************************************************************/



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
@brief	DNS lookup worker for the Cpp (Posix) variant of the async environment
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
     * @param 	dns The lookup object
     */
    CppDnsLookupWorker(const DnsLookup& dns);

    /**
     * @brief 	Destructor
     */
    ~CppDnsLookupWorker(void);

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
    struct ThreadContext
    {
      std::string         label;
      DnsLookup::Type     type                = DnsLookup::Type::A;
      int                 notifier_wr         = -1;
      unsigned char       answer[NS_MAXMSG];
      int                 anslen              = 0;
      struct addrinfo*    addrinfo            = nullptr;
      char                host[NI_MAXHOST]    = {0};
      std::ostringstream  thread_cerr;

      ~ThreadContext(void)
      {
        if (addrinfo != nullptr)
        {
          freeaddrinfo(addrinfo);
          addrinfo = nullptr;
        }
      }
    };

    Async::FdWatch                  m_notifier_watch;
    std::future<void>               m_result;
    std::unique_ptr<ThreadContext>  m_ctx;

    static void workerFunc(ThreadContext& ctx);
    void notificationReceived(FdWatch *w);
    void printErrno(const std::string& msg);

};  /* class CppDnsLookupWorker */


} /* namespace */

#endif /* ASYNC_CPP_DNS_LOOKUP_WORKER_INCLUDED */



/*
 * This file has not been truncated
 */

