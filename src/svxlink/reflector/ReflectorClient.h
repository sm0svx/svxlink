/**
@file	 ReflectorClient.h
@brief   Represents one client connection
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
@brief	Represents one client connection
@author Tobias Blomberg / SM0SVX
@date   2017-02-11

This class represents one client connection. When a client connects, an
instance of this class will be created that will persist for the lifetime of
the client connection.
*/
class ReflectorClient
{
  public:
    typedef enum
    {
      STATE_DISCONNECTED, STATE_EXPECT_PROTO_VER, STATE_EXPECT_AUTH_RESPONSE,
      STATE_CONNECTED, STATE_EXPECT_DISCONNECT
    } ConState;

    /**
     * @brief 	Constructor
     * @param   ref The associated Reflector object
     * @param   con The associated FramedTcpConnection object
     * @param   cfg The associated configuration file object
     */
    ReflectorClient(Reflector *ref, Async::FramedTcpConnection *con,
                    Async::Config* cfg);

    /**
     * @brief 	Destructor
     */
    ~ReflectorClient(void);

    /**
     * @brief 	Return the client ID
     * @return	Returns the client ID
     *
     * The client ID is a unique number assigned to each connected client.
     * It is for example used to associate incoming audio with the correct
     * client.
     */
    uint32_t clientId(void) const { return m_client_id; }

    /**
     * @brief   Return the remote IP address
     * @return  Returns the IP address of the client
     */
    const Async::IpAddress& remoteHost(void) const
    {
      return m_con->remoteHost();
    }

    /**
     * @brief   Return the remote port number
     * @return  Returns the local port number used by the client
     */
    uint16_t remoteUdpPort(void) const { return m_remote_udp_port; }

    /**
     * @brief   Set the remote port number
     * @param   The port number used by the client
     *
     * The Reflector use this function to set the port number used by the
     * client so that UDP packets can be send to the client and check that
     * incoming packets originate from the correct port.
     */
    void setRemoteUdpPort(uint16_t port) { m_remote_udp_port = port; }

    /**
     * @brief   Get the callsign for this connection
     * @return  Returns the callsign associated with this coinnection
     */
    const std::string& callsign(void) const { return m_callsign; }

    /**
     * @brief   Return the next UDP packet transmit sequence number
     * @return  Returns the UDP packet sequence number that should be used next
     *
     * This function will return the UDP packet sequence number that should be
     * used next. The squence number is incremented when this function is called
     * so it can only be called one time per packet. The sequence number is
     * used by the receiver to find out if a packet is out of order or if a
     * packet has been lost in transit.
     */
    uint16_t nextUdpTxSeq(void) { return m_next_udp_tx_seq++; }

    /**
     * @brief   Get the next expected UDP packet sequence number
     * @return  Returns the next expected UDP packet sequence number
     *
     * This function will return the next expected UDP sequence number, which
     * is simply the previously received sequence number plus one.
     */
    uint16_t nextUdpRxSeq(void) { return m_next_udp_rx_seq; }

    /**
     * @brief   Send a TCP message to the remote end
     * @param   The mesage to send
     * @return  On success 0 is returned or else -1
     */
    int sendMsg(const ReflectorMsg& msg);

    /**
     * @brief   Handle a received UDP message
     * @param   The received UDP message
     *
     * This function is called by the Reflector when a UDP packet is received.
     * The purpose is to handle packet related timers and sequence numbers.
     */
    void udpMsgReceived(const ReflectorUdpMsg &header);

    /**
     * @brief   Send a UDP message to the client
     * @param   The message to send
     */
    void sendUdpMsg(const ReflectorUdpMsg &msg);

    /**
     * @brief   Block client audio for the specified time
     * @param   The number of seconds to block
     *
     * This function is used to block the client from sending audio for the
     * specified time. This is used by the Reflector if a client has been
     * talking for too long.
     */
    void setBlock(unsigned blocktime);

    /**
     * @brief   Check if a client is blocked
     * @return  Returns \em true if the client is blocked or else \em false
     */
    bool isBlocked(void) const { return (m_remaining_blocktime > 0); }

    /**
     * @brief   Get the state of the connection
     * @return  Returns the state of the connection
     */
    ConState conState(void) const { return m_con_state; }

  private:
    struct ProtoVer
    {
      uint16_t major_ver;
      uint16_t minor_ver;

      ProtoVer(void) : major_ver(0), minor_ver(0) {}
      ProtoVer(uint16_t major_ver, uint16_t minor_ver)
        : major_ver(major_ver), minor_ver(minor_ver) {}
      bool operator ==(const ProtoVer& rhs) const
      {
        return (major_ver == rhs.major_ver) && (minor_ver == rhs.minor_ver);
      }
      bool operator !=(const ProtoVer& rhs) const
      {
        return !(major_ver == rhs.major_ver);
      }
      bool operator <(const ProtoVer& rhs) const
      {
        return (major_ver < rhs.major_ver) ||
               ((major_ver == rhs.major_ver) && (minor_ver < rhs.minor_ver));
      }
      bool operator >(const ProtoVer& rhs) const
      {
        return (major_ver > rhs.major_ver) ||
               ((major_ver == rhs.major_ver) && (minor_ver > rhs.minor_ver));
      }
      bool operator <=(const ProtoVer& rhs) const
      {
        return (*this == rhs) || (*this < rhs);
      }
      bool operator >=(const ProtoVer& rhs) const
      {
        return (*this == rhs) || (*this > rhs);
      }
    };

    static const uint16_t MIN_MAJOR_VER = 0;
    static const uint16_t MIN_MINOR_VER = 6;
    static uint32_t next_client_id;

    static const unsigned HEARTBEAT_TX_CNT_RESET      = 10;
    static const unsigned HEARTBEAT_RX_CNT_RESET      = 15;
    static const unsigned UDP_HEARTBEAT_TX_CNT_RESET  = 15;
    static const unsigned UDP_HEARTBEAT_RX_CNT_RESET  = 120;

    Async::FramedTcpConnection* m_con;
    unsigned                  m_msg_type;
    unsigned char             m_auth_challenge[MsgAuthChallenge::CHALLENGE_LEN];
    ConState                  m_con_state;
    Async::Timer              m_disc_timer;
    std::string               m_callsign;
    uint32_t                  m_client_id;
    uint16_t                  m_remote_udp_port;
    Async::Config*            m_cfg;
    uint16_t                  m_next_udp_tx_seq;
    uint16_t                  m_next_udp_rx_seq;
    Async::Timer              m_heartbeat_timer;
    unsigned                  m_heartbeat_tx_cnt;
    unsigned                  m_heartbeat_rx_cnt;
    unsigned                  m_udp_heartbeat_tx_cnt;
    unsigned                  m_udp_heartbeat_rx_cnt;
    Reflector*                m_reflector;
    unsigned                  m_blocktime;
    unsigned                  m_remaining_blocktime;
    ProtoVer                  m_client_proto_ver;
    std::vector<std::string>  m_supported_codecs;

    ReflectorClient(const ReflectorClient&);
    ReflectorClient& operator=(const ReflectorClient&);
    void onFrameReceived(Async::FramedTcpConnection *con,
                         std::vector<uint8_t>& data);
    void handleMsgProtoVer(std::istream& is);
    void handleMsgAuthResponse(std::istream& is);
    void handleMsgError(std::istream& is);
    void sendError(const std::string& msg);
    void onDiscTimeout(Async::Timer *t);
    void disconnect(void);
    void handleHeartbeat(Async::Timer *t);
    std::string lookupUserKey(const std::string& callsign);

};  /* class ReflectorClient */


#endif /* REFLECTOR_CLIENT_INCLUDED */


/*
 * This file has not been truncated
 */
