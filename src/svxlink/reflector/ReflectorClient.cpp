/**
@file	 ReflectorClient.cpp
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

/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <sstream>
#include <cassert>
#include <iomanip>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncTimer.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "ReflectorClient.h"
#include "Reflector.h"



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

uint32_t ReflectorClient::next_client_id = 0;


/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/

ReflectorClient::ReflectorClient(Reflector *ref, Async::TcpConnection *con,
                                 const std::string& auth_key)
  : m_con(con), m_msg_type(0), m_con_state(STATE_EXPECT_PROTO_VER),
    m_disc_timer(10000, Timer::TYPE_ONESHOT, false),
    m_client_id(next_client_id++), m_remote_udp_port(0), m_auth_key(auth_key),
    m_next_udp_tx_seq(0), m_next_udp_rx_seq(0),
    m_heartbeat_timer(1000, Timer::TYPE_PERIODIC),
    m_heartbeat_tx_cnt(HEARTBEAT_TX_CNT_RESET),
    m_heartbeat_rx_cnt(HEARTBEAT_RX_CNT_RESET),
    m_udp_heartbeat_tx_cnt(UDP_HEARTBEAT_TX_CNT_RESET),
    m_udp_heartbeat_rx_cnt(UDP_HEARTBEAT_RX_CNT_RESET),
    m_reflector(ref)
{
  m_con->dataReceived.connect(mem_fun(*this, &ReflectorClient::onDataReceived));
  //m_con->disconnected.connect(mem_fun(*this, &ReflectorClient::onDisconnected));
  m_disc_timer.expired.connect(mem_fun(*this, &ReflectorClient::onDiscTimeout));
  m_heartbeat_timer.expired.connect(
      mem_fun(*this, &ReflectorClient::handleHeartbeat));
} /* ReflectorClient::ReflectorClient */


ReflectorClient::~ReflectorClient(void)
{
} /* ReflectorClient::~ReflectorClient */


void ReflectorClient::sendMsg(const ReflectorMsg& msg)
{
  if (((m_con_state != STATE_CONNECTED) && (msg.type() >= 100)) ||
      !m_con->isConnected())
  {
    return;
  }

  m_heartbeat_tx_cnt = HEARTBEAT_TX_CNT_RESET;

  ReflectorMsg header(msg.type(), msg.packedSize());
  ostringstream ss;
  if (!header.pack(ss) || !msg.pack(ss))
  {
    // FIXME: Better error handling
    cerr << "*** ERROR: Failed to pack TCP message\n";
    return;
  }
  m_con->write(ss.str().data(), ss.str().size());
} /* ReflectorClient::sendMsg */


void ReflectorClient::udpMsgReceived(void)
{
  m_udp_heartbeat_rx_cnt = UDP_HEARTBEAT_RX_CNT_RESET;
} /* ReflectorClient::udpMsgReceived */


void ReflectorClient::sendUdpMsg(const ReflectorUdpMsg &msg)
{
  if (remoteUdpPort() == 0)
  {
    return;
  }

  //cout << "### ReflectorClient::sendUdpMsg: " << client->remoteHost() << ":"
  //     << client->remoteUdpPort() << endl;

  m_udp_heartbeat_tx_cnt = UDP_HEARTBEAT_TX_CNT_RESET;

  ReflectorUdpMsg header(msg.type(), clientId(), nextUdpTxSeq());
  ostringstream ss;
  if (!header.pack(ss) || !msg.pack(ss))
  {
    // FIXME: Better error handling
    cerr << "*** ERROR: Failed to pack reflector UDP message\n";
    return;
  }
  m_reflector->sendUdpDatagram(this, ss.str().data(), ss.str().size());
} /* ReflectorClient::sendUdpMsg */




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

int ReflectorClient::onDataReceived(TcpConnection *con, void *data, int len)
{
  //cout << "### ReflectorClient::onDataReceived: len=" << len << endl;

  assert(len >= 0);

  if ((m_con_state == STATE_DISCONNECTED) ||
      (m_con_state == STATE_EXPECT_DISCONNECT))
  {
    return len;
  }

  char *buf = reinterpret_cast<char*>(data);
  int tot_consumed = 0;
  while ((len > 0) && (m_con_state != STATE_DISCONNECTED))
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

    m_heartbeat_rx_cnt = HEARTBEAT_RX_CNT_RESET;

    switch (header.type())
    {
      case MsgHeartbeat::TYPE:
        //cout << "### " << callsign() << ": MsgHeartbeat()" << endl;
        break;
      case MsgProtoVer::TYPE:
        handleMsgProtoVer(ss);
        break;
      case MsgAuthResponse::TYPE:
        handleMsgAuthResponse(ss);
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
} /* ReflectorClient::onDataReceived */


void ReflectorClient::handleMsgProtoVer(std::istream& is)
{
  if (m_con_state != STATE_EXPECT_PROTO_VER)
  {
    disconnect("Protocol version expected");
    return;
  }

  MsgProtoVer msg;
  if (!msg.unpack(is))
  {
    // FIXME: Disconnect
    cerr << "*** ERROR: Could not unpack MsgProtoVer\n";
    return;
  }
  cout << "### " << m_con->remoteHost() << ":" << m_con->remotePort()
       << ": MsgProtoVer(" << msg.majorVer() << ", " << msg.minorVer()
       << ")" << endl;
  if ((msg.majorVer() != MsgProtoVer::MAJOR) ||
      (msg.minorVer() != MsgProtoVer::MINOR))
  {
    cerr << "*** ERROR: Incompatible protocol version: "
         << msg.majorVer() << "." << msg.minorVer() << ". Should be "
         << MsgProtoVer::MAJOR << "." << MsgProtoVer::MINOR << endl;
    stringstream ss;
    ss << "Unsupported protocol version " << msg.majorVer() << "."
       << msg.minorVer();
    disconnect(ss.str());
    return;
  }

  MsgAuthChallenge challenge_msg;
  memcpy(m_auth_challenge, challenge_msg.challenge(),
         MsgAuthChallenge::CHALLENGE_LEN);
  sendMsg(challenge_msg);
  m_con_state = STATE_EXPECT_AUTH_RESPONSE;
} /* ReflectorClient::handleMsgProtoVer */


void ReflectorClient::handleMsgAuthResponse(std::istream& is)
{
  if (m_con_state != STATE_EXPECT_AUTH_RESPONSE)
  {
    disconnect("Authentication response expected");
    return;
  }

  MsgAuthResponse msg;
  if (!msg.unpack(is))
  {
    // FIXME: Disconnect
    cerr << "*** ERROR: Could not unpack MsgAuthResponse\n";
    return;
  }

  stringstream ss;
  ss << hex << setw(2) << setfill('0');
  for (int i=0; i<MsgAuthResponse::DIGEST_LEN; ++i)
  {
    ss << (int)msg.digest()[i];
  }
  cout << "### " << msg.callsign() << ": MsgAuthResponse(" << ss.str() << ")"
       << endl;

  if (msg.verify(m_auth_key, m_auth_challenge))
  {
    m_callsign = msg.callsign();
    sendMsg(MsgAuthOk());
    cout << m_callsign << ": Login OK from "
         << m_con->remoteHost() << ":" << m_con->remotePort()
         << endl;
    m_con_state = STATE_CONNECTED;
    sendMsg(MsgServerInfo(m_client_id));
    sendNodeList();
    m_reflector->broadcastMsgExcept(MsgNodeJoined(m_callsign), this);
  }
  else
  {
    cout << msg.callsign() << ": Access denied" << endl;
    disconnect("Access denied");
  }
} /* ReflectorClient::handleMsgProtoVer */


void ReflectorClient::sendNodeList(void)
{
  MsgNodeList msg;
  m_reflector->nodeList(msg.nodes());
  sendMsg(msg);
} /* ReflectorClient::sendNodeList */


void ReflectorClient::disconnect(const std::string& msg)
{
  cout << "### ReflectorClient::disconnect: " << msg << endl;
  sendMsg(MsgError(msg));
  m_heartbeat_timer.setEnable(false);
  m_remote_udp_port = 0;
  m_disc_timer.setEnable(true);
  m_con_state = STATE_EXPECT_DISCONNECT;
} /* ReflectorClient::disconnect */


#if 0
void ReflectorClient::onDisconnected(TcpConnection* con,
                                     TcpConnection::DisconnectReason)
{
  if (!m_callsign.empty())
  {
    cout << m_callsign << ": ";
  }
  cout << "Client " << con->remoteHost() << ":" << con->remotePort()
       << " disconnected" << endl;
  m_heartbeat_timer.setEnable(false);
  m_remote_udp_port = 0;
  m_disc_timer.setEnable(false);
  m_con_state = STATE_DISCONNECTED;
} /* ReflectorClient::onDisconnected */
#endif


void ReflectorClient::onDiscTimeout(Timer *t)
{
  cout << "### ReflectorClient::onDiscTimeout" << endl;
  assert(m_con_state == STATE_EXPECT_DISCONNECT);
  m_con->disconnect();
  m_con_state = STATE_DISCONNECTED;
  m_con->disconnected(m_con, TcpConnection::DR_ORDERED_DISCONNECT);
} /* ReflectorClient::onDiscTimeout */


void ReflectorClient::handleHeartbeat(Async::Timer *t)
{
  if (--m_heartbeat_tx_cnt == 0)
  {
    sendMsg(MsgHeartbeat());
  }

  if (--m_udp_heartbeat_tx_cnt == 0)
  {
    sendUdpMsg(MsgUdpHeartbeat());
  }

  if (--m_heartbeat_rx_cnt == 0)
  {
    cout << callsign() << ": Heartbeat timeout" << endl;
    disconnect("Heartbeat timeout");
  }

  if (--m_udp_heartbeat_rx_cnt == 0)
  {
    cout << callsign() << ": UDP heartbeat timeout" << endl;
    disconnect("UDP heartbeat timeout");
  }
} /* ReflectorClient::handleHeartbeat */


/*
 * This file has not been truncated
 */

