/**
@file	 ReflectorClient.cpp
@brief   Represents one client connection
@author  Tobias Blomberg / SM0SVX
@date	 2017-02-11

\verbatim
SvxReflector - An audio reflector for connecting SvxLink Servers
Copyright (C) 2003-2023 Tobias Blomberg / SM0SVX

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
#include <algorithm>
#include <cerrno>
#include <iterator>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncTimer.h>
#include <AsyncAudioEncoder.h>
#include <AsyncAudioDecoder.h>
#include <common.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "ReflectorClient.h"
#include "Reflector.h"
#include "TGHandler.h"


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

ReflectorClient::ClientMap ReflectorClient::client_map;
std::mt19937 ReflectorClient::id_gen(std::random_device{}());
ReflectorClient::ClientIdRandomDist ReflectorClient::id_dist(0, CLIENT_ID_MAX);


/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/

ReflectorClient* ReflectorClient::lookup(ClientId id)
{
  auto it = client_map.find(id);
  if (it == client_map.end())
  {
    return nullptr;
  }
  return it->second;
} /* ReflectorClient::lookup */


void ReflectorClient::cleanup(void)
{
  auto client_map_copy = client_map;
  for (const auto& item : client_map_copy)
  {
    delete item.second;
  }
  assert(client_map.size() == 0);
} /* ReflectorClient::cleanup */


bool ReflectorClient::TgFilter::operator()(ReflectorClient* client) const
{
  //cout << "m_tg=" << m_tg << "  client_tg="
  //     << TGHandler::instance()->TGForClient(client) << endl;
  return m_tg == TGHandler::instance()->TGForClient(client);
}


ReflectorClient::ReflectorClient(Reflector *ref, Async::FramedTcpConnection *con,
                                 Async::Config *cfg)
  : m_con(con), m_con_state(STATE_EXPECT_PROTO_VER),
    m_disc_timer(10000, Timer::TYPE_ONESHOT, false),
    m_client_id(newClient(this)), m_remote_udp_port(0), m_cfg(cfg),
    m_next_udp_tx_seq(0), m_next_udp_rx_seq(0),
    m_heartbeat_timer(1000, Timer::TYPE_PERIODIC),
    m_heartbeat_tx_cnt(HEARTBEAT_TX_CNT_RESET),
    m_heartbeat_rx_cnt(HEARTBEAT_RX_CNT_RESET),
    m_udp_heartbeat_tx_cnt(UDP_HEARTBEAT_TX_CNT_RESET),
    m_udp_heartbeat_rx_cnt(UDP_HEARTBEAT_RX_CNT_RESET),
    m_reflector(ref), m_blocktime(0), m_remaining_blocktime(0),
    m_current_tg(0)
{
  m_con->setMaxFrameSize(ReflectorMsg::MAX_PREAUTH_FRAME_SIZE);
  m_con->frameReceived.connect(
      mem_fun(*this, &ReflectorClient::onFrameReceived));
  m_disc_timer.expired.connect(
      mem_fun(*this, &ReflectorClient::onDiscTimeout));
  m_heartbeat_timer.expired.connect(
      mem_fun(*this, &ReflectorClient::handleHeartbeat));

  string codecs;
  if (m_cfg->getValue("GLOBAL", "CODECS", codecs))
  {
    SvxLink::splitStr(m_supported_codecs, codecs, ",");
  }
  if (m_supported_codecs.size() > 1)
  {
    m_supported_codecs.erase(m_supported_codecs.begin()+1,
                             m_supported_codecs.end());
    cout << "*** WARNING: The GLOBAL/CODECS configuration "
            "variable can only take one codec at the moment. Using the first "
            "one: \"" << m_supported_codecs.front() << "\"" << endl;
  }
  else if (m_supported_codecs.empty())
  {
    string codec = "GSM";
    if (AudioDecoder::isAvailable("OPUS") &&
        AudioEncoder::isAvailable("OPUS"))
    {
      codec = "OPUS";
    }
    else if (AudioDecoder::isAvailable("SPEEX") &&
             AudioEncoder::isAvailable("SPEEX"))
    {
      codec = "SPEEX";
    }
    m_supported_codecs.push_back(codec);
  }
} /* ReflectorClient::ReflectorClient */


ReflectorClient::~ReflectorClient(void)
{
  auto client_it = client_map.find(m_client_id);
  assert(client_it != client_map.end());
  client_map.erase(client_it);
  TGHandler::instance()->removeClient(this);
} /* ReflectorClient::~ReflectorClient */


int ReflectorClient::sendMsg(const ReflectorMsg& msg)
{
  if (((m_con_state != STATE_CONNECTED) && (msg.type() >= 100)) ||
      !m_con->isConnected())
  {
    errno = ENOTCONN;
    return -1;
  }

  m_heartbeat_tx_cnt = HEARTBEAT_TX_CNT_RESET;

  ReflectorMsg header(msg.type());
  ostringstream ss;
  if (!header.pack(ss) || !msg.pack(ss))
  {
    cerr << "*** ERROR: Failed to pack TCP message\n";
    errno = EBADMSG;
    return -1;
  }
  return m_con->write(ss.str().data(), ss.str().size());
} /* ReflectorClient::sendMsg */


void ReflectorClient::udpMsgReceived(const ReflectorUdpMsg &header)
{
  m_next_udp_rx_seq = header.sequenceNum() + 1;

  m_udp_heartbeat_rx_cnt = UDP_HEARTBEAT_RX_CNT_RESET;

  if ((m_blocktime > 0) && (header.type() == MsgUdpAudio::TYPE))
  {
    m_remaining_blocktime = m_blocktime;
  }
} /* ReflectorClient::udpMsgReceived */


void ReflectorClient::sendUdpMsg(const ReflectorUdpMsg &msg)
{
  if (remoteUdpPort() == 0)
  {
    return;
  }

  m_udp_heartbeat_tx_cnt = UDP_HEARTBEAT_TX_CNT_RESET;

  ReflectorUdpMsg header(msg.type(), clientId(), nextUdpTxSeq());
  ostringstream ss;
  assert(header.pack(ss) && msg.pack(ss));
  (void)m_reflector->sendUdpDatagram(this, ss.str().data(), ss.str().size());
} /* ReflectorClient::sendUdpMsg */


void ReflectorClient::setBlock(unsigned blocktime)
{
  m_blocktime = blocktime;
  m_remaining_blocktime = blocktime;
} /* ReflectorClient::setBlock */


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

ReflectorClient::ClientId ReflectorClient::newClient(ReflectorClient* client)
{
  assert(!(client_map.size() > CLIENT_ID_MAX));
  ClientId id = id_dist(id_gen);
  while (client_map.count(id) > 0)
  {
    id = (id < CLIENT_ID_MAX) ? id+1 : 0;
  }
  client_map[id] = client;
  return id;
} /* ReflectorClient::newClient */


void ReflectorClient::onFrameReceived(FramedTcpConnection *con,
                                      std::vector<uint8_t>& data)
{
  int len = data.size();
  //cout << "### ReflectorClient::onFrameReceived: len=" << len << endl;

  assert(len >= 0);

  if ((m_con_state == STATE_DISCONNECTED) ||
      (m_con_state == STATE_EXPECT_DISCONNECT))
  {
    return;
  }

  char *buf = reinterpret_cast<char*>(&data.front());
  stringstream ss;
  ss.write(buf, len);

  ReflectorMsg header;
  if (!header.unpack(ss))
  {
    if (!m_callsign.empty())
    {
      cout << m_callsign << ": ";
    }
    else
    {
      cout << m_con->remoteHost() << ":" << m_con->remotePort() << " ";
    }
    cout << "ERROR: Unpacking failed for TCP message header" << endl;
    sendError("Protocol message header too short");
    return;
  }

  m_heartbeat_rx_cnt = HEARTBEAT_RX_CNT_RESET;

  switch (header.type())
  {
    case MsgHeartbeat::TYPE:
      break;
    case MsgProtoVer::TYPE:
      handleMsgProtoVer(ss);
      break;
    case MsgAuthResponse::TYPE:
      handleMsgAuthResponse(ss);
      break;
    case MsgSelectTG::TYPE:
      handleSelectTG(ss);
      break;
    case MsgTgMonitor::TYPE:
      handleTgMonitor(ss);
      break;
    case MsgNodeInfo::TYPE:
      handleNodeInfo(ss);
      break;
    case MsgSignalStrengthValues::TYPE:
      handleMsgSignalStrengthValues(ss);
      break;
    case MsgTxStatus::TYPE:
      handleMsgTxStatus(ss);
      break;
#if 0
    case MsgNodeInfo::TYPE:
      handleNodeInfo(ss);
      break;
#endif
    case MsgRequestQsy::TYPE:
      handleRequestQsy(ss);
      break;
    case MsgStateEvent::TYPE:
      handleStateEvent(ss);
      break;
    case MsgError::TYPE:
      handleMsgError(ss);
      break;
    default:
      // Better just ignoring unknown protocol messages for making it easier to
      // add messages to the protocol and still be backwards compatible.

      //cerr << "*** WARNING: Unknown protocol message received: msg_type="
      //     << header.type() << endl;
      break;
  }
} /* ReflectorClient::onFrameReceived */


void ReflectorClient::handleMsgProtoVer(std::istream& is)
{
  if (m_con_state != STATE_EXPECT_PROTO_VER)
  {
    sendError("Protocol version expected");
    return;
  }

  MsgProtoVer msg;
  if (!msg.unpack(is))
  {
    std::cout << "Client " << m_con->remoteHost() << ":" << m_con->remotePort()
              << " ERROR: Could not unpack MsgProtoVer\n";
    sendError("Illegal MsgProtoVer protocol message received");
    return;
  }
  m_client_proto_ver.set(msg.majorVer(), msg.minorVer());
  ProtoVer max_proto_ver(MsgProtoVer::MAJOR, MsgProtoVer::MINOR);
  if (m_client_proto_ver > max_proto_ver)
  {
    std::cout << "Client " << m_con->remoteHost() << ":" << m_con->remotePort()
              << " use protocol version "
              << msg.majorVer() << "." << msg.minorVer()
              << " which is newer than we can handle. Asking for downgrade to "
              << MsgProtoVer::MAJOR << "." << MsgProtoVer::MINOR << "."
              << std::endl;
    sendMsg(MsgProtoVerDowngrade());
    return;
  }
  else if (m_client_proto_ver < ProtoVer(MIN_MAJOR_VER, MIN_MINOR_VER))
  {
    std::cout << "Client " << m_con->remoteHost() << ":" << m_con->remotePort()
              << " is using protocol version "
              << msg.majorVer() << "." << msg.minorVer()
              << " which is too old. Must at least be version "
              << MIN_MAJOR_VER << "." << MIN_MINOR_VER << "." << std::endl;
    std::ostringstream ss;
    ss << "Unsupported protocol version " << msg.majorVer() << "."
       << msg.minorVer() << ". Must be at least "
       << MIN_MAJOR_VER << "." << MIN_MINOR_VER << ".";
    sendError(ss.str());
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
    cout << "Client " << m_con->remoteHost() << ":" << m_con->remotePort()
         << " Authentication response unexpected" << endl;
    sendError("Authentication response unexpected");
    return;
  }

  MsgAuthResponse msg;
  if (!msg.unpack(is))
  {
    cout << "Client " << m_con->remoteHost() << ":" << m_con->remotePort()
         << " ERROR: Could not unpack MsgAuthResponse" << endl;
    sendError("Illegal MsgAuthResponse protocol message received");
    return;
  }

  string auth_key = lookupUserKey(msg.callsign());
  if (!auth_key.empty() && msg.verify(auth_key, m_auth_challenge))
  {
    vector<string> connected_nodes;
    m_reflector->nodeList(connected_nodes);
    if (find(connected_nodes.begin(), connected_nodes.end(),
             msg.callsign()) == connected_nodes.end())
    {
      m_con->setMaxFrameSize(ReflectorMsg::MAX_POSTAUTH_FRAME_SIZE);
      m_callsign = msg.callsign();
      sendMsg(MsgAuthOk());
      cout << m_callsign << ": Login OK from "
           << m_con->remoteHost() << ":" << m_con->remotePort()
           << " with protocol version " << m_client_proto_ver.majorVer()
           << "." << m_client_proto_ver.minorVer()
           << endl;
      m_con_state = STATE_CONNECTED;
      MsgServerInfo msg_srv_info(m_client_id, m_supported_codecs);
      m_reflector->nodeList(msg_srv_info.nodes());
      sendMsg(msg_srv_info);
      if (m_client_proto_ver < ProtoVer(0, 7))
      {
        MsgNodeList msg_node_list(msg_srv_info.nodes());
        sendMsg(msg_node_list);
      }
      if (m_client_proto_ver < ProtoVer(2, 0))
      {
        if (TGHandler::instance()->switchTo(this, m_reflector->tgForV1Clients()))
        {
          std::cout << m_callsign << ": Select TG #"
                    << m_reflector->tgForV1Clients() << std::endl;
          m_current_tg = m_reflector->tgForV1Clients();
        }
        else
        {
          std::cout << m_callsign
                    << ": V1 client not allowed to use default TG #"
                    << m_reflector->tgForV1Clients() << std::endl;
        }
      }
      m_reflector->broadcastMsg(MsgNodeJoined(m_callsign), ExceptFilter(this));
    }
    else
    {
      cout << msg.callsign() << ": Already connected" << endl;
      sendError("Access denied");
    }
  }
  else
  {
    cout << "Client " << m_con->remoteHost() << ":" << m_con->remotePort()
         << " Authentication failed for user \"" << msg.callsign()
         << "\"" << endl;
    sendError("Access denied");
  }
} /* ReflectorClient::handleMsgAuthResponse */


void ReflectorClient::handleSelectTG(std::istream& is)
{
  MsgSelectTG msg;
  if (!msg.unpack(is))
  {
    cout << "Client " << m_con->remoteHost() << ":" << m_con->remotePort()
         << " ERROR: Could not unpack MsgSelectTG" << endl;
    sendError("Illegal MsgSelectTG protocol message received");
    return;
  }
  if (msg.tg() != m_current_tg)
  {
    ReflectorClient *talker = TGHandler::instance()->talkerForTG(m_current_tg);
    if (talker == this)
    {
      m_reflector->broadcastUdpMsg(MsgUdpFlushSamples(),
          mkAndFilter(
            TgFilter(m_current_tg),
            ExceptFilter(this)));
    }
    else if (talker != 0)
    {
      sendUdpMsg(MsgUdpFlushSamples());
    }
    if (TGHandler::instance()->switchTo(this, msg.tg()))
    {
      cout << m_callsign << ": Select TG #" << msg.tg() << endl;
      m_current_tg = msg.tg();
    }
    else
    {
      // FIXME: Notify the client that the TG selection was not allowed
      std::cout << m_callsign << ": Not allowed to use TG #"
                << msg.tg() << std::endl;
      TGHandler::instance()->switchTo(this, 0);
      m_current_tg = 0;
    }
  }
} /* ReflectorClient::handleSelectTG */


void ReflectorClient::handleTgMonitor(std::istream& is)
{
  MsgTgMonitor msg;
  if (!msg.unpack(is))
  {
    cout << "Client " << m_con->remoteHost() << ":" << m_con->remotePort()
         << " ERROR: Could not unpack MsgTgMonitor" << endl;
    sendError("Illegal MsgTgMonitor protocol message received");
    return;
  }
  auto tgs = msg.tgs();
  auto it = tgs.cbegin();
  while (it != tgs.end())
  {
    const auto& tg = *it;
    if (!TGHandler::instance()->allowTgSelection(this, tg) || (tg == 0))
    {
      std::cout << m_callsign << ": Not allowed to monitor TG #"
                << tg << std::endl;
      tgs.erase(it++);
      continue;
    }
    ++it;
  }
  cout << m_callsign << ": Monitor TG#: [ ";
  std::copy(tgs.begin(), tgs.end(), std::ostream_iterator<uint32_t>(cout, " "));
  cout << "]" << endl;

  m_monitored_tgs = tgs;
} /* ReflectorClient::handleTgMonitor */


void ReflectorClient::handleNodeInfo(std::istream& is)
{
  MsgNodeInfo msg;
  if (!msg.unpack(is))
  {
    cout << "Client " << m_con->remoteHost() << ":" << m_con->remotePort()
         << " ERROR: Could not unpack MsgNodeInfo" << endl;
    sendError("Illegal MsgNodeInfo protocol message received");
    return;
  }
  //std::cout << "### handleNodeInfo: " << msg.json() << std::endl;
  try
  {
    std::istringstream is(msg.json());
    is >> m_node_info;
  }
  catch (const Json::Exception& e)
  {
    std::cerr << "*** WARNING[" << m_callsign
              << "]: Failed to parse MsgNodeInfo JSON object: "
              << e.what() << std::endl;
  }
} /* ReflectorClient::handleNodeInfo */


void ReflectorClient::handleMsgSignalStrengthValues(std::istream& is)
{
  MsgSignalStrengthValues msg;
  if (!msg.unpack(is))
  {
    cerr << "*** WARNING[" << callsign()
         << "]: Could not unpack incoming "
            "MsgSignalStrengthValues message" << endl;
    return;
  }
  typedef MsgSignalStrengthValues::Rxs::const_iterator RxsIter;
  for (RxsIter it = msg.rxs().begin(); it != msg.rxs().end(); ++it)
  {
    const MsgSignalStrengthValues::Rx& rx = *it;
    //std::cout << "### MsgSignalStrengthValues:"
    //  << " id=" << rx.id()
    //  << " siglev=" << rx.siglev()
    //  << " enabled=" << rx.enabled()
    //  << " sql_open=" << rx.sqlOpen()
    //  << " active=" << rx.active()
    //  << std::endl;
    setRxSiglev(rx.id(), rx.siglev());
    setRxEnabled(rx.id(), rx.enabled());
    setRxSqlOpen(rx.id(), rx.sqlOpen());
    setRxActive(rx.id(), rx.active());
  }
} /* ReflectorClient::handleMsgSignalStrengthValues */


void ReflectorClient::handleMsgTxStatus(std::istream& is)
{
  MsgTxStatus msg;
  if (!msg.unpack(is))
  {
    cerr << "*** WARNING[" << callsign()
         << "]: Could not unpack incoming MsgTxStatus message" << endl;
    return;
  }
  typedef MsgTxStatus::Txs::const_iterator TxsIter;
  for (TxsIter it = msg.txs().begin(); it != msg.txs().end(); ++it)
  {
    const MsgTxStatus::Tx& tx = *it;
    //std::cout << "### MsgTxStatus:"
    //  << " id=" << tx.id()
    //  << " transmit=" << tx.transmit()
    //  << std::endl;
    setTxTransmit(tx.id(), tx.transmit());
  }
} /* ReflectorClient::handleMsgTxStatus */


void ReflectorClient::handleRequestQsy(std::istream& is)
{
  MsgRequestQsy msg;
  if (!msg.unpack(is))
  {
    cout << "Client " << m_con->remoteHost() << ":" << m_con->remotePort()
         << " ERROR: Could not unpack MsgRequestQsy" << endl;
    sendError("Illegal MsgRequestQsy protocol message received");
    return;
  }
  m_reflector->requestQsy(this, msg.tg());
} /* ReflectorClient::handleRequestQsy */


void ReflectorClient::handleStateEvent(std::istream& is)
{
  MsgStateEvent msg;
  if (!msg.unpack(is))
  {
    cout << "Client " << m_con->remoteHost() << ":" << m_con->remotePort()
         << " ERROR: Could not unpack MsgStateEvent" << endl;
    sendError("Illegal MsgStateEvent protocol message received");
    return;
  }
  cout << "### ReflectorClient::handleStateEvent:"
       << " src=" << msg.src()
       << " name=" << msg.name()
       << " msg=" << msg.msg()
       << std::endl;
} /* ReflectorClient::handleStateEvent */


#if 0
void ReflectorClient::handleNodeInfo(std::istream& is)
{
  MsgNodeInfo msg;
  if (!msg.unpack(is))
  {
    cout << "Client " << m_con->remoteHost() << ":" << m_con->remotePort()
         << " ERROR: Could not unpack MsgNodeInfo" << endl;
    sendError("Illegal MsgNodeInfo protocol message received");
    return;
  }
  cout << m_callsign << ": Client info"
       << "\n--- Software: \"" << msg.swInfo() << "\"";
  for (size_t i=0; i<msg.rxSites().size(); ++i)
  {
    const MsgNodeInfo::RxSite& rx_site = msg.rxSites().at(i);
    cout << "\n--- Receiver \"" << rx_site.rxName() << "\":"
         << "\n---   QTH Name          : " << rx_site.qthName();
    if (rx_site.antennaHeightIsValid())
    {
      cout << "\n---   Antenna Height    : " << rx_site.antennaHeight()
           << "m above sea level";
    }
    if (rx_site.antennaDirectionIsValid())
    {
      cout << "\n---   Antenna Direction : " << rx_site.antennaDirection()
           << " degrees";
    }
    if (rx_site.rfFrequencyIsValid())
    {
      cout << "\n---   RF Frequency      : " << rx_site.rfFrequency()
           << "Hz";
    }
    if (rx_site.ctcssFrequenciesIsValid())
    {
      cout << "\n---   CTCSS Frequencies : ";
      std::copy(rx_site.ctcssFrequencies().begin(),
                rx_site.ctcssFrequencies().end(),
                std::ostream_iterator<float>(cout, " "));
    }
  }
  for (size_t i=0; i<msg.txSites().size(); ++i)
  {
    const MsgNodeInfo::TxSite& tx_site = msg.txSites().at(i);
    cout << "\n--- Transmitter \"" << tx_site.txName() << "\":"
         << "\n---   QTH Name          : " << tx_site.qthName();
    if (tx_site.antennaHeightIsValid())
    {
      cout << "\n---   Antenna Height    : " << tx_site.antennaHeight()
           << "m above sea level";
    }
    if (tx_site.antennaDirectionIsValid())
    {
      cout << "\n---   Antenna Direction : " << tx_site.antennaDirection()
           << " degrees";
    }
    if (tx_site.rfFrequencyIsValid())
    {
      cout << "\n---   RF Frequency      : " << tx_site.rfFrequency()
           << "Hz";
    }
    if (tx_site.ctcssFrequenciesIsValid())
    {
      cout << "\n---   CTCSS Frequencies : ";
      std::copy(tx_site.ctcssFrequencies().begin(),
                tx_site.ctcssFrequencies().end(),
                std::ostream_iterator<float>(cout, " "));
    }
    if (tx_site.txPowerIsValid())
    {
      cout << "\n---   TX Power          : " << tx_site.txPower() << "W";
    }
  }
  //if (!msg.qthName().empty())
  //{
  //  cout << "\n--- QTH Name=\"" << msg.qthName() << "\"";
  //}
  cout << endl;
} /* ReflectorClient::handleNodeInfo */
#endif


void ReflectorClient::handleMsgError(std::istream& is)
{
  MsgError msg;
  string message;
  if (msg.unpack(is))
  {
    message = msg.message();
  }
  if (!m_callsign.empty())
  {
    cout << m_callsign << ": ";
  }
  else
  {
    cout << m_con->remoteHost() << ":" << m_con->remotePort() << " ";
  }
  cout << "Error message received from remote peer: " << message << endl;
  disconnect();
} /* ReflectorClient::handleMsgError */


void ReflectorClient::sendError(const std::string& msg)
{
  sendMsg(MsgError(msg));
  m_heartbeat_timer.setEnable(false);
  m_remote_udp_port = 0;
  m_disc_timer.setEnable(true);
  m_con_state = STATE_EXPECT_DISCONNECT;
} /* ReflectorClient::sendError */


void ReflectorClient::onDiscTimeout(Timer *t)
{
  assert(m_con_state == STATE_EXPECT_DISCONNECT);
  disconnect();
} /* ReflectorClient::onDiscTimeout */


void ReflectorClient::disconnect(void)
{
  m_heartbeat_timer.setEnable(false);
  m_remote_udp_port = 0;
  m_con->disconnect();
  m_con_state = STATE_DISCONNECTED;
  m_con->disconnected(m_con, FramedTcpConnection::DR_ORDERED_DISCONNECT);
} /* ReflectorClient::disconnect */


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
    if (!callsign().empty())
    {
      cout << callsign() << ": ";
    }
    else
    {
      cout << "Client " << m_con->remoteHost() << ":"
           << m_con->remotePort() << " ";
    }
    cout << "TCP heartbeat timeout" << endl;
    sendError("TCP heartbeat timeout");
  }

  if (--m_udp_heartbeat_rx_cnt == 0)
  {
    if (!callsign().empty())
    {
      cout << callsign() << ": ";
    }
    else
    {
      cout << "Client " << m_con->remoteHost() << ":"
           << m_con->remotePort() << " ";
    }
    cout << "UDP heartbeat timeout" << endl;
    sendError("UDP heartbeat timeout");
  }

  if (m_blocktime > 0)
  {
    if (m_remaining_blocktime == 0)
    {
      m_blocktime = 0;
    }
    else
    {
      m_remaining_blocktime -= 1;
    }
  }
} /* ReflectorClient::handleHeartbeat */


std::string ReflectorClient::lookupUserKey(const std::string& callsign)
{
  string auth_group;
  if (!m_cfg->getValue("USERS", callsign, auth_group) || auth_group.empty())
  {
    cout << "*** WARNING: Unknown user \"" << callsign << "\""
         << endl;
    return "";
  }
  string auth_key;
  if (!m_cfg->getValue("PASSWORDS", auth_group, auth_key) || auth_key.empty())
  {
    cout << "*** ERROR: User \"" << callsign << "\" found in SvxReflector "
         << "configuration but password with groupname \"" << auth_group
         << "\" not found." << endl;
    return "";
  }
  return auth_key;
} /* ReflectorClient::lookupUserKey */


/*
 * This file has not been truncated
 */

