/**
@file   FederationPeerConnection.cpp
@brief  Outgoing V2 connection to a federated SVXReflector peer
@author Chris Jackson / G4NAB
@date   2026-08-15

\verbatim
Copyright (C) 2026 Chris Jackson / G4NAB

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.
\endverbatim
*/


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <algorithm>
#include <iostream>
#include <sstream>

/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncUdpSocket.h>

/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "FederationPeerConnection.h"
#include "FederationMsg.h"
#include "ReflectorMsg.h"


/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/

using namespace Async;
using namespace sigc;


/****************************************************************************
 *
 * Local constants
 *
 ****************************************************************************/

namespace
{
  const unsigned RECONNECT_INTERVAL_MS = 60000;
  const unsigned UDP_HEARTBEAT_TX_RESET = 15;
  const unsigned UDP_HEARTBEAT_RX_RESET = 60;
  const unsigned TCP_HEARTBEAT_TX_RESET = 10;
  const unsigned TCP_HEARTBEAT_RX_RESET = 15;
  const std::size_t MAX_PENDING_AUDIO_FRAMES = 25;
}


/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/

FederationPeerConnection::FederationPeerConnection(
    const std::string& peer,
    const std::string& remote_reflector_id,
    const std::string& local_reflector_id,
    const std::string& local_domain,
    std::uint64_t library_generation,
    const std::string& callsign,
    const std::string& host,
    std::uint16_t port,
    const std::string& auth_key)
  : m_peer(peer),
    m_remote_reflector_id(remote_reflector_id),
    m_local_reflector_id(local_reflector_id),
    m_local_domain(local_domain),
    m_library_generation(library_generation),
    m_callsign(callsign),
    m_host(host),
    m_port(port),
    m_auth_key(auth_key),
    m_udp_sock(0),
    m_reconnect_timer(
        RECONNECT_INTERVAL_MS,
        Timer::TYPE_ONESHOT,
        false),
    m_heartbeat_timer(
        1000,
        Timer::TYPE_PERIODIC,
        false),
    m_state(STATE_DISCONNECTED),
    m_started(false),
    m_udp_registered(false),
    m_client_id(0),
    m_next_udp_tx_sequence(0),
    m_next_udp_rx_sequence(0),
    m_udp_heartbeat_tx_count(0),
    m_udp_heartbeat_rx_count(0),
    m_tcp_heartbeat_tx_count(0),
    m_tcp_heartbeat_rx_count(0)
{
  m_con.addStaticSRVRecord(
      0,
      100,
      100,
      m_port,
      m_host);

  m_con.connected.connect(
      mem_fun(*this, &FederationPeerConnection::onConnected));

  m_con.disconnected.connect(
      mem_fun(*this, &FederationPeerConnection::onDisconnected));

  m_con.frameReceived.connect(
      mem_fun(*this, &FederationPeerConnection::onFrameReceived));

  m_reconnect_timer.expired.connect(
      hide(mem_fun(*this, &FederationPeerConnection::reconnect)));

  m_heartbeat_timer.expired.connect(
      mem_fun(*this, &FederationPeerConnection::heartbeatTick));

  m_con.setMaxFrameSize(ReflectorMsg::MAX_PREAUTH_FRAME_SIZE);
} /* FederationPeerConnection::FederationPeerConnection */


FederationPeerConnection::~FederationPeerConnection(void)
{
  stop();

  delete m_udp_sock;
  m_udp_sock = 0;
} /* FederationPeerConnection::~FederationPeerConnection */


void FederationPeerConnection::start(void)
{
  if (m_started)
  {
    return;
  }

  m_started = true;
  connect();
} /* FederationPeerConnection::start */


void FederationPeerConnection::stop(void)
{
  m_started = false;
  m_reconnect_timer.setEnable(false);
  m_heartbeat_timer.setEnable(false);
  disconnect();
} /* FederationPeerConnection::stop */


const FederationPeerConnection::OutgoingStream*
FederationPeerConnection::findOutgoingStream(
    std::uint32_t tg) const
{
  std::map<std::uint32_t, OutgoingStream>::const_iterator it =
      m_outgoing_streams.find(tg);

  return (it == m_outgoing_streams.end())
      ? 0
      : &it->second;
} /* FederationPeerConnection::findOutgoingStream */


bool FederationPeerConnection::startOutgoingStream(
    std::uint32_t tg,
    std::uint64_t stream_id,
    const std::string& source_callsign,
    const std::string& codec,
    std::string& error)
{
  error.clear();

  if (!isConnected())
  {
    error = "Federation peer is not connected";
    return false;
  }

  if (!isUdpRegistered())
  {
    error = "Federation peer UDP path is not registered";
    return false;
  }

  if (tg == 0)
  {
    error = "Talkgroup zero is not a valid federation stream";
    return false;
  }

  if (stream_id == 0)
  {
    error = "Stream ID zero is invalid";
    return false;
  }

  if (source_callsign.empty())
  {
    error = "Stream source callsign is missing";
    return false;
  }

  if (codec != "OPUS")
  {
    error = "Only OPUS federation streams are supported";
    return false;
  }

  if (m_outgoing_streams.find(tg) !=
      m_outgoing_streams.end())
  {
    error = "Talkgroup already has an outgoing federation stream";
    return false;
  }

  OutgoingStream stream;
  stream.tg = tg;
  stream.stream_id = stream_id;
  stream.source_callsign = source_callsign;
  stream.codec = codec;
  stream.state = OUTGOING_STREAM_PENDING;

  m_outgoing_streams[tg] = stream;

  sendMsg(MsgFederationStreamStart(
      m_local_reflector_id,
      tg,
      stream_id,
      source_callsign,
      codec));

  std::cout << "Federation peer " << m_peer
            << ": Requested outgoing stream:"
            << " tg=" << tg
            << " stream_id=" << stream_id
            << " source=" << source_callsign
            << " codec=" << codec
            << std::endl;

  return true;
} /* FederationPeerConnection::startOutgoingStream */


bool FederationPeerConnection::stopOutgoingStream(
    std::uint32_t tg,
    std::uint64_t stream_id,
    std::string& error)
{
  error.clear();

  if (!isConnected())
  {
    error = "Federation peer is not connected";
    return false;
  }

  std::map<std::uint32_t, OutgoingStream>::iterator it =
      m_outgoing_streams.find(tg);

  if (it == m_outgoing_streams.end())
  {
    error = "Outgoing federation stream is not active";
    return false;
  }

  if (it->second.stream_id != stream_id)
  {
    error = "Outgoing federation stream identity does not match";
    return false;
  }

  sendMsg(MsgFederationStreamStop(
      m_local_reflector_id,
      tg,
      stream_id));

  m_outgoing_streams.erase(it);

  std::cout << "Federation peer " << m_peer
            << ": Stopped outgoing stream:"
            << " tg=" << tg
            << " stream_id=" << stream_id
            << std::endl;

  return true;
} /* FederationPeerConnection::stopOutgoingStream */


bool FederationPeerConnection::sendOutgoingAudio(
    std::uint32_t tg,
    std::uint64_t stream_id,
    const std::vector<std::uint8_t>& audio_data,
    std::string& error)
{
  error.clear();

  if (!isConnected())
  {
    error = "Federation peer is not connected";
    return false;
  }

  if (!isUdpRegistered())
  {
    error = "Federation peer UDP path is not registered";
    return false;
  }

  std::map<std::uint32_t, OutgoingStream>::iterator it =
      m_outgoing_streams.find(tg);

  if (it == m_outgoing_streams.end())
  {
    error = "Outgoing federation stream does not exist";
    return false;
  }

  if (stream_id == 0)
  {
    error = "Stream ID zero is invalid";
    return false;
  }

  if (it->second.stream_id != stream_id)
  {
    error = "Outgoing federation stream identity does not match";
    return false;
  }

  if (audio_data.empty())
  {
    error = "Federation audio data is empty";
    return false;
  }

  if (it->second.state == OUTGOING_STREAM_PENDING)
  {
    if (it->second.pending_audio.size() >=
        MAX_PENDING_AUDIO_FRAMES)
    {
      it->second.pending_audio.pop_front();
    }

    it->second.pending_audio.push_back(audio_data);
    return true;
  }

  if (it->second.state != OUTGOING_STREAM_ACTIVE)
  {
    error = "Outgoing federation stream has an invalid state";
    return false;
  }

  sendOutgoingAudioFrame(it->second, audio_data);
  return true;
} /* FederationPeerConnection::sendOutgoingAudio */


/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/

void FederationPeerConnection::sendOutgoingAudioFrame(
    OutgoingStream& stream,
    const std::vector<std::uint8_t>& audio_data)
{
  MsgUdpFederationAudio msg(
      m_local_reflector_id,
      stream.tg,
      stream.stream_id,
      stream.next_audio_sequence,
      audio_data);

  sendUdpMsg(msg);
  ++stream.next_audio_sequence;
} /* FederationPeerConnection::sendOutgoingAudioFrame */


void FederationPeerConnection::connect(void)
{
  if (!m_started || m_con.isConnected())
  {
    return;
  }

  m_reconnect_timer.setEnable(false);

  std::cout << "Federation peer " << m_peer
            << ": Connecting to "
            << m_host << ":" << m_port
            << std::endl;

  m_con.connect();
} /* FederationPeerConnection::connect */


void FederationPeerConnection::disconnect(void)
{
  const bool was_connected = m_con.isConnected();
  m_con.disconnect();

  if (was_connected)
  {
    onDisconnected(
        &m_con,
        TcpConnection::DR_ORDERED_DISCONNECT);
  }

  m_state = STATE_DISCONNECTED;
} /* FederationPeerConnection::disconnect */


void FederationPeerConnection::reconnect(void)
{
  if (!m_started)
  {
    return;
  }

  disconnect();
  connect();
} /* FederationPeerConnection::reconnect */


void FederationPeerConnection::onConnected(void)
{
  std::cout << "Federation peer " << m_peer
            << ": TCP connection established to "
            << m_con.remoteHost() << ":"
            << m_con.remotePort()
            << std::endl;

  m_state = STATE_EXPECT_AUTH_CHALLENGE;
  m_udp_registered = false;
  m_client_id = 0;
  m_next_udp_tx_sequence = 0;
  m_next_udp_rx_sequence = 0;
  m_udp_heartbeat_tx_count = UDP_HEARTBEAT_TX_RESET;
  m_udp_heartbeat_rx_count = UDP_HEARTBEAT_RX_RESET;
  m_tcp_heartbeat_tx_count = TCP_HEARTBEAT_TX_RESET;
  m_tcp_heartbeat_rx_count = TCP_HEARTBEAT_RX_RESET;

  m_con.setMaxFrameSize(ReflectorMsg::MAX_PREAUTH_FRAME_SIZE);
  m_heartbeat_timer.setEnable(true);

  sendMsg(MsgProtoVer(2, 0));
} /* FederationPeerConnection::onConnected */


void FederationPeerConnection::onDisconnected(
    Async::TcpConnection* con,
    Async::TcpConnection::DisconnectReason reason)
{
  (void)con;

  std::cout << "Federation peer " << m_peer
            << ": Disconnected: "
            << TcpConnection::disconnectReasonStr(reason)
            << std::endl;

  m_heartbeat_timer.setEnable(false);
  delete m_udp_sock;
  m_udp_sock = 0;
  m_udp_registered = false;
  m_state = STATE_DISCONNECTED;
  m_client_id = 0;
  m_next_udp_tx_sequence = 0;
  m_next_udp_rx_sequence = 0;
  m_udp_heartbeat_tx_count = 0;
  m_udp_heartbeat_rx_count = 0;
  m_outgoing_streams.clear();
  m_tcp_heartbeat_tx_count = 0;
  m_tcp_heartbeat_rx_count = 0;

  if (m_started)
  {
    m_reconnect_timer.setEnable(true);
  }
} /* FederationPeerConnection::onDisconnected */


void FederationPeerConnection::onFrameReceived(
    Async::FramedTcpConnection* con,
    std::vector<std::uint8_t>& data)
{
  (void)con;

  if (data.empty())
  {
    std::cerr << "*** ERROR: Federation peer "
              << m_peer << ": Empty TCP frame received"
              << std::endl;
    disconnect();
    return;
  }

  std::stringstream stream;
  stream.write(
      reinterpret_cast<const char*>(&data.front()),
      data.size());

  ReflectorMsg header;
  if (!header.unpack(stream))
  {
    std::cerr << "*** ERROR: Federation peer "
              << m_peer
              << ": Could not unpack TCP message header"
              << std::endl;
    disconnect();
    return;
  }

  if ((header.type() >= 100) &&
      (m_state < STATE_EXPECT_SERVER_INFO))
  {
    std::cerr << "*** ERROR: Federation peer "
              << m_peer
              << ": User message received before authentication"
              << std::endl;
    disconnect();
    return;
  }

  m_tcp_heartbeat_rx_count = TCP_HEARTBEAT_RX_RESET;

  switch (header.type())
  {
    case MsgHeartbeat::TYPE:
      break;

    case MsgError::TYPE:
    {
      MsgError msg;
      if (!msg.unpack(stream))
      {
        std::cerr << "*** ERROR: Federation peer "
                  << m_peer
                  << ": Could not unpack MsgError"
                  << std::endl;
      }
      else
      {
        std::cerr << "*** ERROR: Federation peer "
                  << m_peer << ": "
                  << msg.message()
                  << std::endl;
      }
      disconnect();
      break;
    }

    case MsgProtoVerDowngrade::TYPE:
      std::cerr << "*** ERROR: Federation peer "
                << m_peer
                << ": Remote reflector rejected V2 protocol"
                << std::endl;
      disconnect();
      break;

    case MsgAuthChallenge::TYPE:
      handleAuthChallenge(stream);
      break;

    case MsgAuthOk::TYPE:
      if (m_state != STATE_EXPECT_AUTH_OK)
      {
        std::cerr << "*** ERROR: Federation peer "
                  << m_peer
                  << ": Unexpected MsgAuthOk"
                  << std::endl;
        disconnect();
        return;
      }

      std::cout << "Federation peer " << m_peer
                << ": V2 authentication accepted"
                << std::endl;

      m_state = STATE_EXPECT_SERVER_INFO;
      m_con.setMaxFrameSize(
          ReflectorMsg::MAX_POSTAUTH_FRAME_SIZE);
      break;

    case MsgServerInfo::TYPE:
      handleServerInfo(stream);
      break;

    case MsgFederationHelloAck::TYPE:
      handleFederationAck(stream);
      break;

    case MsgFederationStreamResult::TYPE:
      handleFederationStreamResult(stream);
      break;

    default:
      break;
  }
} /* FederationPeerConnection::onFrameReceived */


void FederationPeerConnection::handleAuthChallenge(
    std::istream& is)
{
  if (m_state != STATE_EXPECT_AUTH_CHALLENGE)
  {
    std::cerr << "*** ERROR: Federation peer "
              << m_peer
              << ": Unexpected MsgAuthChallenge"
              << std::endl;
    disconnect();
    return;
  }

  MsgAuthChallenge msg;
  if (!msg.unpack(is) || (msg.challenge() == 0))
  {
    std::cerr << "*** ERROR: Federation peer "
              << m_peer
              << ": Invalid authentication challenge"
              << std::endl;
    disconnect();
    return;
  }

  sendMsg(MsgAuthResponse(
      m_callsign,
      m_auth_key,
      msg.challenge()));

  m_state = STATE_EXPECT_AUTH_OK;
} /* FederationPeerConnection::handleAuthChallenge */


void FederationPeerConnection::handleServerInfo(
    std::istream& is)
{
  if (m_state != STATE_EXPECT_SERVER_INFO)
  {
    std::cerr << "*** ERROR: Federation peer "
              << m_peer
              << ": Unexpected MsgServerInfo"
              << std::endl;
    disconnect();
    return;
  }

  MsgServerInfo msg;
  if (!msg.unpack(is))
  {
    std::cerr << "*** ERROR: Federation peer "
              << m_peer
              << ": Could not unpack MsgServerInfo"
              << std::endl;
    disconnect();
    return;
  }

  if (msg.clientId() == 0)
  {
    std::cerr << "*** ERROR: Federation peer "
              << m_peer
              << ": Remote reflector supplied client ID zero"
              << std::endl;
    disconnect();
    return;
  }

  const std::vector<std::string>& codecs = msg.codecs();
  if (std::find(codecs.begin(), codecs.end(), "OPUS") ==
      codecs.end())
  {
    std::cerr << "*** ERROR: Federation peer "
              << m_peer
              << ": Remote reflector does not advertise OPUS"
              << std::endl;
    disconnect();
    return;
  }

  m_client_id = msg.clientId();

  delete m_udp_sock;
  m_udp_sock = new UdpSocket;
  m_udp_sock->dataReceived.connect(
      mem_fun(
          *this,
          &FederationPeerConnection::udpDatagramReceived));

  m_state = STATE_EXPECT_FEDERATION_ACK;

  sendMsg(MsgFederationHello(
      FederationProtocol::VERSION_MAJOR,
      FederationProtocol::VERSION_MINOR,
      m_local_reflector_id,
      m_local_domain,
      m_library_generation,
      FederationProtocol::CAP_MULTIPLEXED_OPUS));
} /* FederationPeerConnection::handleServerInfo */


void FederationPeerConnection::handleFederationAck(
    std::istream& is)
{
  if (m_state != STATE_EXPECT_FEDERATION_ACK)
  {
    std::cerr << "*** ERROR: Federation peer "
              << m_peer
              << ": Unexpected MsgFederationHelloAck"
              << std::endl;
    disconnect();
    return;
  }

  MsgFederationHelloAck msg;
  if (!msg.unpack(is))
  {
    std::cerr << "*** ERROR: Federation peer "
              << m_peer
              << ": Could not unpack MsgFederationHelloAck"
              << std::endl;
    disconnect();
    return;
  }

  if (msg.major() != FederationProtocol::VERSION_MAJOR)
  {
    std::cerr << "*** ERROR: Federation peer "
              << m_peer
              << ": Federation major version mismatch"
              << std::endl;
    disconnect();
    return;
  }

  if (msg.minor() > FederationProtocol::VERSION_MINOR)
  {
    std::cerr << "*** ERROR: Federation peer "
              << m_peer
              << ": Invalid negotiated federation minor version"
              << std::endl;
    disconnect();
    return;
  }

  if (msg.reflectorId() != m_remote_reflector_id)
  {
    std::cerr << "*** ERROR: Federation peer "
              << m_peer
              << ": Federation acknowledgment identity mismatch"
              << std::endl;
    disconnect();
    return;
  }

  if (msg.domain() != m_peer)
  {
    std::cerr << "*** ERROR: Federation peer "
              << m_peer
              << ": Federation acknowledgment domain mismatch"
              << std::endl;
    disconnect();
    return;
  }

  if ((msg.capabilities() &
       FederationProtocol::CAP_MULTIPLEXED_OPUS) == 0)
  {
    std::cerr << "*** ERROR: Federation peer "
              << m_peer
              << ": Mandatory multiplexed OPUS capability missing"
              << std::endl;
    disconnect();
    return;
  }

  m_state = STATE_CONNECTED;
  m_udp_heartbeat_tx_count = UDP_HEARTBEAT_TX_RESET;
  m_udp_heartbeat_rx_count = UDP_HEARTBEAT_RX_RESET;

  sendUdpMsg(MsgUdpHeartbeat());

  std::cout << "Federation peer " << m_peer
            << ": Federation session established:"
            << " reflector_id=" << msg.reflectorId()
            << " version=" << msg.major()
            << "." << msg.minor()
            << " capabilities=" << msg.capabilities()
            << std::endl;
} /* FederationPeerConnection::handleFederationAck */


void FederationPeerConnection::handleFederationStreamResult(
    std::istream& is)
{
  if (!isConnected())
  {
    std::cerr << "*** ERROR: Federation peer "
              << m_peer
              << ": Stream result received before federation connection"
              << std::endl;
    disconnect();
    return;
  }

  MsgFederationStreamResult msg;
  if (!msg.unpack(is))
  {
    std::cerr << "*** ERROR: Federation peer "
              << m_peer
              << ": Could not unpack MsgFederationStreamResult"
              << std::endl;
    disconnect();
    return;
  }

  if (msg.originReflectorId() != m_local_reflector_id)
  {
    std::cerr << "*** ERROR: Federation peer "
              << m_peer
              << ": Stream result origin identity mismatch"
              << std::endl;
    disconnect();
    return;
  }

  std::map<std::uint32_t, OutgoingStream>::iterator it =
      m_outgoing_streams.find(msg.tg());

  if ((it == m_outgoing_streams.end()) ||
      (it->second.stream_id != msg.streamId()) ||
      (it->second.state != OUTGOING_STREAM_PENDING))
  {
    std::cerr << "*** WARNING: Federation peer "
              << m_peer
              << ": Stream result does not match a pending stream:"
              << " tg=" << msg.tg()
              << " stream_id=" << msg.streamId()
              << std::endl;
    return;
  }

  if (msg.accepted() &&
      (msg.reason() == FederationProtocol::STREAM_ACCEPTED))
  {
    it->second.state = OUTGOING_STREAM_ACTIVE;

    const std::size_t queued_frames =
        it->second.pending_audio.size();

    while (!it->second.pending_audio.empty())
    {
      const std::vector<std::uint8_t> audio_data(
          it->second.pending_audio.front());

      it->second.pending_audio.pop_front();
      sendOutgoingAudioFrame(it->second, audio_data);
    }

    std::cout << "Federation peer " << m_peer
              << ": Outgoing stream accepted:"
              << " tg=" << msg.tg()
              << " stream_id=" << msg.streamId()
              << " queued_audio_frames=" << queued_frames
              << std::endl;
    return;
  }

  if (msg.accepted() ||
      (msg.reason() == FederationProtocol::STREAM_ACCEPTED))
  {
    std::cerr << "*** ERROR: Federation peer "
              << m_peer
              << ": Inconsistent stream result"
              << std::endl;
    m_outgoing_streams.erase(it);
    disconnect();
    return;
  }

  std::cerr << "*** WARNING: Federation peer "
            << m_peer
            << ": Outgoing stream rejected:"
            << " tg=" << msg.tg()
            << " stream_id=" << msg.streamId()
            << " reason=" << msg.reason()
            << " detail=" << msg.detail()
            << std::endl;

  m_outgoing_streams.erase(it);
} /* FederationPeerConnection::handleFederationStreamResult */


void FederationPeerConnection::udpDatagramReceived(
    const Async::IpAddress& address,
    std::uint16_t port,
    void* buffer,
    int count)
{
  if (!isConnected() || (buffer == 0) || (count <= 0))
  {
    return;
  }

  if (address != m_con.remoteHost())
  {
    std::cerr << "*** WARNING: Federation peer "
              << m_peer
              << ": UDP packet received from wrong address "
              << address
              << std::endl;
    return;
  }

  if (port != m_con.remotePort())
  {
    std::cerr << "*** WARNING: Federation peer "
              << m_peer
              << ": UDP packet received from wrong port "
              << port
              << std::endl;
    return;
  }

  std::stringstream stream;
  stream.write(
      reinterpret_cast<const char*>(buffer),
      count);

  ReflectorUdpMsgV2 header;
  if (!header.unpack(stream))
  {
    std::cerr << "*** WARNING: Federation peer "
              << m_peer
              << ": Could not unpack V2 UDP header"
              << std::endl;
    return;
  }

  if (header.clientId() != m_client_id)
  {
    std::cerr << "*** WARNING: Federation peer "
              << m_peer
              << ": UDP client ID mismatch"
              << std::endl;
    return;
  }

  const std::uint16_t difference =
      header.sequenceNum() - m_next_udp_rx_sequence;

  if (difference > 0x7fff)
  {
    std::cerr << "*** WARNING: Federation peer "
              << m_peer
              << ": Dropping out-of-order UDP packet"
              << std::endl;
    return;
  }

  if (difference > 0)
  {
    std::cout << "Federation peer " << m_peer
              << ": UDP packet loss: expected="
              << m_next_udp_rx_sequence
              << " received=" << header.sequenceNum()
              << std::endl;
  }

  m_next_udp_rx_sequence = header.sequenceNum() + 1;
  m_udp_heartbeat_rx_count = UDP_HEARTBEAT_RX_RESET;

  switch (header.type())
  {
    case MsgUdpHeartbeat::TYPE:
      if (!m_udp_registered)
      {
        m_udp_registered = true;

        std::cout << "Federation peer " << m_peer
                  << ": V2 UDP path registered"
                  << std::endl;
      }
      break;

    default:
      break;
  }
} /* FederationPeerConnection::udpDatagramReceived */


void FederationPeerConnection::sendUdpMsg(
    const ReflectorUdpMsg& msg)
{
  if (!isConnected() ||
      (m_udp_sock == 0) ||
      (m_client_id == 0))
  {
    return;
  }

  m_udp_heartbeat_tx_count = UDP_HEARTBEAT_TX_RESET;

  ReflectorUdpMsgV2 header(
      msg.type(),
      m_client_id,
      m_next_udp_tx_sequence++);

  std::ostringstream stream;
  if (!header.pack(stream) || !msg.pack(stream))
  {
    std::cerr << "*** ERROR: Federation peer "
              << m_peer
              << ": Could not pack V2 UDP message"
              << std::endl;
    return;
  }

  const std::string datagram(stream.str());
  m_udp_sock->write(
      m_con.remoteHost(),
      m_con.remotePort(),
      datagram.data(),
      datagram.size());
} /* FederationPeerConnection::sendUdpMsg */


void FederationPeerConnection::heartbeatTick(
    Async::Timer* timer)
{
  (void)timer;

  if (m_state == STATE_DISCONNECTED)
  {
    return;
  }

  if (m_state == STATE_CONNECTED)
  {
    if ((m_udp_heartbeat_tx_count > 0) &&
        (--m_udp_heartbeat_tx_count == 0))
    {
      sendUdpMsg(MsgUdpHeartbeat());
    }

    if ((m_udp_heartbeat_rx_count > 0) &&
        (--m_udp_heartbeat_rx_count == 0))
    {
      std::cerr << "*** ERROR: Federation peer "
                << m_peer
                << ": UDP heartbeat timeout"
                << std::endl;
      disconnect();
      return;
    }
  }

  if ((m_tcp_heartbeat_tx_count > 0) &&
      (--m_tcp_heartbeat_tx_count == 0))
  {
    sendMsg(MsgHeartbeat());
  }

  if ((m_tcp_heartbeat_rx_count > 0) &&
      (--m_tcp_heartbeat_rx_count == 0))
  {
    std::cerr << "*** ERROR: Federation peer "
              << m_peer
              << ": TCP heartbeat timeout"
              << std::endl;
    disconnect();
  }
} /* FederationPeerConnection::heartbeatTick */


void FederationPeerConnection::sendMsg(
    const ReflectorMsg& msg)
{
  if (!m_con.isConnected())
  {
    return;
  }

  if ((msg.type() >= 100) &&
      (m_state < STATE_EXPECT_FEDERATION_ACK))
  {
    std::cerr << "*** ERROR: Federation peer "
              << m_peer
              << ": Attempt to send user message before authentication"
              << std::endl;
    disconnect();
    return;
  }

  m_tcp_heartbeat_tx_count = TCP_HEARTBEAT_TX_RESET;

  std::ostringstream stream;
  ReflectorMsg header(msg.type());

  if (!header.pack(stream) || !msg.pack(stream))
  {
    std::cerr << "*** ERROR: Federation peer "
              << m_peer
              << ": Could not pack TCP message"
              << std::endl;
    disconnect();
    return;
  }

  const std::string frame(stream.str());
  if (m_con.write(frame.data(), frame.size()) == -1)
  {
    disconnect();
  }
} /* FederationPeerConnection::sendMsg */
