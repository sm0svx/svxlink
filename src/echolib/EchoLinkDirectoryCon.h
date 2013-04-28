/**
@file	 EchoLinkDirectoryCon.h
@brief   A_brief_description_for_this_file
@author  Tobias Blomberg / SM0SVX
@date	 2010-

A_detailed_description_for_this_file

\verbatim
<A brief description of the program or library this file belongs to>
Copyright (C) 2003-2010 Tobias Blomberg / SM0SVX

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

/** @example EchoLinkDirectoryCon_demo.cpp
An example of how to use the EchoLinkDirectoryCon class
*/


#ifndef ECHOLINK_DIRECTORY_CON_INCLUDED
#define ECHOLINK_DIRECTORY_CON_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <string>
#include <vector>
#include <sigc++/sigc++.h>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncTcpClient.h>
#include <AsyncDnsLookup.h>


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

namespace Async
{
  class DnsLookup;
};


/****************************************************************************
 *
 * Namespace
 *
 ****************************************************************************/

namespace EchoLink
{


/****************************************************************************
 *
 * Forward declarations of classes inside of the declared namespace
 *
 ****************************************************************************/

class Proxy;
  

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
@brief	A_brief_class_description
@author Tobias Blomberg / SM0SVX
@date   2013-04-27

A_detailed_class_description

\include EchoLinkDirectoryCon_demo.cpp
*/
class DirectoryCon : public sigc::trackable
{
  public:
    /**
     * @brief 	Default constuctor
     */
    DirectoryCon(const std::vector<std::string> &servers);
  
    /**
     * @brief 	Destructor
     */
    ~DirectoryCon(void);
  
    /**
     * @brief 	A_brief_member_function_description
     * @param 	param1 Description_of_param1
     * @return	Return_value_of_this_member_function
     */
    void connect(void);
    void disconnect(void);
    int lastDisconnectReason(void) { return last_disconnect_reason; }
    int write(const void *data, unsigned len);
    bool isReady(void) const { return is_ready; }
    bool isIdle(void) const;

    sigc::signal<void, bool> ready;
    sigc::signal<void> connected;
    sigc::signal<void> disconnected;
    sigc::signal<int, void *, unsigned> dataReceived;
    
  protected:
    
  private:
    static const int DIRECTORY_SERVER_PORT = 5200;

    std::vector<std::string>                servers;
    std::vector<Async::DnsLookup*>          dns_lookups;
    std::vector<Async::IpAddress>           addresses;
    std::vector<Async::IpAddress>::iterator current_server;
    Async::TcpClient *                      client;
    int                                     last_disconnect_reason;
    bool                                    is_ready;

    DirectoryCon(const DirectoryCon&);
    DirectoryCon& operator=(const DirectoryCon&);
    void doDnsLookup(void);
    void onDnsLookupResultsReady(Async::DnsLookup &dns);
    void doConnect(void);

    void onDisconnected(Async::TcpConnection *con,
                        Async::TcpClient::DisconnectReason reason);
    int onDataReceived(Async::TcpConnection *con, void *data, int len);

    //void proxyTcpStatusReceived(uint32_t status);
    void proxyReady(bool is_ready);
    //int proxyTcpDataReceived(void *data, unsigned len);
    //void proxyTcpCloseReceived(void);
    
};  /* class DirectoryCon */


} /* namespace */

#endif /* ECHOLINK_DIRECTORY_CON_INCLUDED */



/*
 * This file has not been truncated
 */

