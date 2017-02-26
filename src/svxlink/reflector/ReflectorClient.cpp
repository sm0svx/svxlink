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

ReflectorClient::ReflectorClient(Async::TcpConnection *con)
  : m_con(con), m_msg_type(0), m_con_state(STATE_EXPECT_PROTO_VER),
    m_disc_timer(10000, Timer::TYPE_ONESHOT, false),
    m_client_id(next_client_id++), m_remote_udp_port(0)
{
  m_con->dataReceived.connect(mem_fun(*this, &ReflectorClient::onDataReceived));
  m_con->disconnected.connect(mem_fun(*this, &ReflectorClient::onDisconnected));
  m_disc_timer.expired.connect(mem_fun(*this, &ReflectorClient::onDiscTimeout));
} /* ReflectorClient::ReflectorClient */


ReflectorClient::~ReflectorClient(void)
{
} /* ReflectorClient::~ReflectorClient */




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

  if (m_con_state == STATE_DISCONNECTED)
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

    switch (header.type())
    {
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
  m_callsign = msg.callsign();

  stringstream ss;
  ss << hex << setw(2) << setfill('0');
  for (int i=0; i<MsgAuthResponse::DIGEST_LEN; ++i)
  {
    ss << (int)msg.digest()[i];
  }
  cout << "### " << m_callsign << ": MsgAuthResponse(" << ss.str() << ")"
       << endl;

  if (msg.verify("ThePassword :-)", m_auth_challenge))
  {
    sendMsg(MsgAuthOk());
    cout << m_callsign << ": Login OK from "
         << m_con->remoteHost() << ":" << m_con->remotePort()
         << endl;
    m_con_state = STATE_CONNECTED;
    sendMsg(MsgServerInfo(m_client_id));
  }
  else
  {
    cout << m_callsign << ": Access denied" << endl;
    disconnect("Access denied");
  }
} /* ReflectorClient::handleMsgProtoVer */


void ReflectorClient::sendMsg(const ReflectorMsg& msg)
{
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


void ReflectorClient::disconnect(const std::string& msg)
{
  cout << "### ReflectorClient::disconnect: " << msg << endl;
  sendMsg(MsgError(msg));
  m_disc_timer.setEnable(true);
  m_con_state = STATE_EXPECT_DISCONNECT;
} /* ReflectorClient::disconnect */


void ReflectorClient::onDisconnected(TcpConnection* con,
                                     TcpConnection::DisconnectReason)
{
  if (!m_callsign.empty())
  {
    cout << m_callsign << ": ";
  }
  cout << "Client " << con->remoteHost() << ":" << con->remotePort()
       << " disconnected" << endl;
  m_disc_timer.setEnable(false);
  m_con_state = STATE_DISCONNECTED;
} /* ReflectorClient::onDisconnected */


void ReflectorClient::onDiscTimeout(Timer *t)
{
  cout << "### ReflectorClient::onDiscTimeout" << endl;
  assert(m_con_state == STATE_EXPECT_DISCONNECT);
  m_con->disconnect();
  m_con_state = STATE_DISCONNECTED;
} /* ReflectorClient::onDiscTimeout */


/*
 * This file has not been truncated
 */

