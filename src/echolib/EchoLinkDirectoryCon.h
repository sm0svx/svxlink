/**
@file	 EchoLinkDirectoryCon.h
@brief   EchoLink directory server connection
@author  Tobias Blomberg / SM0SVX
@date	 2013-04-27

\verbatim
EchoLib - A library for EchoLink communication
Copyright (C) 2003-2013 Tobias Blomberg / SM0SVX

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
@brief	Representation of the connection to the EchoLink directory server
@author Tobias Blomberg / SM0SVX
@date   2013-04-27

This class represents the connection to the EchoLink directory server.
There are two ways to connect, either directly or via an EchoLink proxy
server. The API is the same, no matter how the connection is made so the
user class does not have to bother with details about if the connection
goes directly or through a proxy.
*/
class DirectoryCon : public sigc::trackable
{
  public:
    /**
     * @brief 	Constructor
     * @param   servers A list of possible directory servers to use
     */
    DirectoryCon(const std::vector<std::string> &servers,
                 const Async::IpAddress &bind_ip);
  
    /**
     * @brief 	Destructor
     */
    ~DirectoryCon(void);
  
    /**
     * @brief 	Initiate a connection to the directory server
     */
    void connect(void);

    /**
     * @brief   Disconnect from the directory server
     */
    void disconnect(void);

    /**
     * @brief   Get the reason for the last disconnect
     * @return  Returns the reason for the last disconnect
     */
    int lastDisconnectReason(void) { return last_disconnect_reason; }

    /**
     * @brief   Write data to the directory server
     * @param   data Pointer to the buffer where data to send is stored
     * @param   len The size of the data to write
     * @returns Returns the number of bytes written
     */
    int write(const void *data, unsigned len);

    /**
     * @brief   Check if the connection object is ready
     * @return  Returns \em true if the connection is ready or else \em false
     *
     * The connection object is considered ready if it is prepared to process
     * connections to the directory server. When using a direct connection,
     * the connection object will always be ready. When using a proxy, the
     * connection will be considered ready when connected to the proxy.
     */
    bool isReady(void) const { return is_ready; }

    /**
     * @brief   Check if the connection object is idle
     * @return  Returns \em if the connection object is idle
     *
     * The connection object is considered to be idle when ready but not
     * connected, that is, it is not busy doing something and ready to
     * process new connections.
     */
    bool isIdle(void) const;

    /**
     * @brief Signal emitted when the ready status changes
     * @param is_ready Set to \em true if ready
     *
     * This signal will be emitted when the ready status changes. For an
     * explaination of what ready mean, have a look att the documentation for
     * the isReady function.
     */
    sigc::signal<void, bool> ready;

    /**
     * @brief Signal emitted when the connection has been established
     */
    sigc::signal<void> connected;

    /**
     * @brief Signal emitted when the connection has been closed
     */
    sigc::signal<void> disconnected;

    /**
     * @brief   Signal emitted when data has been received
     * @param   data Pointer to the buffer where data to send is stored
     * @param   len The size of the data to write
     * @returns Return the number of bytes processed in the handler
     */
    sigc::signal<int, void *, unsigned> dataReceived;
    
  protected:
    
  private:
    static const int DIRECTORY_SERVER_PORT = 5200;

    std::vector<std::string>                servers;
    std::vector<Async::DnsLookup*>          dns_lookups;
    std::vector<Async::IpAddress>           addresses;
    std::vector<Async::IpAddress>::iterator current_server;
    Async::TcpClient<>*                     client;
    int                                     last_disconnect_reason;
    bool                                    is_ready;

    DirectoryCon(const DirectoryCon&);
    DirectoryCon& operator=(const DirectoryCon&);
    void doDnsLookup(void);
    void onDnsLookupResultsReady(Async::DnsLookup &dns);
    void doConnect(void);
    void onDisconnected(Async::TcpConnection *con,
                        Async::TcpClient<>::DisconnectReason reason);
    int onDataReceived(Async::TcpConnection *con, void *data, int len);
    void proxyReady(bool is_ready);
    
};  /* class DirectoryCon */


} /* namespace */

#endif /* ECHOLINK_DIRECTORY_CON_INCLUDED */



/*
 * This file has not been truncated
 */

