/**
@file	 Reflector.cpp
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
#include <AsyncApplication.h>


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

namespace {
void delete_client(ReflectorClient *client);
};


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
  : m_srv(0), m_udp_sock(0), m_talker(0),
    m_talker_timeout_timer(1000, Timer::TYPE_PERIODIC),
    m_sql_timeout(0), m_sql_timeout_cnt(0), m_sql_timeout_blocktime(60)
{
  timerclear(&m_last_talker_timestamp);
  m_talker_timeout_timer.expired.connect(
      mem_fun(*this, &Reflector::checkTalkerTimeout));
} /* Reflector::Reflector */


Reflector::~Reflector(void)
{
  delete m_udp_sock;
  delete m_srv;

  for (ReflectorClientMap::iterator it = m_client_map.begin();
       it != m_client_map.end(); ++it)
  {
    delete (*it).second;
  }
} /* Reflector::~Reflector */


bool Reflector::initialize(Async::Config &cfg)
{
  m_cfg = &cfg;

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
  m_srv = new TcpServer<FramedTcpConnection>(listen_port);
  m_srv->clientConnected.connect(
      mem_fun(*this, &Reflector::clientConnected));
  m_srv->clientDisconnected.connect(
      mem_fun(*this, &Reflector::clientDisconnected));

  uint16_t udp_listen_port = 5300;
  cfg.getValue("GLOBAL", "LISTEN_PORT", udp_listen_port);
  m_udp_sock = new UdpSocket(udp_listen_port);
  if ((m_udp_sock == 0) || !m_udp_sock->initOk())
  {
    cerr << "*** ERROR: Could not initialize UDP socket" << endl;
    return false;
  }
  m_udp_sock->dataReceived.connect(
      mem_fun(*this, &Reflector::udpDatagramReceived));

  cfg.getValue("GLOBAL", "SQL_TIMEOUT", m_sql_timeout);
  cfg.getValue("GLOBAL", "SQL_TIMEOUT_BLOCKTIME", m_sql_timeout_blocktime);
  m_sql_timeout_blocktime = max(m_sql_timeout_blocktime, 1U);

  return true;
} /* Reflector::initialize */


void Reflector::nodeList(std::vector<std::string>& nodes) const
{
  nodes.clear();
  for (ReflectorClientMap::const_iterator it = m_client_map.begin();
       it != m_client_map.end(); ++it)
  {
    const std::string& callsign = (*it).second->callsign();
    if (!callsign.empty())
    {
      nodes.push_back(callsign);
    }
  }
} /* Reflector::nodeList */


void Reflector::broadcastMsgExcept(const ReflectorMsg& msg,
                                   ReflectorClient *except)
{
  ReflectorClientMap::const_iterator it = m_client_map.begin();
  for (; it != m_client_map.end(); ++it)
  {
    ReflectorClient *client = (*it).second;
    if ((client != except) &&
        (client->conState() == ReflectorClient::STATE_CONNECTED))
    {
      (*it).second->sendMsg(msg);
    }
  }
} /* Reflector::broadcastMsgExcept */


bool Reflector::sendUdpDatagram(ReflectorClient *client, const void *buf,
                                size_t count)
{
  return m_udp_sock->write(client->remoteHost(), client->remoteUdpPort(), buf,
                           count);
} /* Reflector::sendUdpDatagram */


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

void Reflector::clientConnected(Async::FramedTcpConnection *con)
{
  cout << "Client " << con->remoteHost() << ":" << con->remotePort()
       << " connected" << endl;
  ReflectorClient *rc = new ReflectorClient(this, con, m_cfg);
  m_client_map[rc->clientId()] = rc;
  m_client_con_map[con] = rc;
} /* Reflector::clientConnected */


void Reflector::clientDisconnected(Async::FramedTcpConnection *con,
                           Async::FramedTcpConnection::DisconnectReason reason)
{
  ReflectorClientConMap::iterator it = m_client_con_map.find(con);
  assert(it != m_client_con_map.end());
  ReflectorClient *client = (*it).second;

  if (!client->callsign().empty())
  {
    cout << client->callsign() << ": ";
  }
  else
  {
    cout << "Client " << con->remoteHost() << ":" << con->remotePort() << " ";
  }
  cout << "disconnected: " << TcpConnection::disconnectReasonStr(reason)
       << endl;

  m_client_map.erase(client->clientId());
  m_client_con_map.erase(it);

  if (client == m_talker)
  {
    setTalker(0);
  }

  if (!client->callsign().empty())
  {
    broadcastMsgExcept(MsgNodeLeft(client->callsign()), client);
  }
  Application::app().runTask(sigc::bind(sigc::ptr_fun(&delete_client), client));
} /* Reflector::clientDisconnected */


void Reflector::udpDatagramReceived(const IpAddress& addr, uint16_t port,
                                    void *buf, int count)
{
  stringstream ss;
  ss.write(reinterpret_cast<const char *>(buf), count);

  ReflectorUdpMsg header;
  if (!header.unpack(ss))
  {
    cout << "*** WARNING: Unpacking failed for UDP message header\n";
    return;
  }

  ReflectorClientMap::iterator it = m_client_map.find(header.clientId());
  if (it == m_client_map.end())
  {
    cerr << "*** WARNING: Incoming UDP packet has invalid client id" << endl;
    return;
  }
  ReflectorClient *client = (*it).second;
  if (addr != client->remoteHost())
  {
    cerr << "*** WARNING[" << client->callsign()
         << "]: Incoming UDP packet has the wrong source ip" << endl;
    return;
  }
  if (client->remoteUdpPort() == 0)
  {
    client->setRemoteUdpPort(port);
    client->sendUdpMsg(MsgUdpHeartbeat());
  }
  else if (port != client->remoteUdpPort())
  {
    cerr << "*** WARNING[" << client->callsign()
         << "]: Incoming UDP packet has the wrong source UDP "
            "port number" << endl;
    return;
  }

    // Check sequence number
  uint16_t udp_rx_seq_diff = header.sequenceNum() - client->nextUdpRxSeq();
  if (udp_rx_seq_diff > 0x7fff) // Frame out of sequence (ignore)
  {
    cout << client->callsign()
         << ": Dropping out of sequence frame with seq="
         << header.sequenceNum() << ". Expected seq="
         << client->nextUdpRxSeq() << endl;
    return;
  }
  else if (udp_rx_seq_diff > 0) // Frame(s) lost
  {
    cout << client->callsign()
         << ": UDP frame(s) lost. Expected seq=" << client->nextUdpRxSeq()
         << ". Received seq=" << header.sequenceNum() << endl;
  }

  client->udpMsgReceived(header);

  switch (header.type())
  {
    case MsgUdpHeartbeat::TYPE:
      break;

    case MsgUdpAudio::TYPE:
    {
      if (!client->isBlocked())
      {
        MsgUdpAudio msg;
        if (!msg.unpack(ss))
        {
          cerr << "*** WARNING[" << client->callsign()
               << "]: Could not unpack incoming MsgUdpAudio message" << endl;
          return;
        }
        if (!msg.audioData().empty())
        {
          if (m_talker == 0)
          {
            setTalker(client);
            cout << m_talker->callsign() << ": Talker start" << endl;
          }
          if (m_talker == client)
          {
            gettimeofday(&m_last_talker_timestamp, NULL);
            broadcastUdpMsgExcept(client, msg);
          }
        }
      }
      break;
    }

    case MsgUdpFlushSamples::TYPE:
    {
      if (client == m_talker)
      {
        cout << m_talker->callsign() << ": Talker stop" << endl;
        setTalker(0);
      }
        // To be 100% correct the reflector should wait for all connected
        // clients to send a MsgUdpAllSamplesFlushed message but that will
        // probably lead to problems, especially on reflectors with many
        // clients. We therefore acknowledge the flush immediately here to
        // the client who sent the flush request.
      client->sendUdpMsg(MsgUdpAllSamplesFlushed());
      break;
    }

    case MsgUdpAllSamplesFlushed::TYPE:
      // Ignore
      break;

    default:
      // Better ignoring unknown messages to make it easier to add messages to
      // the protocol but still be backwards compatible

      //cerr << "*** WARNING[" << client->callsign()
      //     << "]: Unknown UDP protocol message received: msg_type="
      //     << header.type() << endl;
      break;
  }
} /* Reflector::udpDatagramReceived */


void Reflector::broadcastUdpMsgExcept(const ReflectorClient *except,
                                      const ReflectorUdpMsg& msg)
{
  for (ReflectorClientMap::iterator it = m_client_map.begin();
       it != m_client_map.end(); ++it)
  {
    ReflectorClient *client = (*it).second;
    if ((client != except) &&
        (client->conState() == ReflectorClient::STATE_CONNECTED))
    {
      (*it).second->sendUdpMsg(msg);
    }
  }
} /* Reflector::broadcastUdpMsgExcept */


void Reflector::checkTalkerTimeout(Async::Timer *t)
{
  if (m_talker != 0)
  {
    struct timeval now, diff;
    gettimeofday(&now, NULL);
    timersub(&now, &m_last_talker_timestamp, &diff);
    if (diff.tv_sec > TALKER_AUDIO_TIMEOUT)
    {
      cout << m_talker->callsign() << ": Talker audio timeout"
           << endl;
      setTalker(0);
    }

    if ((m_sql_timeout_cnt > 0) && (--m_sql_timeout_cnt == 0))
    {
      cout << m_talker->callsign() << ": Talker squelch timeout"
           << endl;
      m_talker->setBlock(m_sql_timeout_blocktime);
      setTalker(0);
    }
  }
} /* Reflector::checkTalkerTimeout */


void Reflector::setTalker(ReflectorClient *client)
{
  if (client == m_talker)
  {
    return;
  }

  if (client == 0)
  {
    broadcastMsgExcept(MsgTalkerStop(m_talker->callsign()));
    broadcastUdpMsgExcept(m_talker, MsgUdpFlushSamples());
    m_sql_timeout_cnt = 0;
    m_talker = 0;
  }
  else
  {
    assert(m_talker == 0);
    m_sql_timeout_cnt = m_sql_timeout;
    m_talker = client;
    broadcastMsgExcept(MsgTalkerStart(m_talker->callsign()));
  }
} /* Reflector::setTalker */


namespace {
void delete_client(ReflectorClient *client) { delete client; }
};


/*
 * This file has not been truncated
 */

