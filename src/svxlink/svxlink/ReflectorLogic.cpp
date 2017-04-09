/**
@file	 ReflectorLogic.cpp
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



/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <sstream>
#include <iostream>
#include <iomanip>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncTcpClient.h>
#include <AsyncUdpSocket.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "ReflectorLogic.h"
#include "../reflector/ReflectorMsg.h"


/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/

using namespace std;
using namespace Async;



/****************************************************************************
 *
 * Defines & typedefs
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Local class definitions
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Prototypes
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Exported Global Variables
 *
 ****************************************************************************/




/****************************************************************************
 *
 * Local Global Variables
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/

ReflectorLogic::ReflectorLogic(Async::Config& cfg, const std::string& name)
  : LogicBase(cfg, name), m_con(0), m_msg_type(0), m_udp_sock(0),
    m_logic_con_in(0), m_logic_con_out(0),
    m_reconnect_timer(20000, Timer::TYPE_ONESHOT, false),
    m_next_udp_tx_seq(0), m_next_udp_rx_seq(0),
    m_heartbeat_timer(1000, Timer::TYPE_PERIODIC, false), m_dec(0),
    m_flush_timeout_timer(3000, Timer::TYPE_ONESHOT, false),
    m_udp_heartbeat_tx_cnt(0), m_udp_heartbeat_rx_cnt(0),
    m_tcp_heartbeat_tx_cnt(0), m_tcp_heartbeat_rx_cnt(0)
{
  m_reconnect_timer.expired.connect(mem_fun(*this, &ReflectorLogic::reconnect));
  m_heartbeat_timer.expired.connect(
      mem_fun(*this, &ReflectorLogic::handleTimerTick));
  m_flush_timeout_timer.expired.connect(
      mem_fun(*this, &ReflectorLogic::flushTimeout));
  timerclear(&m_last_talker_timestamp);
} /* ReflectorLogic::ReflectorLogic */


ReflectorLogic::~ReflectorLogic(void)
{
  delete m_udp_sock;
  m_udp_sock = 0;
  delete m_logic_con_in;
  m_logic_con_in = 0;
  delete m_dec;
  m_dec = 0;
  delete m_con;
  m_con = 0;
} /* ReflectorLogic::~ReflectorLogic */


bool ReflectorLogic::initialize(void)
{
  string reflector_host;
  if (!cfg().getValue(name(), "HOST", reflector_host))
  {
    cerr << "*** ERROR: " << name() << "/HOST missing in configuration" << endl;
    return false;
  }

  uint16_t reflector_port = 5300;
  cfg().getValue(name(), "PORT", reflector_port);

  if (!cfg().getValue(name(), "CALLSIGN", m_callsign))
  {
    cerr << "*** ERROR: " << name() << "/CALLSIGN missing in configuration"
         << endl;
    return false;
  }

  if (!cfg().getValue(name(), "AUTH_KEY", m_auth_key))
  {
    cerr << "*** ERROR: " << name() << "/AUTH_KEY missing in configuration"
         << endl;
    return false;
  }
  if (m_auth_key == "Change this key now!")
  {
    cerr << "*** ERROR: You must change " << name() << "/AUTH_KEY from the "
            "default value" << endl;
    return false;
  }

  string audio_codec("OPUS");
  cfg().getValue(name(), "AUDIO_CODEC", audio_codec);

  m_logic_con_in = Async::AudioEncoder::create(audio_codec);
  if (m_logic_con_in == 0)
  {
    cerr << "*** ERROR[" << name()
         << "]: Failed to initialize audio encoder" << endl;
    return false;
  }
  m_logic_con_in->writeEncodedSamples.connect(
      mem_fun(*this, &ReflectorLogic::sendEncodedAudio));
  m_logic_con_in->flushEncodedSamples.connect(
      mem_fun(*this, &ReflectorLogic::flushEncodedAudio));


    // Create audio decoder
  m_dec = Async::AudioDecoder::create(audio_codec);
  if (m_dec == 0)
  {
    cerr << "*** ERROR[" << name()
         << "]: Failed to initialize audio decoder" << endl;
    return false;
  }
  m_dec->allEncodedSamplesFlushed.connect(
      mem_fun(*this, &ReflectorLogic::allEncodedSamplesFlushed));
  AudioSource *prev_src = m_dec;

    // Create jitter FIFO if jitter buffer delay > 0
  unsigned jitter_buffer_delay = 0;
  cfg().getValue(name(), "JITTER_BUFFER_DELAY", jitter_buffer_delay);
  if (jitter_buffer_delay > 0)
  {
    AudioFifo *fifo = new Async::AudioFifo(
        2 * jitter_buffer_delay * INTERNAL_SAMPLE_RATE / 1000);
        //new Async::AudioJitterFifo(100 * INTERNAL_SAMPLE_RATE / 1000);
    fifo->setPrebufSamples(jitter_buffer_delay * INTERNAL_SAMPLE_RATE / 1000);
    prev_src->registerSink(fifo, true);
    prev_src = fifo;
  }
  m_logic_con_out = prev_src;

  if (!LogicBase::initialize())
  {
    return false;
  }

  m_con = new TcpClient<FramedTcpConnection>(reflector_host, reflector_port);
  m_con->connected.connect(mem_fun(*this, &ReflectorLogic::onConnected));
  m_con->disconnected.connect(mem_fun(*this, &ReflectorLogic::onDisconnected));
  m_con->frameReceived.connect(mem_fun(*this, &ReflectorLogic::onFrameReceived));
  m_con->connect();

  return true;
} /* ReflectorLogic::initialize */



/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/

void ReflectorLogic::onConnected(void)
{
  cout << name() << ": Connection established to " << m_con->remoteHost() << ":"
       << m_con->remotePort() << endl;
  sendMsg(MsgProtoVer());
  m_udp_heartbeat_tx_cnt = UDP_HEARTBEAT_TX_CNT_RESET;
  m_udp_heartbeat_rx_cnt = UDP_HEARTBEAT_RX_CNT_RESET;
  m_tcp_heartbeat_tx_cnt = TCP_HEARTBEAT_TX_CNT_RESET;
  m_tcp_heartbeat_rx_cnt = TCP_HEARTBEAT_RX_CNT_RESET;
  m_heartbeat_timer.setEnable(true);
  m_next_udp_tx_seq = 0;
  m_next_udp_rx_seq = 0;
  timerclear(&m_last_talker_timestamp);
} /* ReflectorLogic::onConnected */


void ReflectorLogic::onDisconnected(TcpConnection *con,
                                    TcpConnection::DisconnectReason reason)
{
  cout << name() << ": Disconnected from " << m_con->remoteHost() << ":"
       << m_con->remotePort() << ": "
       << TcpConnection::disconnectReasonStr(reason) << endl;
  m_reconnect_timer.setEnable(true);
  delete m_udp_sock;
  m_udp_sock = 0;
  m_next_udp_tx_seq = 0;
  m_next_udp_rx_seq = 0;
  m_heartbeat_timer.setEnable(false);
  if (m_flush_timeout_timer.isEnabled())
  {
    m_flush_timeout_timer.setEnable(false);
    m_logic_con_in->allEncodedSamplesFlushed();
  }
  if (timerisset(&m_last_talker_timestamp))
  {
    m_dec->flushEncodedSamples();
    timerclear(&m_last_talker_timestamp);
  }
} /* ReflectorLogic::onDisconnected */


void ReflectorLogic::onFrameReceived(FramedTcpConnection *con,
                                     std::vector<uint8_t>& data)
{
  char *buf = reinterpret_cast<char*>(&data.front());
  int len = data.size();

  //cout << "### ReflectorLogic::onFrameReceived: len=" << len << endl;

  stringstream ss;
  ss.write(buf, len);

  ReflectorMsg header;
  if (!header.unpack(ss))
  {
    cout << "*** ERROR[" << name()
         << "]: Unpacking failed for TCP message header\n";
    disconnect();
    return;
  }

  m_tcp_heartbeat_rx_cnt = TCP_HEARTBEAT_RX_CNT_RESET;

  switch (header.type())
  {
    case MsgHeartbeat::TYPE:
      //cout << "### " << name() << ": MsgHeartbeat()" << endl;
      break;
    case MsgError::TYPE:
      handleMsgError(ss);
      break;
    case MsgAuthChallenge::TYPE:
      handleMsgAuthChallenge(ss);
      break;
    case MsgAuthOk::TYPE:
      handleMsgAuthOk();
      break;
    case MsgServerInfo::TYPE:
      handleMsgServerInfo(ss);
      break;
    case MsgNodeList::TYPE:
      handleMsgNodeList(ss);
      break;
    case MsgNodeJoined::TYPE:
      handleMsgNodeJoined(ss);
      break;
    case MsgNodeLeft::TYPE:
      handleMsgNodeLeft(ss);
      break;
    case MsgTalkerStart::TYPE:
      handleMsgTalkerStart(ss);
      break;
    case MsgTalkerStop::TYPE:
      handleMsgTalkerStop(ss);
      break;
    default:
      cerr << "*** WARNING[" << name()
           << "]: Unknown protocol message received: msg_type="
           << header.type() << endl;
      break;
  }
} /* ReflectorLogic::onFrameReceived */


void ReflectorLogic::handleMsgError(std::istream& is)
{
  MsgError msg;
  if (!msg.unpack(is))
  {
    cerr << "*** ERROR[" << name() << "]: Could not unpack MsgAuthChallenge\n";
    disconnect();
    return;
  }
  cout << "### " << name() << ": MsgError(\"" << msg.message() << "\")" << endl;
  disconnect();
} /* ReflectorLogic::handleMsgError */


void ReflectorLogic::handleMsgAuthChallenge(std::istream& is)
{
  MsgAuthChallenge msg;
  if (!msg.unpack(is))
  {
    cerr << "*** ERROR[" << name() << "]: Could not unpack MsgAuthChallenge\n";
    disconnect();
    return;
  }
  stringstream ss;
  ss << hex << setw(2) << setfill('0');
  for (int i=0; i<MsgAuthChallenge::CHALLENGE_LEN; ++i)
  {
    ss << (int)msg.challenge()[i];
  }
  cout << "### " << name() << ": MsgAuthChallenge(" << ss.str() << ")" << endl;

  sendMsg(MsgAuthResponse(m_callsign, m_auth_key, msg.challenge()));
} /* ReflectorLogic::handleMsgAuthChallenge */


void ReflectorLogic::handleMsgAuthOk(void)
{
  cout << "### " << name() << ": MsgAuthOk()" << endl;
} /* ReflectorLogic::handleMsgAuthOk */


void ReflectorLogic::handleMsgServerInfo(std::istream& is)
{
  MsgServerInfo msg;
  if (!msg.unpack(is))
  {
    cerr << "*** ERROR[" << name() << "]: Could not unpack MsgAuthChallenge\n";
    disconnect();
    return;
  }
  cout << "### " << name() << ": MsgServerInfo(" << msg.clientId() << ")"
       << endl;
  m_client_id = msg.clientId();

  delete m_udp_sock;
  m_udp_sock = new UdpSocket;
  m_udp_sock->dataReceived.connect(
      mem_fun(*this, &ReflectorLogic::udpDatagramReceived));

  sendUdpMsg(MsgUdpHeartbeat());
} /* ReflectorLogic::handleMsgAuthChallenge */


void ReflectorLogic::handleMsgNodeList(std::istream& is)
{
  MsgNodeList msg;
  if (!msg.unpack(is))
  {
    cerr << "*** ERROR[" << name() << "]: Could not unpack MsgNodeList\n";
    disconnect();
    return;
  }
  cout << "### " << name() << ": MsgNodeList(";
  const vector<string>& nodes = msg.nodes();
  if (!nodes.empty())
  {
    vector<string>::const_iterator it = nodes.begin();
    cout << *it++;
    for (; it != nodes.end(); ++it)
    {
      cout << ", " << *it;
    }
  }
  cout << ")" << endl;
} /* ReflectorLogic::handleMsgNodeList */


void ReflectorLogic::handleMsgNodeJoined(std::istream& is)
{
  MsgNodeJoined msg;
  if (!msg.unpack(is))
  {
    cerr << "*** ERROR[" << name() << "]: Could not unpack MsgNodeJoined\n";
    disconnect();
    return;
  }
  cout << "### " << name() << ": MsgNodeJoined(" << msg.callsign() << ")"
       << endl;

} /* ReflectorLogic::handleMsgNodeJoined */


void ReflectorLogic::handleMsgNodeLeft(std::istream& is)
{
  MsgNodeLeft msg;
  if (!msg.unpack(is))
  {
    cerr << "*** ERROR[" << name() << "]: Could not unpack MsgNodeLeft\n";
    disconnect();
    return;
  }
  cout << "### " << name() << ": MsgNodeLeft(" << msg.callsign() << ")"
       << endl;

} /* ReflectorLogic::handleMsgNodeLeft */


void ReflectorLogic::handleMsgTalkerStart(std::istream& is)
{
  MsgTalkerStart msg;
  if (!msg.unpack(is))
  {
    cerr << "*** ERROR[" << name() << "]: Could not unpack MsgTalkerStart\n";
    disconnect();
    return;
  }
  cout << "### " << name() << ": MsgTalkerStart(" << msg.callsign() << ")"
       << endl;

} /* ReflectorLogic::handleMsgTalkerStart */


void ReflectorLogic::handleMsgTalkerStop(std::istream& is)
{
  MsgTalkerStop msg;
  if (!msg.unpack(is))
  {
    cerr << "*** ERROR[" << name() << "]: Could not unpack MsgTalkerStop\n";
    disconnect();
    return;
  }
  cout << "### " << name() << ": MsgTalkerStop(" << msg.callsign() << ")"
       << endl;

} /* ReflectorLogic::handleMsgTalkerStop */


void ReflectorLogic::sendMsg(const ReflectorMsg& msg)
{
  if (!m_con->isConnected())
  {
    return;
  }

  m_tcp_heartbeat_tx_cnt = TCP_HEARTBEAT_TX_CNT_RESET;

  ostringstream ss;
  ReflectorMsg header(msg.type());
  if (!header.pack(ss) || !msg.pack(ss))
  {
    cerr << "*** ERROR[" << name()
         << "]: Failed to pack reflector TCP message\n";
    disconnect();
    return;
  }
  if (m_con->write(ss.str().data(), ss.str().size()) == -1)
  {
    disconnect();
  }
} /* ReflectorLogic::sendMsg */


void ReflectorLogic::sendEncodedAudio(const void *buf, int count)
{
  //cout << "### " << name() << ": ReflectorLogic::sendEncodedAudio: count="
  //     << count << endl;
  if (!m_con->isConnected())
  {
    return;
  }

  if (m_flush_timeout_timer.isEnabled())
  {
    m_flush_timeout_timer.setEnable(false);
  }
  sendUdpMsg(MsgUdpAudio(buf, count));
} /* ReflectorLogic::sendEncodedAudio */


void ReflectorLogic::flushEncodedAudio(void)
{
  //cout << "### " << name() << ": ReflectorLogic::flushEncodedAudio" << endl;
  if (!m_con->isConnected())
  {
    flushTimeout();
    return;
  }
  sendUdpMsg(MsgUdpFlushSamples());
  m_flush_timeout_timer.setEnable(true);
} /* ReflectorLogic::flushEncodedAudio */


void ReflectorLogic::udpDatagramReceived(const IpAddress& addr, uint16_t port,
                                         void *buf, int count)
{
  //cout << "### " << name() << ": ReflectorLogic::udpDatagramReceived: addr="
  //     << addr << " port=" << port << " count=" << count;

  if (addr != m_con->remoteHost())
  {
    cout << "*** WARNING[" << name()
         << "]: UDP packet received from wrong source address "
         << addr << ". Should be " << m_con->remoteHost() << "." << endl;
    return;
  }
  if (port != m_con->remotePort())
  {
    cout << "*** WARNING[" << name()
         << "]: UDP packet received with wrong source port number "
         << port << ". Should be " << m_con->remotePort() << "." << endl;
    return;
  }

  stringstream ss;
  ss.write(reinterpret_cast<const char *>(buf), count);

  ReflectorUdpMsg header;
  if (!header.unpack(ss))
  {
    cout << "*** WARNING[" << name()
         << "]: Unpacking failed for UDP message header" << endl;
    return;
  }

  //cout << " msg_type=" << header.type()
  //     << " client_id=" << header.clientId()
  //     << " seq=" << header.sequenceNum()
  //     << std::endl;

  if (header.clientId() != m_client_id)
  {
    cout << "*** WARNING[" << name()
         << "]: UDP packet received with wrong client id "
         << header.clientId() << ". Should be " << m_client_id << "." << endl;
    return;
  }

    // Check sequence number
  uint16_t udp_rx_seq_diff = header.sequenceNum() - m_next_udp_rx_seq;
  if (udp_rx_seq_diff > 0x7fff) // Frame out of sequence (ignore)
  {
    cout << "### " << name()
         << ": Dropping out of sequence UDP frame with seq="
         << header.sequenceNum() << endl;
    return;
  }
  else if (udp_rx_seq_diff > 0) // Frame lost
  {
    cout << "### " << name() << ": UDP frame(s) lost. Expected seq="
         << m_next_udp_rx_seq
         << " but received " << header.sequenceNum()
         << ". Resetting next expected sequence number to "
         << (header.sequenceNum() + 1) << endl;
  }
  m_next_udp_rx_seq = header.sequenceNum() + 1;

  m_udp_heartbeat_rx_cnt = UDP_HEARTBEAT_RX_CNT_RESET;

  switch (header.type())
  {
    case MsgUdpHeartbeat::TYPE:
      //cout << "### " << name() << ": MsgUdpHeartbeat()" << endl;
      break;

    case MsgUdpAudio::TYPE:
    {
      MsgUdpAudio msg;
      if (!msg.unpack(ss))
      {
        cerr << "*** WARNING[" << name() << "]: Could not unpack MsgUdpAudio\n";
        return;
      }
      if (!msg.audioData().empty())
      {
        gettimeofday(&m_last_talker_timestamp, NULL);
        m_dec->writeEncodedSamples(
            &msg.audioData().front(), msg.audioData().size());
      }
      break;
    }

    case MsgUdpFlushSamples::TYPE:
      //cout << "### " << name() << ": MsgUdpFlushSamples()" << endl;
      m_dec->flushEncodedSamples();
      timerclear(&m_last_talker_timestamp);
      break;

    case MsgUdpAllSamplesFlushed::TYPE:
      //cout << "### " << name() << ": MsgUdpAllSamplesFlushed()" << endl;
      m_logic_con_in->allEncodedSamplesFlushed();
      break;

    default:
      cerr << "*** WARNING[" << name()
           << "]: Unknown UDP protocol message received: msg_type="
           << header.type() << endl;
      break;
  }
} /* ReflectorLogic::udpDatagramReceived */


void ReflectorLogic::sendUdpMsg(const ReflectorUdpMsg& msg)
{
  m_udp_heartbeat_tx_cnt = UDP_HEARTBEAT_TX_CNT_RESET;

  if (m_udp_sock == 0)
  {
    return;
  }

  ReflectorUdpMsg header(msg.type(), m_client_id, m_next_udp_tx_seq++);
  ostringstream ss;
  if (!header.pack(ss) || !msg.pack(ss))
  {
    cerr << "*** ERROR[" << name()
         << "]: Failed to pack reflector TCP message\n";
    return;
  }
  m_udp_sock->write(m_con->remoteHost(), m_con->remotePort(),
                    ss.str().data(), ss.str().size());
} /* ReflectorLogic::sendUdpMsg */


void ReflectorLogic::reconnect(Timer *t)
{
  cout << "### " << name() << ": Reconnecting to reflector server\n";
  t->setEnable(false);
  m_con->connect();
} /* ReflectorLogic::reconnect */


void ReflectorLogic::disconnect(void)
{
  m_con->disconnect();
  onDisconnected(m_con, TcpConnection::DR_ORDERED_DISCONNECT);
} /* ReflectorLogic::disconnect */


void ReflectorLogic::allEncodedSamplesFlushed(void)
{
  sendUdpMsg(MsgUdpAllSamplesFlushed());
} /* ReflectorLogic::allEncodedSamplesFlushed */


void ReflectorLogic::flushTimeout(Async::Timer *t)
{
  m_flush_timeout_timer.setEnable(false);
  m_logic_con_in->allEncodedSamplesFlushed();
} /* ReflectorLogic::flushTimeout */


void ReflectorLogic::handleTimerTick(Async::Timer *t)
{
  if (timerisset(&m_last_talker_timestamp))
  {
    struct timeval now, diff;
    gettimeofday(&now, NULL);
    timersub(&now, &m_last_talker_timestamp, &diff);
    if (diff.tv_sec > 3)
    {
      cout << "### " << name() << ": Talker audio timeout" << endl;
      m_dec->flushEncodedSamples();
      timerclear(&m_last_talker_timestamp);
    }
  }

  if (--m_udp_heartbeat_tx_cnt == 0)
  {
    sendUdpMsg(MsgUdpHeartbeat());
  }

  if (--m_tcp_heartbeat_tx_cnt == 0)
  {
    sendMsg(MsgHeartbeat());
  }

  if (--m_udp_heartbeat_rx_cnt == 0)
  {
    cout << name() << ": UDP Heartbeat timeout" << endl;
    disconnect();
  }

  if (--m_tcp_heartbeat_rx_cnt == 0)
  {
    cout << name() << ": Heartbeat timeout" << endl;
    disconnect();
  }
} /* ReflectorLogic::handleTimerTick */


/*
 * This file has not been truncated
 */
