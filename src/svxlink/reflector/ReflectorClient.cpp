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
#include <fstream>
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
#include <AsyncSslCertSigningReq.h>
#include <AsyncEncryptedUdpSocket.h>
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

#define RENEW_AFTER 2/3


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
ReflectorClient::ClientSrcMap ReflectorClient::client_src_map;
ReflectorClient::ClientCallsignMap ReflectorClient::client_callsign_map;
std::mt19937 ReflectorClient::id_gen(std::random_device{}());
ReflectorClient::ClientIdRandomDist ReflectorClient::id_dist(
    CLIENT_ID_MIN, CLIENT_ID_MAX);


/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/

ReflectorClient* ReflectorClient::lookup(const ClientId& id)
{
  auto it = client_map.find(id);
  if (it == client_map.end())
  {
    return nullptr;
  }
  return it->second;
} /* ReflectorClient::lookup */


ReflectorClient* ReflectorClient::lookup(const ClientSrc& src)
{
  auto it = client_src_map.find(src);
  if (it == client_src_map.end())
  {
    return nullptr;
  }
  return it->second;
} /* ReflectorClient::lookup */


ReflectorClient* ReflectorClient::lookup(const std::string& cs)
{
  auto it = client_callsign_map.find(cs);
  if (it == client_callsign_map.end())
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
    m_client_id(newClientId(this)), m_remote_udp_port(0), m_cfg(cfg),
    /*m_next_udp_tx_seq(0),*/ m_next_udp_rx_seq(0),
    m_heartbeat_timer(1000, Timer::TYPE_PERIODIC),
    m_heartbeat_tx_cnt(HEARTBEAT_TX_CNT_RESET),
    m_heartbeat_rx_cnt(HEARTBEAT_RX_CNT_RESET),
    m_udp_heartbeat_tx_cnt(UDP_HEARTBEAT_TX_CNT_RESET),
    m_udp_heartbeat_rx_cnt(UDP_HEARTBEAT_RX_CNT_RESET),
    m_reflector(ref), m_blocktime(0), m_remaining_blocktime(0),
    m_current_tg(0), m_udp_cipher_iv_cntr(0)
{
  m_con->setMaxFrameSize(ReflectorMsg::MAX_PREAUTH_FRAME_SIZE);
  m_con->sslConnectionReady.connect(
      sigc::mem_fun(*this, &ReflectorClient::onSslConnectionReady));
  m_con->frameReceived.connect(
      sigc::mem_fun(*this, &ReflectorClient::onFrameReceived));
  m_disc_timer.expired.connect(
      sigc::mem_fun(*this, &ReflectorClient::onDiscTimeout));
  m_heartbeat_timer.expired.connect(
      sigc::mem_fun(*this, &ReflectorClient::handleHeartbeat));
  m_renew_cert_timer.expired.connect(sigc::hide(
      sigc::mem_fun(*this, &ReflectorClient::renewClientCertificate)));

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
  client_src_map.erase(m_client_src);
  if (!m_callsign.empty())
  {
    client_callsign_map.erase(m_callsign);
  }
  TGHandler::instance()->removeClient(this);
} /* ReflectorClient::~ReflectorClient */


void ReflectorClient::setRemoteUdpPort(uint16_t port)
{
  assert(m_remote_udp_port == 0);
  m_remote_udp_port = port;
  if (m_client_proto_ver >= ProtoVer(3, 0))
  {
    m_client_src = newClientSrc(this);
  }
} /* ReflectorClient::setRemoteUdpPort */


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

  (void)m_reflector->sendUdpDatagram(this, msg);
} /* ReflectorClient::sendUdpMsg */


void ReflectorClient::setBlock(unsigned blocktime)
{
  m_blocktime = blocktime;
  m_remaining_blocktime = blocktime;
} /* ReflectorClient::setBlock */


std::vector<uint8_t> ReflectorClient::udpCipherIV(void) const
{
  return UdpCipher::IV{udpCipherIVRand(), 0, m_udp_cipher_iv_cntr};
} /* ReflectorClient::udpCipherIV */


void ReflectorClient::certificateUpdated(Async::SslX509& cert)
{
  if (m_con_state == STATE_CONNECTED)
  {
    //sendMsg(MsgClientCert(cert.pem()));
    sendClientCert(cert);
  }
} /* ReflectorClient::certificateUpdated */


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

ReflectorClient::ClientId ReflectorClient::newClientId(ReflectorClient* client)
{
  assert(client_map.size() <
         (static_cast<size_t>(CLIENT_ID_MAX)-CLIENT_ID_MIN+1));
  ClientId id = id_dist(id_gen);
  while (client_map.count(id) > 0)
  {
    id = (id < CLIENT_ID_MAX) ? id+1 : CLIENT_ID_MIN;
  }
  client_map[id] = client;
  return id;
} /* ReflectorClient::newClientId */


ReflectorClient::ClientSrc ReflectorClient::newClientSrc(ReflectorClient* client)
{
  ClientSrc src{std::make_pair(client->m_con->remoteHost(),
                               client->m_remote_udp_port)};
  client_src_map[src] = client;
  return src;
} /* ReflectorClient::newClientSrc */


void ReflectorClient::onSslConnectionReady(TcpConnection *con)
{
  //std::cout << "### ReflectorClient::onSslConnectionReady" << std::endl;

  if (m_con_state != STATE_EXPECT_SSL_CON_READY)
  {
    std::cout << "*** ERROR[" << m_con->remoteHost() << ":"
              << m_con->remotePort()
              << "]: SSL connection ready event unexpected" << std::endl;
    disconnect();
    return;
  }

  m_con->setMaxFrameSize(ReflectorMsg::MAX_POST_SSL_SETUP_SIZE);

  Async::SslX509 peer_cert(con->sslPeerCertificate());
  if (peer_cert.isNull())
  {
    std::cout << m_con->remoteHost() << ":" << m_con->remotePort()
              << ": No client certificate. Requesting Certificate "
                 "Signing Request from client." << std::endl;
    sendMsg(MsgClientCsrRequest());
    m_con_state = STATE_EXPECT_CSR;
    return;
    //MsgAuthChallenge challenge_msg;
    //if (challenge_msg.challenge() == nullptr)
    //{
    //  disconnect();
    //  return;
    //}
    //memcpy(m_auth_challenge, challenge_msg.challenge(),
    //       MsgAuthChallenge::LENGTH);
    //sendMsg(challenge_msg);
    //m_con_state = STATE_EXPECT_AUTH_RESPONSE;
    //return;
  }

  std::cout << "------------- Client Certificate --------------" << std::endl;
  peer_cert.print();
  std::cout << "-----------------------------------------------" << std::endl;

  std::string callsign = peer_cert.commonName();
  if (!m_reflector->callsignOk(callsign))
  {
    std::cout << "*** WARNING[" << m_con->remoteHost() << ":"
              << m_con->remotePort()
              << "]: client certificate has invalid CN (callsign)"
              << std::endl;
    disconnect();
    return;
  }

  int days=0, seconds=0;
  peer_cert.timeSpan(days, seconds);
  time_t renew_time = peer_cert.notBefore() +
      (static_cast<time_t>(days)*24*3600 + seconds)*RENEW_AFTER;
  //std::cout << "### Client cert days=" << days << " seconds="
  //          << seconds << " renew_in=" << (renew_time - time(NULL))
  //          << std::endl;
  m_renew_cert_timer.setTimeout(renew_time);
  m_renew_cert_timer.setExpireOffset(10000);
  m_renew_cert_timer.start();

  std::cout << callsign << ": " << peer_cert.subjectNameString() << std::endl;
  connectionAuthenticated(callsign);
} /* ReflectorClient::onSslConnectionReady */


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
    case MsgCABundleRequest::TYPE:
      handleMsgCABundleRequest(ss);
      break;
    case MsgStartEncryptionRequest::TYPE:
      handleMsgStartEncryptionRequest(ss);
      break;
    case MsgAuthResponse::TYPE:
      handleMsgAuthResponse(ss);
      break;
    case MsgClientCsr::TYPE:
      handleMsgClientCsr(ss);
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
    std::cout << "*** ERROR[" << m_con->remoteHost() << ":"
              << m_con->remotePort() << "]: Could not unpack MsgProtoVer"
              << std::endl;
    sendError("Illegal MsgProtoVer protocol message received");
    return;
  }
  m_client_proto_ver.set(msg.majorVer(), msg.minorVer());
  ProtoVer max_proto_ver(MsgProtoVer::MAJOR, MsgProtoVer::MINOR);
  if (m_client_proto_ver > max_proto_ver)
  {
    std::cout << m_con->remoteHost() << ":" << m_con->remotePort()
              << ": Using protocol version "
              << msg.majorVer() << "." << msg.minorVer()
              << " which is newer than we can handle.";
    if (m_con_state == STATE_EXPECT_PROTO_VER)
    {
      std::cout << " Asking for downgrade to "
                << MsgProtoVer::MAJOR << "." << MsgProtoVer::MINOR << ".";
      sendMsg(MsgProtoVerDowngrade());
    }
    else
    {
      std::cout << " Downgrade failed.";
      std::ostringstream ss;
      ss << "Unsupported protocol version " << msg.majorVer() << "."
         << msg.minorVer() << ". May be at most "
         << MsgProtoVer::MAJOR << "." << MsgProtoVer::MINOR << ".";
      sendError(ss.str());
    }
    std::cout << std::endl;
    return;
  }
  else if (m_client_proto_ver < ProtoVer(MIN_MAJOR_VER, MIN_MINOR_VER))
  {
    std::cout << "*** ERROR[" << m_con->remoteHost() << ":"
              << m_con->remotePort()
              << "]: Client is using protocol version "
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

  if (m_client_proto_ver.majorVer() >= 3)
  {
    //std::cout << "### ReflectorClient::handMsgProtoVer: Send CAInfo"
    //          << std::endl;
    m_con->setMaxFrameSize(ReflectorMsg::MAX_PRE_SSL_SETUP_SIZE);
    sendMsg(MsgCAInfo(m_reflector->caSize(), m_reflector->caDigest()));
    m_con_state = STATE_EXPECT_START_ENCRYPTION;
  }
  else
  {
    sendAuthChallenge();
  }
} /* ReflectorClient::handleMsgProtoVer */


void ReflectorClient::handleMsgCABundleRequest(std::istream& is)
{
  //std::cout << "### ReflectorClient::handleMsgCABundleRequest" << std::endl;

  if (m_con_state != STATE_EXPECT_START_ENCRYPTION)
  {
    std::cout << "*** ERROR[" << m_con->remoteHost() << ":"
              << m_con->remotePort() << "]: Unexpected MsgCABundleRequest"
              << std::endl;
    disconnect();
    return;
  }

  std::cout << "### Sending CA Bundle" << std::endl;
  m_con->setMaxFrameSize(ReflectorMsg::MAX_PRE_SSL_SETUP_SIZE);
  sendMsg(MsgCABundle(m_reflector->caBundlePem(), m_reflector->caSignature(),
                      m_reflector->issuingCertPem()));
} /* ReflectorClient::handleMsgCABundleRequest */


void ReflectorClient::handleMsgStartEncryptionRequest(std::istream& is)
{
  //std::cout << "### ReflectorClient::handleMsgStartEncryptionRequest"
  //          << std::endl;

  if (m_con_state != STATE_EXPECT_START_ENCRYPTION)
  {
    std::cout << "*** ERROR[" << m_con->remoteHost() << ":"
              << m_con->remotePort()
              << "]: Unexpected MsgStartEncryptionRequest" << std::endl;
    disconnect();
    return;
  }

  MsgStartEncryptionRequest msg;
  if (!msg.unpack(is))
  {
    std::cerr << "*** ERROR[" << m_con->remoteHost() << ":"
              << m_con->remotePort()
              << "]: Could not unpack MsgStartEncryptionRequest" << std::endl;
    disconnect();
    return;
  }

  std::cout << m_con->remoteHost() << ":" << m_con->remotePort()
            << ": Starting encryption" << std::endl;

  sendMsg(MsgStartEncryption());
  m_con->enableSsl(true);
  m_con_state = STATE_EXPECT_SSL_CON_READY;
} /* ReflectorClient::handleMsgStartEncryptionRequest */


void ReflectorClient::handleMsgAuthResponse(std::istream& is)
{
  if (m_con_state != STATE_EXPECT_AUTH_RESPONSE)
  {
    std::cerr << "*** ERROR[" << m_con->remoteHost() << ":"
              << m_con->remotePort()
              << "]: Authentication response unexpected" << std::endl;
    sendError("Authentication response unexpected");
    return;
  }

  MsgAuthResponse msg;
  if (!msg.unpack(is))
  {
    std::cerr << "*** ERROR[" << m_con->remoteHost() << ":"
              << m_con->remotePort()
              << "]: Could not unpack MsgAuthResponse" << std::endl;
    sendError("Illegal MsgAuthResponse protocol message received");
    return;
  }

  if (!m_reflector->callsignOk(msg.callsign()))
  {
    std::cerr << "*** ERROR[" << m_con->remoteHost() << ":"
              << m_con->remotePort()
              << "]: Invalid node callsign '" << msg.callsign()
              << "' in MsgAuthResponse"
              << std::endl;
    sendError("Invalid callsign");
    return;
  }

  //const auto peer_cert = m_reflector->loadClientCertificate(msg.callsign());
  //if (!peer_cert.isNull())
  //{
  //  std::cout << "Client " << m_con->remoteHost() << ":"
  //            << m_con->remotePort() << " (" << msg.callsign() << "?)"
  //            << ": Sending client certificate to peer" << std::endl;
  //  //sendMsg(MsgClientCert(peer_cert.pem()));
  //  sendClientCert(peer_cert);
  //  //m_con_state = STATE_EXPECT_CSR;
  //  return;
  //}
  //else //if (m_reflector->loadClientPendingCsr(msg.callsign()).isNull())
  //{
  //  std::cout << "Client " << m_con->remoteHost() << ":"
  //            << m_con->remotePort() << " (" << msg.callsign() << "?)"
  //            << ": Sending CSR request to peer" << std::endl;
  //  sendMsg(MsgClientCsrRequest());
  //}

  string auth_key = lookupUserKey(msg.callsign());
  if (!auth_key.empty() && msg.verify(auth_key, m_auth_challenge))
  {
    std::cout << msg.callsign() << ": Received valid auth key" << std::endl;
    connectionAuthenticated(msg.callsign());
  }
  else
  {
    std::cerr << "*** ERROR[" << m_con->remoteHost() << ":"
              << m_con->remotePort() << "]: Authentication failed for user '"
              << msg.callsign() << "'" << std::endl;
    sendError("Access denied");
  }
} /* ReflectorClient::handleMsgAuthResponse */


void ReflectorClient::handleMsgClientCsr(std::istream& is)
{
  std::ostringstream idss;
  if (m_con_state == STATE_CONNECTED)
  {
    idss << m_callsign;
  }
  else
  {
    idss << m_con->remoteHost() << ":" << m_con->remotePort();
  }

  if ((m_con_state != STATE_CONNECTED) && (m_con_state != STATE_EXPECT_CSR))
  {
    std::cerr << "*** ERROR[" << idss.str()
              << "]: Certificate Signing Request unexpected" << std::endl;
    sendError("Certificate Signing Request unexpected");
    return;
  }

  MsgClientCsr msg;
  if (!msg.unpack(is))
  {
    std::cout << "*** ERROR[" << idss.str()
              << "]: Could not unpack MsgClientCsr" << std::endl;
    sendError("Illegal MsgClientCsr protocol message received");
    return;
  }

  std::cerr << idss.str() << ": Received CSR" << std::endl;

  Async::SslCertSigningReq req;
  if (!req.readPem(msg.csrPem()) || req.isNull())
  {
    std::cerr << "*** ERROR[" << idss.str() << "]: Invalid CSR received:\n"
              << msg.csrPem() << std::endl;
    sendError("Invalid CSR received");
    return;
  }
  req.print(idss.str() + ":   ");

  auto cert = csrReceived(req);
  auto current_req = m_reflector->loadClientCsr(req.commonName());
  if ((
        (m_con_state == STATE_EXPECT_CSR) ||
        (!current_req.isNull() && (req.digest() == current_req.digest()))
      ) &&
      sendClientCert(cert))
  {
    //std::cout << "### Sent certificate to peer:" << std::endl;
    //cert.print();
    m_con_state = STATE_EXPECT_DISCONNECT;
  }
  else if (m_con_state == STATE_EXPECT_CSR)
  {
    std::cout << idss.str() << ": No valid certificate found matching CSR. "
                 "Sending authentication challenge." << std::endl;
    sendAuthChallenge();
    m_con_state = STATE_EXPECT_AUTH_RESPONSE;
  }
} /* ReflectorClient::handleMsgClientCsr */


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
  std::string jsonstr;
  if (m_client_proto_ver >= ProtoVer(3, 0))
  {
    MsgNodeInfo msg;
    if (!msg.unpack(is))
    {
      cout << "Client " << m_con->remoteHost() << ":" << m_con->remotePort()
           << " ERROR: Could not unpack MsgNodeInfo" << endl;
      sendError("Illegal MsgNodeInfo protocol message received");
      return;
    }
    //std::cout << "### handleNodeInfo: udpSrcPort()=" << msg.udpSrcPort()
    //          << " JSON=" << msg.json() << std::endl;
    //setRemoteUdpPort(msg.udpSrcPort());
    setUdpCipherIVRand(msg.ivRand());
    setUdpCipherKey(msg.udpCipherKey());
    jsonstr = msg.json();

    sendMsg(MsgStartUdpEncryption());
  }
  else
  {
    MsgNodeInfoV2 msg;
    if (!msg.unpack(is))
    {
      std::cout << "Client " << m_con->remoteHost() << ":"
                << m_con->remotePort()
                << " ERROR: Could not unpack MsgNodeInfoV2" << std::endl;
      sendError("Illegal MsgNodeInfo protocol message received");
      return;
    }
    jsonstr = msg.json();
  }
  try
  {
    std::istringstream is(jsonstr);
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
  cout << "Error message received from client: " << message << endl;
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


void ReflectorClient::connectionAuthenticated(const std::string& callsign)
{
  vector<string> connected_nodes;
  m_reflector->nodeList(connected_nodes);
  if (find(connected_nodes.begin(), connected_nodes.end(),
           callsign) == connected_nodes.end())
  {
    m_con->setMaxFrameSize(ReflectorMsg::MAX_POSTAUTH_FRAME_SIZE);
    m_callsign = callsign;
    sendMsg(MsgAuthOk());
    cout << m_callsign << ": Login OK from "
         << m_con->remoteHost() << ":" << m_con->remotePort()
         << " with protocol version " << m_client_proto_ver.majorVer()
         << "." << m_client_proto_ver.minorVer()
         << endl;
    m_con_state = STATE_CONNECTED;

    assert(client_callsign_map.find(m_callsign) == client_callsign_map.end());
    client_callsign_map[m_callsign] = this;

    //const auto cert = m_reflector->loadClientCertificate(m_callsign);
    //const auto peer_cert = m_con->sslPeerCertificate();
    //if (!cert.isNull() && !peer_cert.isNull() &&
    //    (cert.publicKey() != peer_cert.publicKey()))
    //{
    //  std::cout << m_callsign << ": Requesting CSR from peer" << std::endl;
    //  sendMsg(MsgClientCsrRequest());
    //}

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
    cout << callsign << ": Already connected" << endl;
    sendError("Access denied");
  }
} /* ReflectorClient::connectionAuthenticated */


bool ReflectorClient::sendClientCert(const Async::SslX509& cert)
{
  if (cert.isNull())
  {
    return false;
  }

  const auto callsign = cert.commonName();
  const auto pending_csr = m_reflector->loadClientPendingCsr(callsign);
  if (!pending_csr.isNull() && (cert.publicKey() != pending_csr.publicKey()))
  {
    //std::cout << "### ReflectorClient::sendClientCert: Cert public key "
    //             "differs compared to pending CSR public key" << std::endl;
    //sendMsg(MsgClientCert());
    return false;
  }
  return sendMsg(MsgClientCert(m_reflector->clientCertPem(callsign)));
} /* ReflectorClient::sendClientCert */


void ReflectorClient::sendAuthChallenge(void)
{
  MsgAuthChallenge challenge_msg;
  if (challenge_msg.challenge() == nullptr)
  {
    disconnect();
    return;
  }
  memcpy(m_auth_challenge, challenge_msg.challenge(),
         MsgAuthChallenge::LENGTH);
  sendMsg(challenge_msg);
  m_con_state = STATE_EXPECT_AUTH_RESPONSE;
}


void ReflectorClient::renewClientCertificate(void)
{
  std::cout << m_callsign << ": Renew client certificate" << std::endl;
  auto cert = m_con->sslPeerCertificate();
  if (cert.isNull() || !m_reflector->signClientCert(cert))
  {
    std::cerr << "*** WARNING: Certificate resigning for '"
              << m_callsign << "' failed" << std::endl;
    return;
  }
  sendClientCert(cert);
  m_con_state = STATE_EXPECT_DISCONNECT;
} /* ReflectorClient::renewClientCertificate */


/*
 * This file has not been truncated
 */

