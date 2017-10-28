/**
@file	 Reflector.h
@brief   The main reflector class
@author  Tobias Blomberg / SM0SVX
@date	 2017-02-11

\verbatim
SvxReflector - An audio reflector for connecting SvxLink Servers
Copyright (C) 2003-2017 Tobias Blomberg / SM0SVX

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
  class UdpSocket;
  class Config;
};

class ReflectorClient;
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
     * @brief   Broadcast a TCP message to all connected clients except one
     * @param   msg The message to broadcast
     * @param   client The client to exclude from the broadcast
     *
     * This function is used to broadcast a message to all connected clients,
     * possibly excluding one client. The excluded client is most often the one
     * where the message originate from. The message is not really a IP
     * broadcast but rather unicast to all connected clients.
     */
    void broadcastMsgExcept(const ReflectorMsg& msg, ReflectorClient *except=0);

    /**
     * @brief   Send a UDP datagram to the specificed ReflectorClient
     * @param   client The client to the send datagram to
     * @param   buf The payload to send
     * @param   count The number of bytes in the payload
     * @return  Returns \em true on success or else \em false
     */
    bool sendUdpDatagram(ReflectorClient *client, const void *buf, size_t count);

  private:
    static const time_t TALKER_AUDIO_TIMEOUT = 3;   // Max three seconds gap

    typedef std::map<uint32_t, ReflectorClient*> ReflectorClientMap;
    typedef std::map<Async::FramedTcpConnection*,
                     ReflectorClient*> ReflectorClientConMap;
    typedef Async::TcpServer<Async::FramedTcpConnection> FramedTcpServer;

    FramedTcpServer*      m_srv;
    Async::UdpSocket*     m_udp_sock;
    ReflectorClientMap    m_client_map;
    ReflectorClient*      m_talker;
    Async::Timer          m_talker_timeout_timer;
    struct timeval        m_last_talker_timestamp;
    ReflectorClientConMap m_client_con_map;
    unsigned              m_sql_timeout;
    unsigned              m_sql_timeout_cnt;
    unsigned              m_sql_timeout_blocktime;
    Async::Config*        m_cfg;

    Reflector(const Reflector&);
    Reflector& operator=(const Reflector&);
    void clientConnected(Async::FramedTcpConnection *con);
    void clientDisconnected(Async::FramedTcpConnection *con,
                            Async::FramedTcpConnection::DisconnectReason reason);
    void udpDatagramReceived(const Async::IpAddress& addr, uint16_t port,
                             void *buf, int count);
    void broadcastUdpMsgExcept(const ReflectorClient *except,
                               const ReflectorUdpMsg& msg);
    void checkTalkerTimeout(Async::Timer *t);
    void setTalker(ReflectorClient *client);

};  /* class Reflector */


#endif /* REFLECTOR_INCLUDED */



/*
 * This file has not been truncated
 */
