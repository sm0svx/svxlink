/**
@file	 EchoLinkProxy.h
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

/** @example EchoLinkProxy_demo.cpp
An example of how to use the EchoLinkProxy class
*/


#ifndef ECHOLINK_PROXY_INCLUDED
#define ECHOLINK_PROXY_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <string>
#include <sigc++/sigc++.h>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncTcpClient.h>


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

namespace EchoLink
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
@brief	A_brief_class_description
@author Tobias Blomberg / SM0SVX
@date   2013-

A_detailed_class_description

\include EchoLinkProxy_demo.cpp
*/
class Proxy : public sigc::trackable
{
  public:
    typedef enum
    {
      STATE_DISCONNECTED, STATE_WAITING_FOR_DIGEST, STATE_WAITING_FOR_FIRST_MSG,
      STATE_CONNECTED
    } ProxyState;

    typedef enum
    {
      TCP_STATE_DISCONNECTED, TCP_STATE_DISCONNECTING, TCP_STATE_CONNECTING,
      TCP_STATE_CONNECTED
    } TcpState;

    static Proxy *instance(void) { return the_instance; }

    /**
     * @brief 	Default constuctor
     */
    Proxy(const std::string &host, uint16_t port, const std::string &callsign,
        const std::string &password);
  
    /**
     * @brief 	Destructor
     */
    ~Proxy(void);
  
    /**
     * @brief 	A_brief_member_function_description
     * @param 	param1 Description_of_param1
     * @return	Return_value_of_this_member_function
     */
    void connect(void);
    void disconnect(void);
    
    bool tcpOpen(const Async::IpAddress &remote_ip);
    bool tcpClose(void);
    TcpState tcpState(void) { return tcp_state; }
    bool tcpData(const void *data, unsigned len);
    bool udpData(const Async::IpAddress &addr, const void *data, unsigned len);
    bool udpCtrl(const Async::IpAddress &addr, const void *data, unsigned len);

    sigc::signal<void, bool> proxyReady;

    sigc::signal<void, uint32_t> tcpStatusReceived;
    sigc::signal<int, void*, unsigned> tcpDataReceived;
    sigc::signal<void> tcpCloseReceived;
    sigc::signal<void> tcpConnected;
    sigc::signal<void> tcpDisconnected;

    sigc::signal<void, const Async::IpAddress&, void*,
                 unsigned> udpDataReceived;
    sigc::signal<void, const Async::IpAddress&, void*,
                 unsigned> udpCtrlReceived;

  protected:
    
  private:
    typedef enum
    {
      MSG_TYPE_TCP_OPEN=1, MSG_TYPE_TCP_DATA, MSG_TYPE_TCP_CLOSE,
      MSG_TYPE_TCP_STATUS, MSG_TYPE_UDP_DATA, MSG_TYPE_UDP_CONTROL,
      MSG_TYPE_SYSTEM
    } MsgBlockType;

    static const int NONCE_SIZE = 8;
    static const int MSG_HEADER_SIZE = 1 + 4 + 4;
    static const int recv_buf_size = 16384;
    static Proxy *the_instance;

    Async::TcpClient con;
    const std::string callsign;
    std::string password;
    ProxyState state;
    TcpState tcp_state;
    uint8_t recv_buf[recv_buf_size];
    int recv_buf_cnt;

    Proxy(const Proxy&);
    Proxy& operator=(const Proxy&);
    bool sendMsgBlock(MsgBlockType type,
                      const Async::IpAddress &remote_ip=Async::IpAddress(),
                      const void *data=0, unsigned len=0);
    void onConnected(void);
    int onDataReceived(Async::TcpConnection *con, void *data, int len);
    void onDisconnected(Async::TcpConnection *con,
        Async::TcpClient::DisconnectReason reason);
    int handleAuthentication(const unsigned char *buf, int len);
    int parseProxyMessageBlock(unsigned char *buf, int len);
    void handleProxyMessageBlock(MsgBlockType type,
        const Async::IpAddress &remote_ip, uint32_t len,
        unsigned char *data);
    void handleTcpDataMsg(uint8_t *buf, int len);
    void handleTcpCloseMsg(const uint8_t *buf, int len);
    void handleTcpStatusMsg(const uint8_t *buf, int len);
    void handleUdpDataMsg(const Async::IpAddress &remote_ip, uint8_t *buf,
                          int len);
    void handleUdpCtrlMsg(const Async::IpAddress &remote_ip, uint8_t *buf,
                          int len);
    void handleSystemMsg(const unsigned char *buf, int len);
    
};  /* class Proxy */


} /* namespace */

#endif /* ECHOLINK_PROXY_INCLUDED */



/*
 * This file has not been truncated
 */

