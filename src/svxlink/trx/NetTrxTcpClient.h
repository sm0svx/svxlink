/**
@file	 NetTrxTcpClient.h
@brief   Network connection manager for remote transceivers
@author  Tobias Blomberg / SM0SVX
@date	 2008-03-15

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2024 Tobias Blomberg / SM0SVX

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


#ifndef NET_TRX_TCP_CLIENT_INCLUDED
#define NET_TRX_TCP_CLIENT_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <map>
#include <utility>
#include <string>

#include <sys/time.h>


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

#include "NetTrxMsg.h"


/****************************************************************************
 *
 * Forward declarations
 *
 ****************************************************************************/

namespace Async
{
  class Timer;
};


/****************************************************************************
 *
 * Namespace
 *
 ****************************************************************************/

//namespace MyNameSpace
//{


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
@brief	Network connection manager for remote transceivers
@author Tobias Blomberg / SM0SVX
@date   2008-03-15
*/
class NetTrxTcpClient : public Async::TcpClient<>
{
  public:
    typedef Async::TcpConnection::DisconnectReason DiscReason;
    
    /**
     * @brief 	Get a pointer to a client object with the given parameters
     * @param   remote_host   The hostname of the remote host
     * @param   remote_port   The port on the remote host to connect to
     * @return	Returns a pointer to a client object with the given parameters
     *
     * Use this function to get a pointer to a client object with the given
     * parameters. If remote_host and remote_port matches an already existing
     * client object, that will be returned. This way it's possible to share
     * one connection between two other objects.
     */
    static NetTrxTcpClient *instance(const std::string& remote_host,
      	      	      	      	     uint16_t remote_port);
    
    /**
     * @brief Delete a previously allocated instance if there are no more users
     */
    void deleteInstance(void);
    
    /**
     * @brief Setup the authentication key
     * @param key The autentication key to use
     */
    void setAuthKey(const std::string &key) { auth_key = key; }
    
    /**
     * @brief Send a message over the connection
     * @param msg The message to send
     */
    void sendMsg(NetTrxMsg::Msg *msg);
    
    /**
     * @brief Get the reason for the last disconnect
     */
    DiscReason disconnectReason(void) const { return disc_reason; }

    /**
     * @brief 	Connect to the remote host
     *
     * This function will initiate a connection to the remote host. The
     * connection must not be written to before the connected signal
     * (see @ref TcpClient::connected) has been emitted. If the connection is
     * already established or pending, nothing will happen.
     */
    void connect(void);

    /**
     * @brief A signal that is emitted when the connection to the remote side
     *        is ready for operation
     * @param is_ready  \em true when ready, \em false when not ready
     */
    sigc::signal<void, bool> isReady;
    
    /**
     * @brief A signal that is emitted when a message has been received
     * @param msg The received message
     */
    sigc::signal<void, NetTrxMsg::Msg*> msgReceived;
    
    
  protected:
    /**
     * @brief   Constructor
     * @param   remote_host   The hostname of the remote host
     * @param   remote_port   The port on the remote host to connect to
     * @param   recv_buf_len  The length of the receiver buffer to use
     *
     * The object will be constructed and variables will be initialized but
     * no connection will be created until the connect function
     * (see @ref TcpClient::connect) is called.
     */
    NetTrxTcpClient(const std::string& remote_host, uint16_t remote_port,
              size_t recv_buf_len = DEFAULT_RECV_BUF_LEN);
  
    /**
     * @brief 	Destructor
     */
    ~NetTrxTcpClient(void);
  
    
  private:
    typedef std::map<std::pair<const std::string, uint16_t>, NetTrxTcpClient*>
      	    Clients;
    typedef enum
    {
      STATE_DISC, STATE_VER_WAIT, STATE_AUTH_WAIT, STATE_READY
    } State;
    
    static const int RECV_BUF_SIZE = 4096;
    static Clients clients;

    char      	    recv_buf[RECV_BUF_SIZE];
    unsigned        recv_cnt;
    unsigned        recv_exp;
    Async::Timer    *reconnect_timer;
    struct timeval  last_msg_timestamp;
    Async::Timer    *heartbeat_timer;
    int       	    user_cnt;
    std::string     auth_key;
    State           state;
    DiscReason      disc_reason;
    
    NetTrxTcpClient(const NetTrxTcpClient&);
    using TcpClientBase::operator=;
    using TcpConnection::operator=;
    NetTrxTcpClient& operator=(const NetTrxTcpClient&);
    void tcpConnected(void);
    void tcpDisconnected(Async::TcpConnection *con,
      	      	      	 Async::TcpConnection::DisconnectReason reason);
    int tcpDataReceived(TcpConnection *con, void *data, int size);
    void reconnect(Async::Timer *t);
    void handleMsg(NetTrxMsg::Msg *msg);
    void heartbeat(Async::Timer *t);
    void localDisconnect(void);
    void sendMsgP(NetTrxMsg::Msg *msg);

};  /* class NetTrxTcpClient */


//} /* namespace */

#endif /* NET_TRX_TCP_CLIENT_INCLUDED */



/*
 * This file has not been truncated
 */

