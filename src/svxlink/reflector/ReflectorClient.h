/**
@file	 ReflectorClient.h
@brief   A_brief_description_for_this_file
@author  Tobias Blomberg / SM0SVX
@date	 2017-02-11

A_detailed_description_for_this_file

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

/** @example Template_demo.cpp
An example of how to use the ReflectorClient class
*/


#ifndef REFLECTOR_CLIENT_INCLUDED
#define REFLECTOR_CLIENT_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <string>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncFramedTcpConnection.h>
#include <AsyncTimer.h>
#include <AsyncConfig.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "ReflectorMsg.h"


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

//namespace MyNameSpace
//{


/****************************************************************************
 *
 * Forward declarations of classes inside of the declared namespace
 *
 ****************************************************************************/

class Reflector;


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
@date   2017-02-11

A_detailed_class_description

\include Template_demo.cpp
*/
class ReflectorClient
{
  public:
    /**
     * @brief 	Constructor
     */
    ReflectorClient(Reflector *ref, Async::FramedTcpConnection *con,
                    Async::Config* cfg);

    /**
     * @brief 	Destructor
     */
    ~ReflectorClient(void);

    /**
     * @brief 	A_brief_member_function_description
     * @param 	param1 Description_of_param1
     * @return	Return_value_of_this_member_function
     */
    uint32_t clientId(void) const { return m_client_id; }
    const Async::IpAddress& remoteHost(void) const
    {
      return m_con->remoteHost();
    }
    uint16_t remoteUdpPort(void) const { return m_remote_udp_port; }
    void setRemoteUdpPort(uint16_t port) { m_remote_udp_port = port; }
    const std::string& callsign(void) const { return m_callsign; }
    uint16_t nextUdpTxSeq(void) { return m_next_udp_tx_seq++; }
    uint16_t nextUdpRxSeq(void) { return m_next_udp_rx_seq; }
    void setNextUdpRxSeq(uint16_t seq) { m_next_udp_rx_seq = seq; }
    int sendMsg(const ReflectorMsg& msg);
    void udpMsgReceived(const ReflectorUdpMsg &header);
    void sendUdpMsg(const ReflectorUdpMsg &msg);
    void setBlock(unsigned blocktime);
    bool isBlocked(void) const { return (m_remaining_blocktime > 0); }

  protected:

  private:
    static uint32_t next_client_id;

    typedef enum
    {
      STATE_DISCONNECTED, STATE_EXPECT_PROTO_VER, STATE_EXPECT_AUTH_RESPONSE,
      STATE_CONNECTED, STATE_EXPECT_DISCONNECT
    } ConState;

    static const unsigned HEARTBEAT_TX_CNT_RESET      = 10;
    static const unsigned HEARTBEAT_RX_CNT_RESET      = 15;
    static const unsigned UDP_HEARTBEAT_TX_CNT_RESET  = 15;
    static const unsigned UDP_HEARTBEAT_RX_CNT_RESET  = 120;

    Async::FramedTcpConnection* m_con;
    unsigned              m_msg_type;
    unsigned char         m_auth_challenge[MsgAuthChallenge::CHALLENGE_LEN];
    ConState              m_con_state;
    Async::Timer          m_disc_timer;
    std::string           m_callsign;
    uint32_t              m_client_id;
    uint16_t              m_remote_udp_port;
    Async::Config*        m_cfg;
    uint16_t              m_next_udp_tx_seq;
    uint16_t              m_next_udp_rx_seq;
    Async::Timer          m_heartbeat_timer;
    unsigned              m_heartbeat_tx_cnt;
    unsigned              m_heartbeat_rx_cnt;
    unsigned              m_udp_heartbeat_tx_cnt;
    unsigned              m_udp_heartbeat_rx_cnt;
    Reflector*            m_reflector;
    unsigned              m_blocktime;
    unsigned              m_remaining_blocktime;

    ReflectorClient(const ReflectorClient&);
    ReflectorClient& operator=(const ReflectorClient&);
    void onFrameReceived(Async::FramedTcpConnection *con,
                         std::vector<uint8_t>& data);
    void handleMsgProtoVer(std::istream& is);
    void sendNodeList(void);
    void handleMsgAuthResponse(std::istream& is);
    void disconnect(const std::string& msg);
    //void onDisconnected(Async::FramedTcpConnection*,
    //                    Async::FramedTcpConnection::DisconnectReason);
    void onDiscTimeout(Async::Timer *t);
    void handleHeartbeat(Async::Timer *t);
    std::string lookupUserKey(const std::string& callsign);

};  /* class ReflectorClient */


//} /* namespace */

#endif /* REFLECTOR_CLIENT_INCLUDED */



/*
 * This file has not been truncated
 */
