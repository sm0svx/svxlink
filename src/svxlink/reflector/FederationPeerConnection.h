/**
@file   FederationPeerConnection.h
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

#ifndef FEDERATION_PEER_CONNECTION_INCLUDED
#define FEDERATION_PEER_CONNECTION_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <cstdint>
#include <deque>
#include <istream>
#include <map>
#include <string>
#include <vector>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncFramedTcpConnection.h>
#include <AsyncIpAddress.h>
#include <AsyncTcpPrioClient.h>
#include <AsyncTimer.h>


/****************************************************************************
 *
 * Forward declarations
 *
 ****************************************************************************/

namespace Async
{
  class UdpSocket;
}

class ReflectorMsg;
class ReflectorUdpMsg;


/****************************************************************************
 *
 * Class definitions
 *
 ****************************************************************************/

/**
@brief  Maintain one outgoing V2 federation peer connection

The TCP connection is configured during initialization but remains inert until
start() is called. A successfully authenticated V2 connection is upgraded to a
federation session using MsgFederationHello.
*/
class FederationPeerConnection : public sigc::trackable
{
  public:
    enum State
    {
      STATE_DISCONNECTED,
      STATE_EXPECT_AUTH_CHALLENGE,
      STATE_EXPECT_AUTH_OK,
      STATE_EXPECT_SERVER_INFO,
      STATE_EXPECT_FEDERATION_ACK,
      STATE_CONNECTED
    };

    enum OutgoingStreamState
    {
      OUTGOING_STREAM_PENDING,
      OUTGOING_STREAM_ACTIVE
    };

    struct OutgoingStream
    {
      std::uint32_t       tg;
      std::uint64_t       stream_id;
      std::string         source_callsign;
      std::string         codec;
      OutgoingStreamState state;
      std::uint32_t       next_audio_sequence;
      std::deque<std::vector<std::uint8_t> > pending_audio;
      OutgoingStream(void)
        : tg(0),
          stream_id(0),
          state(OUTGOING_STREAM_PENDING),
          next_audio_sequence(0)
      {
      }
    };

    FederationPeerConnection(
        const std::string& peer,
        const std::string& remote_reflector_id,
        const std::string& local_reflector_id,
        const std::string& local_domain,
        std::uint64_t library_generation,
        const std::string& callsign,
        const std::string& host,
        std::uint16_t port,
        const std::string& auth_key);

    ~FederationPeerConnection(void);

    const std::string& peer(void) const { return m_peer; }
    const std::string& host(void) const { return m_host; }
    std::uint16_t port(void) const { return m_port; }
    State state(void) const { return m_state; }

    bool isConnected(void) const
    {
      return m_state == STATE_CONNECTED;
    }

    bool isUdpRegistered(void) const
    {
      return m_udp_registered;
    }

    void start(void);
    void stop(void);

    bool startOutgoingStream(
        std::uint32_t tg,
        std::uint64_t stream_id,
        const std::string& source_callsign,
        const std::string& codec,
        std::string& error);

    bool stopOutgoingStream(
        std::uint32_t tg,
        std::uint64_t stream_id,
        std::string& error);

    bool sendOutgoingAudio(
        std::uint32_t tg,
        std::uint64_t stream_id,
        const std::vector<std::uint8_t>& audio_data,
        std::string& error);

    const OutgoingStream* findOutgoingStream(
        std::uint32_t tg) const;

    std::size_t outgoingStreamCount(void) const
    {
      return m_outgoing_streams.size();
    }

  private:
    typedef Async::TcpPrioClient<
        Async::FramedTcpConnection> FramedTcpClient;

    std::string       m_peer;
    std::string       m_remote_reflector_id;
    std::string       m_local_reflector_id;
    std::string       m_local_domain;
    std::uint64_t     m_library_generation;
    std::string       m_callsign;
    std::string       m_host;
    std::uint16_t     m_port;
    std::string       m_auth_key;
    FramedTcpClient   m_con;
    Async::UdpSocket*  m_udp_sock;
    Async::Timer      m_reconnect_timer;
    Async::Timer      m_heartbeat_timer;
    State             m_state;
    bool              m_started;
    bool              m_udp_registered;
    std::uint32_t     m_client_id;
    std::uint16_t     m_next_udp_tx_sequence;
    std::uint16_t     m_next_udp_rx_sequence;
    unsigned          m_udp_heartbeat_tx_count;
    unsigned          m_udp_heartbeat_rx_count;
    unsigned          m_tcp_heartbeat_tx_count;
    unsigned          m_tcp_heartbeat_rx_count;
    std::map<std::uint32_t, OutgoingStream> m_outgoing_streams;

    FederationPeerConnection(const FederationPeerConnection&);
    FederationPeerConnection& operator=(
        const FederationPeerConnection&);

    void connect(void);
    void disconnect(void);
    void reconnect(void);

    void onConnected(void);
    void onDisconnected(
        Async::TcpConnection* con,
        Async::TcpConnection::DisconnectReason reason);

    void onFrameReceived(
        Async::FramedTcpConnection* con,
        std::vector<std::uint8_t>& data);

    void handleAuthChallenge(std::istream& is);
    void handleServerInfo(std::istream& is);
    void handleFederationAck(std::istream& is);
    void handleFederationStreamResult(std::istream& is);
    void udpDatagramReceived(
        const Async::IpAddress& address,
        std::uint16_t port,
        void* buffer,
        int count);

    void sendOutgoingAudioFrame(
        OutgoingStream& stream,
        const std::vector<std::uint8_t>& audio_data);

    void sendUdpMsg(const ReflectorUdpMsg& msg);

    void heartbeatTick(Async::Timer* timer);

    void sendMsg(const ReflectorMsg& msg);
}; /* class FederationPeerConnection */


#endif /* FEDERATION_PEER_CONNECTION_INCLUDED */
