/**
@file	 ReflectorLogic.h
@brief   A logic core that connect to the SvxReflector
@author  Tobias Blomberg / SM0SVX
@date	 2017-02-12

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
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

#ifndef REFLECTOR_LOGIC_INCLUDED
#define REFLECTOR_LOGIC_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <sys/time.h>
#include <string>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncAudioDecoder.h>
#include <AsyncAudioEncoder.h>
#include <AsyncTcpClient.h>
#include <AsyncFramedTcpConnection.h>
#include <AsyncTimer.h>
#include <AsyncAudioFifo.h>
#include <AsyncAudioPassthrough.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "LogicBase.h"


/****************************************************************************
 *
 * Forward declarations
 *
 ****************************************************************************/

namespace Async
{
  class UdpSocket;
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
@brief	A logic core that connect to the SvxReflector
@author Tobias Blomberg / SM0SVX
@date   2017-02-12
*/
class ReflectorLogic : public LogicBase
{
  public:
    /**
     * @brief 	Constructor
     * @param   cfg A previously initialized configuration object
     * @param   name The name of the logic core
     */
    ReflectorLogic(Async::Config& cfg, const std::string& name);

    /**
     * @brief 	Destructor
     */
    ~ReflectorLogic(void);

    /**
     * @brief 	Initialize the logic core
     * @return	Returns \em true on success or \em false on failure
     */
    virtual bool initialize(void);

    /**
     * @brief 	Get the audio pipe sink used for writing audio into this logic
     * @return	Returns an audio pipe sink object
     */
    virtual Async::AudioSink *logicConIn(void) { return m_logic_con_in; }

    /**
     * @brief 	Get the audio pipe source used for reading audio from this logic
     * @return	Returns an audio pipe source object
     */
    virtual Async::AudioSource *logicConOut(void) { return m_logic_con_out; }

  protected:

  private:
    typedef enum
    {
      STATE_DISCONNECTED, STATE_EXPECT_AUTH_CHALLENGE, STATE_EXPECT_AUTH_OK,
      STATE_EXPECT_SERVER_INFO, STATE_CONNECTED
    } ConState;

    typedef Async::TcpClient<Async::FramedTcpConnection> FramedTcpClient;

    static const unsigned UDP_HEARTBEAT_TX_CNT_RESET = 15;
    static const unsigned UDP_HEARTBEAT_RX_CNT_RESET = 60;
    static const unsigned TCP_HEARTBEAT_TX_CNT_RESET = 10;
    static const unsigned TCP_HEARTBEAT_RX_CNT_RESET = 15;

    std::string               m_reflector_host;
    uint16_t                  m_reflector_port;
    FramedTcpClient*          m_con;
    unsigned                  m_msg_type;
    Async::UdpSocket*         m_udp_sock;
    uint32_t                  m_client_id;
    std::string               m_auth_key;
    std::string               m_callsign;
    Async::AudioPassthrough*  m_logic_con_in;
    Async::AudioSource*       m_logic_con_out;
    Async::Timer              m_reconnect_timer;
    uint16_t                  m_next_udp_tx_seq;
    uint16_t                  m_next_udp_rx_seq;
    Async::Timer              m_heartbeat_timer;
    Async::AudioDecoder*      m_dec;
    Async::Timer              m_flush_timeout_timer;
    unsigned                  m_udp_heartbeat_tx_cnt;
    unsigned                  m_udp_heartbeat_rx_cnt;
    unsigned                  m_tcp_heartbeat_tx_cnt;
    unsigned                  m_tcp_heartbeat_rx_cnt;
    struct timeval            m_last_talker_timestamp;
    ConState                  m_con_state;
    Async::AudioEncoder*      m_enc;

    ReflectorLogic(const ReflectorLogic&);
    ReflectorLogic& operator=(const ReflectorLogic&);
    void onConnected(void);
    void onDisconnected(Async::TcpConnection *con,
                        Async::TcpConnection::DisconnectReason reason);
    void onFrameReceived(Async::FramedTcpConnection *con,
                         std::vector<uint8_t>& data);
    void handleMsgError(std::istream& is);
    void handleMsgAuthChallenge(std::istream& is);
    void handleMsgNodeList(std::istream& is);
    void handleMsgNodeJoined(std::istream& is);
    void handleMsgNodeLeft(std::istream& is);
    void handleMsgTalkerStart(std::istream& is);
    void handleMsgTalkerStop(std::istream& is);
    void handleMsgAuthOk(void);
    void handleMsgServerInfo(std::istream& is);
    void sendMsg(const ReflectorMsg& msg);
    void sendEncodedAudio(const void *buf, int count);
    void flushEncodedAudio(void);
    void udpDatagramReceived(const Async::IpAddress& addr, uint16_t port,
                             void *buf, int count);
    void sendUdpMsg(const ReflectorUdpMsg& msg);
    void connect(void);
    void disconnect(void);
    void reconnect(void);
    void allEncodedSamplesFlushed(void);
    void flushTimeout(Async::Timer *t=0);
    void handleTimerTick(Async::Timer *t);
    bool setAudioCodec(const std::string& codec_name);
    bool codecIsAvailable(const std::string &codec_name);

};  /* class ReflectorLogic */


#endif /* REFLECTOR_LOGIC_INCLUDED */


/*
 * This file has not been truncated
 */
