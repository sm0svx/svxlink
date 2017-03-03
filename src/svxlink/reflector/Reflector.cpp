/**
@file	 Reflector.cpp
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

#include <cassert>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncConfig.h>
#include <AsyncTcpServer.h>
#include <AsyncUdpSocket.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "Reflector.h"
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



/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/

Reflector::Reflector(void)
  : srv(0), udp_sock(0), m_talker(0),
    m_talker_timeout_timer(1000, Timer::TYPE_PERIODIC)
{
  timerclear(&m_last_talker_timestamp);
  m_talker_timeout_timer.expired.connect(
      mem_fun(*this, &Reflector::checkTalkerTimeout));
} /* Reflector::Reflector */


Reflector::~Reflector(void)
{
  delete udp_sock;
  delete srv;
} /* Reflector::~Reflector */


bool Reflector::initialize(Async::Config &cfg)
{
    // Initialize the GCrypt library if not already initialized
  if (!gcry_control(GCRYCTL_INITIALIZATION_FINISHED_P))
  {
    gcry_check_version(NULL);
    gcry_error_t err;
    err = gcry_control(GCRYCTL_DISABLE_SECMEM, 0);
    if (err != GPG_ERR_NO_ERROR)
    {
      cerr << "*** ERROR: Failed to initialize the Libgcrypt library: "
           << gcry_strsource(err) << "/" << gcry_strerror(err) << endl;
      return false;
    }
      // Tell Libgcrypt that initialization has completed
    err = gcry_control(GCRYCTL_INITIALIZATION_FINISHED, 0);
    if (err != GPG_ERR_NO_ERROR)
    {
      cerr << "*** ERROR: Failed to initialize the Libgcrypt library: "
           << gcry_strsource(err) << "/" << gcry_strerror(err) << endl;
      return false;
    }
  }

  std::string listen_port("5300");
  cfg.getValue("GLOBAL", "LISTEN_PORT", listen_port);
  srv = new TcpServer(listen_port);
  srv->clientConnected.connect(
      mem_fun(*this, &Reflector::clientConnected));
  srv->clientDisconnected.connect(
      mem_fun(*this, &Reflector::clientDisconnected));

  uint16_t udp_listen_port = 5300;
  cfg.getValue("GLOBAL", "LISTEN_PORT", udp_listen_port);
  udp_sock = new UdpSocket(udp_listen_port);
  if ((udp_sock == 0) || !udp_sock->initOk())
  {
    cerr << "*** ERROR: Could not initialize UDP socket" << endl;
    return false;
  }
  udp_sock->dataReceived.connect(
      mem_fun(*this, &Reflector::udpDatagramReceived));

  if (!cfg.getValue("GLOBAL", "AUTH_KEY", m_auth_key) || m_auth_key.empty())
  {
    cerr << "*** ERROR: GLOBAL/AUTH_KEY must be specified\n";
    return false;
  }
  if (m_auth_key == "Change this key now!")
  {
    cerr << "*** ERROR: You must change GLOBAL/AUTH_KEY from the "
            "default value" << endl;
    return false;
  }

  return true;
} /* Reflector::initialize */


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

void Reflector::clientConnected(Async::TcpConnection *con)
{
  cout << "Client " << con->remoteHost() << ":" << con->remotePort()
       << " connected" << endl;
  ReflectorClient *rc = new ReflectorClient(con, m_auth_key);
  client_map[rc->clientId()] = rc;
  m_client_con_map[con] = rc;
} /* Reflector::clientConnected */


void Reflector::clientDisconnected(Async::TcpConnection *con,
                                   Async::TcpConnection::DisconnectReason reason)
{
  ReflectorClientConMap::iterator it = m_client_con_map.find(con);
  assert(it != m_client_con_map.end());
  ReflectorClient *client = (*it).second;

  if (!client->callsign().empty())
  {
    cout << client->callsign() << ": ";
  }
  cout << "Client " << con->remoteHost() << ":" << con->remotePort()
       << " disconnected: " << TcpConnection::disconnectReasonStr(reason)
       << endl;

  client_map.erase(client->clientId());
  m_client_con_map.erase(it);
  delete client;
} /* Reflector::clientDisconnected */


void Reflector::udpDatagramReceived(const IpAddress& addr, uint16_t port,
                                    void *buf, int count)
{
  //cout << "### Reflector::udpDatagramReceived: addr=" << addr
  //     << " port=" << port << " count=" << count;

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

  ReflectorClientMap::iterator it = client_map.find(header.clientId());
  if (it == client_map.end())
  {
    cerr << "*** WARNING: Incoming UDP packet has invalid client id" << endl;
    return;
  }
  ReflectorClient *client = (*it).second;
  if (addr != client->remoteHost())
  {
    cerr << "*** WARNING: Incoming UDP packet has the wrong source ip" << endl;
    return;
  }
  if (client->remoteUdpPort() == 0)
  {
    client->setRemoteUdpPort(port);
  }
  else if (port != client->remoteUdpPort())
  {
    cerr << "*** WARNING: Incoming UDP packet has the wrong source UDP "
            "port number" << endl;
    return;
  }

    // Check sequence number
  uint16_t udp_rx_seq_diff = header.sequenceNum() - client->nextUdpRxSeq();
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
    client->setNextUdpRxSeq(header.sequenceNum() + 1);
  }

  switch (header.type())
  {
    case MsgUdpHeartbeat::TYPE:
      cout << "### " << client->callsign() << ": MsgUdpHeartbeat()" << endl;
      // FIXME: Handle heartbeat
      break;

    case MsgUdpAudio::TYPE:
    {
      MsgUdpAudio msg;
      msg.unpack(ss);
      if (!msg.audioData().empty())
      {
        if (m_talker == 0)
        {
          m_talker = client;
        }
        if (m_talker == client)
        {
          gettimeofday(&m_last_talker_timestamp, NULL);
          broadcastUdpMsgExcept(client, msg);
        }
        else
        {
          cout << "### " << m_talker->callsign() << " is already talking...\n";
        }
      }
      break;
    }

    case MsgUdpFlushSamples::TYPE:
    {
      //cout << "### " << client->callsign() << ": MsgUdpFlushSamples()" << endl;
      if (client == m_talker)
      {
        m_talker = 0;
        broadcastUdpMsgExcept(client, MsgUdpFlushSamples());
      }
        // To be 100% correct the reflector should wait for all connected
        // clients to send a MsgUdpAllSamplesFlushed message but that will
        // probably lead to problems, especially on reflectors with many
        // clients. We therefore acknowledge the flush immediately here to
        // the client who sent the flush request.
      sendUdpMsg(client, MsgUdpAllSamplesFlushed());
      break;
    }

    case MsgUdpAllSamplesFlushed::TYPE:
      //cout << "### " << client->callsign() << ": MsgUdpAllSamplesFlushed()"
      //     << endl;
      // Ignore
      break;

    default:
      cerr << "*** WARNING: Unknown UDP protocol message received: msg_type="
           << header.type() << endl;
      // FIXME: Disconnect client or ignore?
      break;
  }
} /* Reflector::udpDatagramReceived */


void Reflector::sendUdpMsg(ReflectorClient *client,
                           const ReflectorUdpMsg &msg)
{
  if (client->remoteUdpPort() == 0)
  {
    return;
  }

  //cout << "### Reflector::sendUdpMsg: " << client->remoteHost() << ":"
  //     << client->remoteUdpPort() << endl;

  ReflectorUdpMsg header(msg.type(), client->clientId(), client->nextUdpTxSeq());
  ostringstream ss;
  if (!header.pack(ss) || !msg.pack(ss))
  {
    // FIXME: Better error handling
    cerr << "*** ERROR: Failed to pack reflector UDP message\n";
    return;
  }
  udp_sock->write(client->remoteHost(), client->remoteUdpPort(),
                  ss.str().data(), ss.str().size());
} /* ReflectorLogic::sendUdpMsg */


void Reflector::broadcastUdpMsgExcept(const ReflectorClient *client,
                                      const ReflectorUdpMsg& msg)
{
  for (ReflectorClientMap::iterator it = client_map.begin();
       it != client_map.end(); ++it)
  {
    if ((*it).second != client)
    {
      sendUdpMsg((*it).second, msg);
    }
  }
} /* Reflector::broadcastUdpMsgExcept */


void Reflector::checkTalkerTimeout(Async::Timer *t)
{
  //cout << "### Reflector::checkTalkerTimeout\n";

  if (m_talker != 0)
  {
    struct timeval now, diff;
    gettimeofday(&now, NULL);
    timersub(&now, &m_last_talker_timestamp, &diff);
    if (diff.tv_sec > 3)
    {
      cout << "### Talker timeout\n";
      broadcastUdpMsgExcept(m_talker, MsgUdpFlushSamples());
      m_talker = 0;
    }
  }
} /* Reflector::checkTalkerTimeout */


/*
 * This file has not been truncated
 */

