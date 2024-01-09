/**
@file	 Reflector.h
@brief   The main reflector class
@author  Tobias Blomberg / SM0SVX
@date	 2017-02-11

\verbatim
SvxReflector - An audio reflector for connecting SvxLink Servers
Copyright (C) 2003-2023 Tobias Blomberg / SM0SVX

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

#ifndef REFLECTOR_INCLUDED
#define REFLECTOR_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <sigc++/sigc++.h>
#include <sys/time.h>
#include <vector>
#include <string>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncTcpServer.h>
#include <AsyncFramedTcpConnection.h>
#include <AsyncTimer.h>
#include <AsyncHttpServerConnection.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "ProtoVer.h"
#include "ReflectorClient.h"


/****************************************************************************
 *
 * Forward declarations
 *
 ****************************************************************************/

namespace Async
{
  class UdpSocket;
  class Config;
};

class ReflectorMsg;
class ReflectorUdpMsg;


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
@brief	The main reflector class
@author Tobias Blomberg / SM0SVX
@date   2017-02-11

This is the main class for the reflector. It handles all network traffic and
the dispatching of incoming messages to the correct ReflectorClient object.
*/
class Reflector : public sigc::trackable
{
  public:
    /**
     * @brief 	Default constructor
     */
    Reflector(void);

    /**
     * @brief 	Destructor
     */
    ~Reflector(void);

    /**
     * @brief 	Initialize the reflector
     * @param 	cfg A previously initialized configuration object
     * @return	Return \em true on success or else \em false
     */
    bool initialize(Async::Config &cfg);

    /**
     * @brief   Return a list of all connected nodes
     * @param   nodes The vector to return the result in
     *
     * This function is used to get a list of the callsigns of all connected
     * nodes.
     */
    void nodeList(std::vector<std::string>& nodes) const;

    /**
     * @brief   Broadcast a TCP message to connected clients
     * @param   msg The message to broadcast
     * @param   filter The client filter to apply
     *
     * This function is used to broadcast a message to all connected clients,
     * possibly applying a client filter.  The message is not really a IP
     * broadcast but rather unicast to all connected clients.
     */
    void broadcastMsg(const ReflectorMsg& msg,
        const ReflectorClient::Filter& filter=ReflectorClient::NoFilter());

    /**
     * @brief   Send a UDP datagram to the specificed ReflectorClient
     * @param   client The client to the send datagram to
     * @param   buf The payload to send
     * @param   count The number of bytes in the payload
     * @return  Returns \em true on success or else \em false
     */
    bool sendUdpDatagram(ReflectorClient *client, const void *buf, size_t count);

    void broadcastUdpMsg(const ReflectorUdpMsg& msg,
        const ReflectorClient::Filter& filter=ReflectorClient::NoFilter());

    /**
     * @brief   Get the TG for protocol V1 clients
     * @return  Returns the TG used for protocol V1 clients
     */
    uint32_t tgForV1Clients(void) { return m_tg_for_v1_clients; }

    /**
     * @brief   Request QSY to another talk group
     * @param   tg The talk group to QSY to
     */
    void requestQsy(ReflectorClient *client, uint32_t tg);

    uint32_t randomQsyLo(void) const { return m_random_qsy_lo; }
    uint32_t randomQsyHi(void) const { return m_random_qsy_hi; }

  private:
    typedef std::map<Async::FramedTcpConnection*,
                     ReflectorClient*> ReflectorClientConMap;
    typedef Async::TcpServer<Async::FramedTcpConnection> FramedTcpServer;

    FramedTcpServer*                                m_srv;
    Async::UdpSocket*                               m_udp_sock;
    ReflectorClientConMap                           m_client_con_map;
    Async::Config*                                  m_cfg;
    uint32_t                                        m_tg_for_v1_clients;
    uint32_t                                        m_random_qsy_lo;
    uint32_t                                        m_random_qsy_hi;
    uint32_t                                        m_random_qsy_tg;
    Async::TcpServer<Async::HttpServerConnection>*  m_http_server;

    Reflector(const Reflector&);
    Reflector& operator=(const Reflector&);
    void clientConnected(Async::FramedTcpConnection *con);
    void clientDisconnected(Async::FramedTcpConnection *con,
                            Async::FramedTcpConnection::DisconnectReason reason);
    void udpDatagramReceived(const Async::IpAddress& addr, uint16_t port,
                             void *buf, int count);
    void onTalkerUpdated(uint32_t tg, ReflectorClient* old_talker,
                         ReflectorClient *new_talker);
    void httpRequestReceived(Async::HttpServerConnection *con,
                             Async::HttpServerConnection::Request& req);
    void httpClientConnected(Async::HttpServerConnection *con);
    void httpClientDisconnected(Async::HttpServerConnection *con,
        Async::HttpServerConnection::DisconnectReason reason);
    void onRequestAutoQsy(uint32_t from_tg);
    uint32_t nextRandomQsyTg(void);

};  /* class Reflector */


#endif /* REFLECTOR_INCLUDED */



/*
 * This file has not been truncated
 */
