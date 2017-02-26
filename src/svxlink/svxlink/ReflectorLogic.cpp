/**
@file	 ReflectorLogic.cpp
@brief   A_brief_description_for_this_file
@author  Tobias Blomberg / SM0SVX
@date	 2017-02-12

A_detailed_description_for_this_file

\verbatim
<A brief description of the program or library this file belongs to>
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
#include <AsyncAudioDebugger.h>


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
  : LogicBase(cfg, name), m_msg_type(0), m_udp_sock(0), m_logic_con_in(0),
    m_logic_con_out(0), m_reconnect_timer(10000, Timer::TYPE_ONESHOT, false),
    m_next_udp_tx_seq(0), m_next_udp_rx_seq(0),
    m_udp_heartbeat_timer(60000, Timer::TYPE_PERIODIC, false)
{
  m_reconnect_timer.expired.connect(mem_fun(*this, &ReflectorLogic::reconnect));
  m_udp_heartbeat_timer.expired.connect(
      mem_fun(*this, &ReflectorLogic::sendUdpHeartbeat));
} /* ReflectorLogic::ReflectorLogic */


ReflectorLogic::~ReflectorLogic(void)
{
  delete m_udp_sock;
  delete m_logic_con_in;
  delete m_logic_con_out;
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

  if (!cfg().getValue(name(), "PASSWORD", m_reflector_password))
  {
    cerr << "*** ERROR: " << name() << "/PASSWORD missing in configuration"
         << endl;
    return false;
  }

  string audio_codec("OPUS");
  cfg().getValue(name(), "AUDIO_CODEC", audio_codec);

  m_logic_con_in = Async::AudioEncoder::create(audio_codec);
  if (m_logic_con_in == 0)
  {
    cerr << "*** ERROR: Failed to initialize audio encoder" << endl;
    return false;
  }
  m_logic_con_in->writeEncodedSamples.connect(
      mem_fun(*this, &ReflectorLogic::sendEncodedAudio));
  m_logic_con_in->flushEncodedSamples.connect(
      mem_fun(*this, &ReflectorLogic::flushEncodedAudio));
  m_logic_con_out = Async::AudioDecoder::create(audio_codec);
  if (m_logic_con_out == 0)
  {
    cerr << "*** ERROR: Failed to initialize audio decoder" << endl;
    return false;
  }

  if (!LogicBase::initialize())
  {
    return false;
  }

  m_con = new TcpClient(reflector_host, reflector_port);
  m_con->connected.connect(mem_fun(*this, &ReflectorLogic::onConnected));
  m_con->disconnected.connect(mem_fun(*this, &ReflectorLogic::onDisconnected));
  m_con->dataReceived.connect(mem_fun(*this, &ReflectorLogic::onDataReceived));
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
  MsgProtoVer msg;
  sendMsg(msg);
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
  m_udp_heartbeat_timer.setEnable(false);
} /* ReflectorLogic::onDisconnected */


int ReflectorLogic::onDataReceived(TcpConnection *con, void *data, int len)
{
  //cout << "### ReflectorLogic::onDataReceived: len=" << len << endl;

  char *buf = reinterpret_cast<char*>(data);
  int tot_consumed = 0;
  while (len > 0)
  {
    ReflectorMsg header;
    size_t msg_tot_size = header.packedSize();
    if (static_cast<size_t>(len) < msg_tot_size)
    {
      cout << "### Header data underflow\n";
      return tot_consumed;
    }

    stringstream ss;
    ss.write(buf, len);

    if (!header.unpack(ss))
    {
      // FIXME: Disconnect
      cout << "*** ERROR: Packing failed for TCP message header\n";
      return tot_consumed;
    }

    msg_tot_size += header.size();
    if (static_cast<size_t>(len) < msg_tot_size)
    {
      cout << "### Payload data underflow\n";
      return tot_consumed;
    }

    switch (header.type())
    {
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
      default:
        cerr << "*** WARNING: Unknown protocol message received: msg_type="
             << header.type() << endl;
        break;
    }

    buf += msg_tot_size;
    len -= msg_tot_size;
    tot_consumed += msg_tot_size;
  }

  return tot_consumed;
} /* ReflectorLogic::onDataReceived */


void ReflectorLogic::handleMsgError(std::istream& is)
{
  MsgError msg;
  if (!msg.unpack(is))
  {
    // FIXME: Disconnect
    cerr << "*** ERROR: Could not unpack MsgAuthChallenge\n";
    return;
  }
  cout << "### " << name() << ": MsgError(\"" << msg.message() << "\")" << endl;
  // FIXME: Handle reconnection
  disconnect();
} /* ReflectorLogic::handleMsgError */


void ReflectorLogic::handleMsgAuthChallenge(std::istream& is)
{
  MsgAuthChallenge msg;
  if (!msg.unpack(is))
  {
    // FIXME: Disconnect
    cerr << "*** ERROR: Could not unpack MsgAuthChallenge\n";
    return;
  }
  stringstream ss;
  ss << hex << setw(2) << setfill('0');
  for (int i=0; i<MsgAuthChallenge::CHALLENGE_LEN; ++i)
  {
    ss << (int)msg.challenge()[i];
  }
  cout << "### " << name() << ": MsgAuthChallenge(" << ss.str() << ")" << endl;

  MsgAuthResponse response_msg(m_callsign, m_reflector_password,
                               msg.challenge());
  sendMsg(response_msg);
} /* ReflectorLogic::handleMsgAuthChallenge */


void ReflectorLogic::handleMsgAuthOk(void)
{
  cout << "### " << name() << ": MsgAuthOk()" << endl;
} /* ReflectorLogic::handleMsgAuthChallenge */


void ReflectorLogic::handleMsgServerInfo(std::istream& is)
{
  MsgServerInfo msg;
  if (!msg.unpack(is))
  {
    // FIXME: Disconnect
    cerr << "*** ERROR: Could not unpack MsgAuthChallenge\n";
    return;
  }
  cout << "### " << name() << ": MsgServerInfo(" << msg.clientId() << ")"
       << endl;
  m_client_id = msg.clientId();

  delete m_udp_sock;
  m_udp_sock = new UdpSocket;
  m_udp_sock->dataReceived.connect(
      mem_fun(*this, &ReflectorLogic::udpDatagramReceived));

  sendUdpHeartbeat();
  m_udp_heartbeat_timer.setEnable(true);
} /* ReflectorLogic::handleMsgAuthChallenge */


void ReflectorLogic::sendMsg(ReflectorMsg& msg)
{
  ostringstream ss;
  msg.setSize(msg.packedSize());
  if (!msg.ReflectorMsg::pack(ss) || !msg.pack(ss))
  {
    // FIXME: Better error handling
    cerr << "*** ERROR: Failed to pack reflector TCP message\n";
    return;
  }
  m_con->write(ss.str().data(), ss.str().size());
} /* ReflectorLogic::sendMsg */


void ReflectorLogic::sendEncodedAudio(const void *buf, int count)
{
  //cout << "### " << name() << ": ReflectorLogic::sendEncodedAudio: count="
  //     << count << endl;
  sendUdpMsg(MsgAudio(buf, count));
} /* ReflectorLogic::sendEncodedAudio */


void ReflectorLogic::flushEncodedAudio(void)
{
  //cout << "### " << name() << ": ReflectorLogic::flushEncodedAudio" << endl;
  sendUdpMsg(MsgAudio());
  m_logic_con_in->allEncodedSamplesFlushed();
} /* ReflectorLogic::sendEncodedAudio */


void ReflectorLogic::udpDatagramReceived(const IpAddress& addr, uint16_t port,
                                         void *buf, int count)
{
  //cout << "### " << name() << ": ReflectorLogic::udpDatagramReceived: addr="
  //     << addr << " port=" << port << " count=" << count;

  stringstream ss;
  ss.write(reinterpret_cast<const char *>(buf), count);

  ReflectorUdpMsg header;
  if (!header.unpack(ss))
  {
    // FIXME: Disconnect
    cout << "*** ERROR: Unpacking failed for UDP message header\n";
    return;
  }

  //cout << " msg_type=" << header.type()
  //     << " client_id=" << header.clientId()
  //     << " seq=" << header.sequenceNum()
  //     << std::endl;

  // FIXME: Check remote IP and port number. Maybe also client ID?

    // Check sequence number
  uint16_t udp_rx_seq_diff = header.sequenceNum() - m_next_udp_rx_seq++;
  if (udp_rx_seq_diff > 0x7fff) // Frame out of sequence (ignore)
  {
    cout << "### Dropping out of sequence frame with seq="
         << header.sequenceNum() << endl;
    return;
  }
  else if (udp_rx_seq_diff > 0) // Frame lost
  {
    cout << "### Frame(s) lost. Resetting next expected sequence number to "
         << (header.sequenceNum() + 1) << endl;
    m_next_udp_rx_seq = header.sequenceNum() + 1;
  }

  switch (header.type())
  {
    case MsgUdpHeartbeat::TYPE:
      cout << "### " << name() << ": MsgUdpHeartbeat()" << endl;
      // FIXME: Handle heartbeat
      break;
    case MsgAudio::TYPE:
    {
      MsgAudio msg;
      msg.unpack(ss);
      if (msg.audioData().empty())
      {
        m_logic_con_out->flushEncodedSamples();
      }
      else
      {
        m_logic_con_out->writeEncodedSamples(
            &msg.audioData().front(), msg.audioData().size());
      }
      break;
    }
    default:
      cerr << "*** WARNING: Unknown UDP protocol message received: msg_type="
           << header.type() << endl;
      // FIXME: Disconnect or ignore?
      break;
  }
} /* ReflectorLogic::udpDatagramReceived */


void ReflectorLogic::sendUdpMsg(const ReflectorUdpMsg& msg)
{
  if (m_udp_sock == 0)
  {
    return;
  }

  ReflectorUdpMsg header(msg.type(), m_client_id, m_next_udp_tx_seq++);
  ostringstream ss;
  if (!header.pack(ss) || !msg.pack(ss))
  {
    // FIXME: Better error handling
    cerr << "*** ERROR: Failed to pack reflector TCP message\n";
    return;
  }
  m_udp_sock->write(m_con->remoteHost(), m_con->remotePort(),
                    ss.str().data(), ss.str().size());
} /* ReflectorLogic::sendUdpMsg */


void ReflectorLogic::reconnect(Timer *t)
{
  cout << "### Reconnecting to reflector server\n";
  t->setEnable(false);
  m_con->connect();
} /* ReflectorLogic::reconnect */


void ReflectorLogic::disconnect(void)
{
  m_con->disconnect();
  onDisconnected(m_con, TcpConnection::DR_ORDERED_DISCONNECT);
} /* ReflectorLogic::disconnect */


void ReflectorLogic::sendUdpHeartbeat(Async::Timer *t)
{
  sendUdpMsg(MsgUdpHeartbeat());
} /* ReflectorLogic::sendUdpHeartbeat */


/*
 * This file has not been truncated
 */

