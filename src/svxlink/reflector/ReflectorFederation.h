/**
@file   ReflectorFederation.h
@brief  Federation support for SVXReflector
@author Chris Jackson / G4NAB
@date   2026-08-14

\verbatim
Copyright (C) 2026 Chris Jackson / G4NAB

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.
\endverbatim
*/

#ifndef REFLECTOR_FEDERATION_INCLUDED
#define REFLECTOR_FEDERATION_INCLUDED

/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <cstddef>
#include <cstdint>
#include <map>
#include <sigc++/sigc++.h>
#include <string>
#include <vector>

/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncConfig.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "FederationLibrary.h"

/****************************************************************************
 *
 * Class definitions
 *
 ****************************************************************************/

class ReflectorClient;

class FederationPeerConnection;

/**
@brief  Manage federation between autonomous SVXReflectors

This class will own federation peers, talkgroup routing policy and runtime
federation state. The initial implementation is deliberately inert.
*/
class ReflectorFederation
{
  public:
    struct PeerConfig
    {
      std::string   name;
      std::string   reflector_id;
      std::string   host;
      std::uint16_t port;
      unsigned      protocol;
      std::string   auth_key;
      bool          connect;

      PeerConfig(void)
        : port(5300), protocol(2), connect(true)
      {
      }
    };

    struct TrustEntry
    {
      std::string callsign;
      std::string peer;
    };

    struct IncomingStream
    {
      std::string   peer;
      std::string   origin_reflector_id;
      std::uint32_t tg;
      std::uint64_t stream_id;
      std::string   source_callsign;
      std::string   codec;
      std::uint32_t last_sequence;
      bool          sequence_seen;

      IncomingStream(void)
        : tg(0),
          stream_id(0),
          last_sequence(0),
          sequence_seen(false)
      {
      }
    };

    struct LocalStream
    {
      std::uint32_t tg;
      std::uint64_t stream_id;
      std::string   source_callsign;
      std::string   codec;
      bool          export_requested;

      LocalStream(void)
        : tg(0),
          stream_id(0),
          export_requested(false)
      {
      }
    };

    sigc::signal<void(std::uint32_t, const std::string&)>
        incomingStreamStarted;

    sigc::signal<void(std::uint32_t, const std::string&)>
        incomingStreamStopped;


    ReflectorFederation(void);
    ~ReflectorFederation(void);

    bool initialize(Async::Config& cfg);
    bool isEnabled(void) const { return m_enabled; }

    const std::string& domain(void) const { return m_domain; }
    const std::string& reflectorId(void) const { return m_reflector_id; }
    const std::string& callsign(void) const { return m_callsign; }
    const std::string& libraryPath(void) const { return m_library_path; }

    const std::vector<std::string>& peers(void) const
    {
      return m_peers;
    }

    const std::vector<PeerConfig>& peerConfigs(void) const
    {
      return m_peer_configs;
    }

    const PeerConfig* findPeerConfig(
        const std::string& peer) const;

    bool validatePeerHello(
        const std::string& authenticated_callsign,
        const std::string& reflector_id,
        const std::string& domain,
        std::uint16_t major,
        std::uint16_t minor,
        std::uint32_t capabilities,
        std::string& peer,
        std::uint16_t& negotiated_minor,
        std::uint32_t& negotiated_capabilities,
        std::string& error) const;

    std::uint16_t validateIncomingStream(
        const std::string& peer,
        const std::string& origin_reflector_id,
        std::uint32_t tg,
        const std::string& codec,
        std::string& error) const;

    std::uint16_t startIncomingStream(
        const std::string& peer,
        const std::string& origin_reflector_id,
        std::uint32_t tg,
        std::uint64_t stream_id,
        const std::string& source_callsign,
        const std::string& codec,
        std::string& error);

    bool stopIncomingStream(
        const std::string& peer,
        const std::string& origin_reflector_id,
        std::uint32_t tg,
        std::uint64_t stream_id,
        std::string& error);

    bool acceptIncomingAudio(
        const std::string& peer,
        const std::string& origin_reflector_id,
        std::uint32_t tg,
        std::uint64_t stream_id,
        std::uint32_t sequence,
        std::string& error);

    std::size_t removeIncomingStreams(
        const std::string& peer);

    bool registerPeerSession(
        const std::string& peer,
        ReflectorClient* client,
        std::string& error);

    void unregisterPeerSession(
        ReflectorClient* client);

    ReflectorClient* peerSession(
        const std::string& peer) const;

    std::size_t peerSessionCount(void) const
    {
      return m_peer_sessions.size();
    }

    const IncomingStream* findIncomingStream(
        std::uint32_t tg) const;

    std::size_t incomingStreamCount(void) const
    {
      return m_incoming_streams.size();
    }

    const std::vector<TrustEntry>& trustEntries(void) const
    {
      return m_trust_entries;
    }

    const TrustEntry* findTrustByCallsign(
        const std::string& callsign) const;

    const TrustEntry* findTrustByPeer(
        const std::string& peer) const;

    std::uint64_t libraryGeneration(void) const
    {
      return m_library.generation();
    }

    std::size_t routeCount(void) const
    {
      return m_library.routeCount();
    }

    bool mayImport(const std::string& peer, std::uint32_t tg) const
    {
      return m_enabled && m_library.mayImport(peer, tg);
    }

    bool mayExport(const std::string& peer, std::uint32_t tg) const
    {
      return m_enabled && m_library.mayExport(peer, tg);
    }

    bool beginLocalStream(
        std::uint32_t tg,
        const std::string& source_callsign,
        const std::string& codec,
        std::uint64_t& stream_id,
        std::vector<std::string>& started_peers,
        std::string& error);

    std::size_t sendLocalStreamAudio(
        std::uint32_t tg,
        const std::vector<std::uint8_t>& audio_data);

    std::size_t endLocalStream(
        std::uint32_t tg,
        std::vector<std::string>& stopped_peers,
        std::string& error);

    const LocalStream* findLocalStream(
        std::uint32_t tg) const;

    std::size_t localStreamCount(void) const
    {
      return m_local_streams.size();
    }

    std::size_t startLocalStream(
        std::uint32_t tg,
        std::uint64_t stream_id,
        const std::string& source_callsign,
        const std::string& codec,
        std::vector<std::string>& started_peers,
        std::string& error);

    std::size_t sendLocalAudio(
        std::uint32_t tg,
        std::uint64_t stream_id,
        const std::vector<std::uint8_t>& audio_data,
        std::vector<std::string>& sent_peers,
        std::string& error);

    std::size_t stopLocalStream(
        std::uint32_t tg,
        std::uint64_t stream_id,
        std::vector<std::string>& stopped_peers,
        std::string& error);

    std::size_t outgoingConnectionCount(void) const
    {
      return m_peer_connections.size();
    }

    FederationPeerConnection* peerConnection(
        const std::string& peer) const;

    void startPeerConnections(void);
    void stopPeerConnections(void);

  private:
    bool                      m_enabled;
    std::string               m_domain;
    std::string               m_reflector_id;
    std::string               m_callsign;
    std::string               m_library_path;
    std::vector<std::string>  m_peers;
    std::vector<PeerConfig>   m_peer_configs;
    std::vector<TrustEntry>   m_trust_entries;
    std::vector<FederationPeerConnection*>  m_peer_connections;
    std::map<std::uint32_t, IncomingStream>  m_incoming_streams;
    std::map<std::uint32_t, LocalStream>     m_local_streams;
    std::uint64_t                            m_next_local_stream_id;
    std::map<std::string, ReflectorClient*>  m_peer_sessions;
    FederationLibrary         m_library;

    ReflectorFederation(const ReflectorFederation&);
    ReflectorFederation& operator=(const ReflectorFederation&);
};  /* class ReflectorFederation */


#endif /* REFLECTOR_FEDERATION_INCLUDED */