/**
@file   ReflectorFederation.cpp
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
/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <iostream>
#include <list>

/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "ReflectorFederation.h"
#include "FederationMsg.h"
#include "FederationPeerConnection.h"


/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/

ReflectorFederation::ReflectorFederation(void)
  : m_enabled(false),
    m_library_path("/etc/svxlink/federation.json"),
    m_next_local_stream_id(1)
{
} /* ReflectorFederation::ReflectorFederation */

ReflectorFederation::~ReflectorFederation(void)
{
  stopPeerConnections();

  for (std::vector<FederationPeerConnection*>::iterator
           it=m_peer_connections.begin();
       it!=m_peer_connections.end(); ++it)
  {
    delete *it;
  }

  m_peer_connections.clear();
} /* ReflectorFederation::~ReflectorFederation */


const ReflectorFederation::PeerConfig*
ReflectorFederation::findPeerConfig(
    const std::string& peer) const
{
  for (std::vector<PeerConfig>::const_iterator
           it=m_peer_configs.begin();
       it!=m_peer_configs.end(); ++it)
  {
    if (it->name == peer)
    {
      return &(*it);
    }
  }

  return 0;
} /* ReflectorFederation::findPeerConfig */


const ReflectorFederation::TrustEntry*
ReflectorFederation::findTrustByCallsign(
    const std::string& callsign) const
{
  for (std::vector<TrustEntry>::const_iterator
           it=m_trust_entries.begin();
       it!=m_trust_entries.end(); ++it)
  {
    if (it->callsign == callsign)
    {
      return &(*it);
    }
  }

  return 0;
} /* ReflectorFederation::findTrustByCallsign */


const ReflectorFederation::TrustEntry*
ReflectorFederation::findTrustByPeer(
    const std::string& peer) const
{
  for (std::vector<TrustEntry>::const_iterator
           it=m_trust_entries.begin();
       it!=m_trust_entries.end(); ++it)
  {
    if (it->peer == peer)
    {
      return &(*it);
    }
  }

  return 0;
} /* ReflectorFederation::findTrustByPeer */


bool ReflectorFederation::validatePeerHello(
    const std::string& authenticated_callsign,
    const std::string& reflector_id,
    const std::string& domain,
    std::uint16_t major,
    std::uint16_t minor,
    std::uint32_t capabilities,
    std::string& peer,
    std::uint16_t& negotiated_minor,
    std::uint32_t& negotiated_capabilities,
    std::string& error) const

{
  peer.clear();
  negotiated_minor = 0;
  negotiated_capabilities = 0;
  error.clear();

  if (!m_enabled)
  {
    error = "Federation is disabled";
    return false;
  }

  const TrustEntry* trust =
      findTrustByCallsign(authenticated_callsign);

  if (trust == 0)
  {
    error = "Authenticated callsign is not a trusted federation peer";
    return false;
  }

  const PeerConfig* peer_config =
      findPeerConfig(trust->peer);

  if (peer_config == 0)
  {
    error = "Trusted callsign refers to an unconfigured peer";
    return false;
  }

  if (reflector_id != peer_config->reflector_id)
  {
    error = "Federation hello reflector identity does not match peer";
    return false;
  }

  if (domain != peer_config->name)
  {
    error = "Federation hello domain does not match peer";
    return false;
  }

  if (major != FederationProtocol::VERSION_MAJOR)
  {
    error = "Unsupported federation protocol major version";
    return false;
  }

  const std::uint32_t local_capabilities =
      FederationProtocol::CAP_MULTIPLEXED_OPUS;

  const std::uint32_t common_capabilities =
      capabilities & local_capabilities;

  if ((common_capabilities &
       FederationProtocol::CAP_MULTIPLEXED_OPUS) == 0)
  {
    error = "Peer does not support multiplexed Opus federation";
    return false;
  }

  peer = peer_config->name;
  negotiated_minor =
      (minor < FederationProtocol::VERSION_MINOR)
      ? minor
      : FederationProtocol::VERSION_MINOR;
  negotiated_capabilities = common_capabilities;

  return true;
} /* ReflectorFederation::validatePeerHello */


std::uint16_t ReflectorFederation::validateIncomingStream(
    const std::string& peer,
    const std::string& origin_reflector_id,
    std::uint32_t tg,
    const std::string& codec,
    std::string& error) const
{
  error.clear();

  if (!m_enabled)
  {
    error = "Federation is disabled";
    return FederationProtocol::STREAM_REJECT_PROTOCOL;
  }

  const PeerConfig* peer_config = findPeerConfig(peer);
  if (peer_config == 0)
  {
    error = "Stream received from an unconfigured peer";
    return FederationProtocol::STREAM_REJECT_PROTOCOL;
  }

  if (origin_reflector_id.empty() ||
      (origin_reflector_id == m_reflector_id) ||
      (origin_reflector_id != peer_config->reflector_id))
  {
    error = "Stream origin does not match the trusted peer";
    return FederationProtocol::STREAM_REJECT_INVALID_ORIGIN;
  }

  if (tg == 0)
  {
    error = "Talkgroup zero is not a valid federation stream";
    return FederationProtocol::STREAM_REJECT_PROTOCOL;
  }

  if (codec != "OPUS")
  {
    error = "Only OPUS federation streams are supported";
    return FederationProtocol::STREAM_REJECT_CODEC;
  }

  const FederationLibrary::Route* route =
      m_library.findRoute(tg);

  if ((route == 0) ||
      (route->scope != "family"))
  {
    error = "Talkgroup is not configured as a family federation route";
    return FederationProtocol::STREAM_REJECT_POLICY;
  }

  if (!m_library.mayImport(peer, tg))
  {
    error = "Talkgroup is denied by local import policy";
    return FederationProtocol::STREAM_REJECT_POLICY;
  }

  return FederationProtocol::STREAM_ACCEPTED;
} /* ReflectorFederation::validateIncomingStream */

const ReflectorFederation::IncomingStream*
ReflectorFederation::findIncomingStream(std::uint32_t tg) const
{
  std::map<std::uint32_t, IncomingStream>::const_iterator it =
      m_incoming_streams.find(tg);

  return (it == m_incoming_streams.end())
      ? 0
      : &it->second;
} /* ReflectorFederation::findIncomingStream */


std::uint16_t ReflectorFederation::startIncomingStream(
    const std::string& peer,
    const std::string& origin_reflector_id,
    std::uint32_t tg,
    std::uint64_t stream_id,
    const std::string& source_callsign,
    const std::string& codec,
    std::string& error)
{
  const std::uint16_t validation =
      validateIncomingStream(
          peer, origin_reflector_id, tg, codec, error);

  if (validation != FederationProtocol::STREAM_ACCEPTED)
  {
    return validation;
  }

  if (stream_id == 0)
  {
    error = "Stream ID zero is invalid";
    return FederationProtocol::STREAM_REJECT_PROTOCOL;
  }

  if (source_callsign.empty())
  {
    error = "Stream source callsign is missing";
    return FederationProtocol::STREAM_REJECT_PROTOCOL;
  }

  std::map<std::uint32_t, IncomingStream>::const_iterator existing =
      m_incoming_streams.find(tg);

  if (existing != m_incoming_streams.end())
  {
    if ((existing->second.peer == peer) &&
        (existing->second.origin_reflector_id ==
         origin_reflector_id) &&
        (existing->second.stream_id == stream_id))
    {
      error = "Federation stream is already active";
      return FederationProtocol::STREAM_REJECT_DUPLICATE;
    }

    error = "Talkgroup already has an active federation stream";
    return FederationProtocol::STREAM_REJECT_LOCAL_BUSY;
  }

  IncomingStream stream;
  stream.peer = peer;
  stream.origin_reflector_id = origin_reflector_id;
  stream.tg = tg;
  stream.stream_id = stream_id;
  stream.source_callsign = source_callsign;
  stream.codec = codec;

  m_incoming_streams[tg] = stream;
  error.clear();

  incomingStreamStarted(
      tg,
      source_callsign);

  return FederationProtocol::STREAM_ACCEPTED;
} /* ReflectorFederation::startIncomingStream */


bool ReflectorFederation::stopIncomingStream(
    const std::string& peer,
    const std::string& origin_reflector_id,
    std::uint32_t tg,
    std::uint64_t stream_id,
    std::string& error)
{
  error.clear();

  std::map<std::uint32_t, IncomingStream>::iterator it =
      m_incoming_streams.find(tg);

  if (it == m_incoming_streams.end())
  {
    error = "Federation stream is not active";
    return false;
  }

  if ((it->second.peer != peer) ||
      (it->second.origin_reflector_id != origin_reflector_id) ||
      (it->second.stream_id != stream_id))
  {
    error = "Federation stream identity does not match active stream";
    return false;
  }

  const std::string source_callsign(
      it->second.source_callsign);

  m_incoming_streams.erase(it);

  incomingStreamStopped(
      tg,
      source_callsign);

  return true;
} /* ReflectorFederation::stopIncomingStream */


bool ReflectorFederation::acceptIncomingAudio(
    const std::string& peer,
    const std::string& origin_reflector_id,
    std::uint32_t tg,
    std::uint64_t stream_id,
    std::uint32_t sequence,
    std::string& error)
{
  error.clear();

  std::map<std::uint32_t, IncomingStream>::iterator it =
      m_incoming_streams.find(tg);

  if (it == m_incoming_streams.end())
  {
    error = "Federation audio has no active stream";
    return false;
  }

  IncomingStream& stream = it->second;

  if ((stream.peer != peer) ||
      (stream.origin_reflector_id != origin_reflector_id) ||
      (stream.stream_id != stream_id))
  {
    error = "Federation audio identity does not match active stream";
    return false;
  }

  if (stream.sequence_seen)
  {
    const std::uint32_t difference =
        sequence - stream.last_sequence;

    if ((difference == 0) || (difference > 0x7fffffffU))
    {
      error = "Federation audio sequence is duplicate or out of order";
      return false;
    }
  }

  stream.last_sequence = sequence;
  stream.sequence_seen = true;
  return true;
} /* ReflectorFederation::acceptIncomingAudio */


ReflectorClient* ReflectorFederation::peerSession(
    const std::string& peer) const
{
  std::map<std::string, ReflectorClient*>::const_iterator it =
      m_peer_sessions.find(peer);

  return (it == m_peer_sessions.end())
      ? 0
      : it->second;
} /* ReflectorFederation::peerSession */


bool ReflectorFederation::registerPeerSession(
    const std::string& peer,
    ReflectorClient* client,
    std::string& error)
{
  error.clear();

  if (!m_enabled)
  {
    error = "Federation is disabled";
    return false;
  }

  if (client == 0)
  {
    error = "Cannot register a null federation session";
    return false;
  }

  if (findPeerConfig(peer) == 0)
  {
    error = "Cannot register an unconfigured federation peer";
    return false;
  }

  if (m_peer_sessions.find(peer) != m_peer_sessions.end())
  {
    error = "Federation peer already has an active session";
    return false;
  }

  for (std::map<std::string, ReflectorClient*>::const_iterator
           it=m_peer_sessions.begin();
       it!=m_peer_sessions.end(); ++it)
  {
    if (it->second == client)
    {
      error = "Federation session is already registered to another peer";
      return false;
    }
  }

  m_peer_sessions[peer] = client;
  return true;
} /* ReflectorFederation::registerPeerSession */


void ReflectorFederation::unregisterPeerSession(
    ReflectorClient* client)
{
  if (client == 0)
  {
    return;
  }

  std::map<std::string, ReflectorClient*>::iterator it =
      m_peer_sessions.begin();

  while (it != m_peer_sessions.end())
  {
    if (it->second == client)
    {
      const std::string peer(it->first);
      it = m_peer_sessions.erase(it);
      removeIncomingStreams(peer);
    }
    else
    {
      ++it;
    }
  }
} /* ReflectorFederation::unregisterPeerSession */


std::size_t ReflectorFederation::removeIncomingStreams(
    const std::string& peer)
{
  std::size_t removed = 0;

  std::map<std::uint32_t, IncomingStream>::iterator it =
      m_incoming_streams.begin();

  while (it != m_incoming_streams.end())
  {
    if (it->second.peer == peer)
    {
      const std::uint32_t tg = it->second.tg;
      const std::string source_callsign(
          it->second.source_callsign);

      it = m_incoming_streams.erase(it);
      ++removed;

      incomingStreamStopped(
          tg,
          source_callsign);
    }
    else
    {
      ++it;
    }
  }

  return removed;
} /* ReflectorFederation::removeIncomingStreams */


const ReflectorFederation::LocalStream*
ReflectorFederation::findLocalStream(
    std::uint32_t tg) const
{
  std::map<std::uint32_t, LocalStream>::const_iterator it =
      m_local_streams.find(tg);

  return (it == m_local_streams.end())
      ? 0
      : &it->second;
} /* ReflectorFederation::findLocalStream */


bool ReflectorFederation::beginLocalStream(
    std::uint32_t tg,
    const std::string& source_callsign,
    const std::string& codec,
    std::uint64_t& stream_id,
    std::vector<std::string>& started_peers,
    std::string& error)
{
  stream_id = 0;
  started_peers.clear();
  error.clear();

  if (!m_enabled)
  {
    error = "Federation is disabled";
    return false;
  }

  if (tg == 0)
  {
    error = "Talkgroup zero is not a valid federation stream";
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

  if (m_local_streams.find(tg) != m_local_streams.end())
  {
    error = "Talkgroup already has a local federation stream";
    return false;
  }

  stream_id = m_next_local_stream_id++;

  if (stream_id == 0)
  {
    stream_id = m_next_local_stream_id++;
  }

  if (m_next_local_stream_id == 0)
  {
    m_next_local_stream_id = 1;
  }

  LocalStream stream;
  stream.tg = tg;
  stream.stream_id = stream_id;
  stream.source_callsign = source_callsign;
  stream.codec = codec;

  const std::size_t started =
      startLocalStream(
          tg,
          stream_id,
          source_callsign,
          codec,
          started_peers,
          error);

  stream.export_requested = (started > 0);
  m_local_streams[tg] = stream;

  return true;
} /* ReflectorFederation::beginLocalStream */


std::size_t ReflectorFederation::sendLocalStreamAudio(
    std::uint32_t tg,
    const std::vector<std::uint8_t>& audio_data)
{
  if (!m_enabled || audio_data.empty())
  {
    return 0;
  }

  std::map<std::uint32_t, LocalStream>::const_iterator local_it =
      m_local_streams.find(tg);

  if ((local_it == m_local_streams.end()) ||
      !local_it->second.export_requested)
  {
    return 0;
  }

  std::size_t handled = 0;
  std::string peer_error;

  for (std::vector<FederationPeerConnection*>::iterator
           it=m_peer_connections.begin();
       it!=m_peer_connections.end(); ++it)
  {
    FederationPeerConnection* connection = *it;

    if ((connection == 0) ||
        !mayExport(connection->peer(), tg))
    {
      continue;
    }

    const FederationPeerConnection::OutgoingStream* stream =
        connection->findOutgoingStream(tg);

    if ((stream == 0) ||
        (stream->stream_id != local_it->second.stream_id))
    {
      continue;
    }

    if (connection->sendOutgoingAudio(
            tg,
            local_it->second.stream_id,
            audio_data,
            peer_error))
    {
      ++handled;
    }
  }

  return handled;
} /* ReflectorFederation::sendLocalStreamAudio */


std::size_t ReflectorFederation::endLocalStream(
    std::uint32_t tg,
    std::vector<std::string>& stopped_peers,
    std::string& error)
{
  stopped_peers.clear();
  error.clear();

  std::map<std::uint32_t, LocalStream>::iterator it =
      m_local_streams.find(tg);

  if (it == m_local_streams.end())
  {
    error = "Talkgroup does not have a local federation stream";
    return 0;
  }

  const std::uint64_t stream_id = it->second.stream_id;
  const bool export_requested = it->second.export_requested;

  if (!export_requested)
  {
    m_local_streams.erase(it);
    return 0;
  }

  const std::size_t stopped =
      stopLocalStream(
          tg,
          stream_id,
          stopped_peers,
          error);

  m_local_streams.erase(it);
  return stopped;
} /* ReflectorFederation::endLocalStream */


std::size_t ReflectorFederation::startLocalStream(
    std::uint32_t tg,
    std::uint64_t stream_id,
    const std::string& source_callsign,
    const std::string& codec,
    std::vector<std::string>& started_peers,
    std::string& error)
{
  started_peers.clear();
  error.clear();

  if (!m_enabled)
  {
    error = "Federation is disabled";
    return 0;
  }

  if (tg == 0)
  {
    error = "Talkgroup zero is not a valid federation stream";
    return 0;
  }

  if (stream_id == 0)
  {
    error = "Stream ID zero is invalid";
    return 0;
  }

  if (source_callsign.empty())
  {
    error = "Stream source callsign is missing";
    return 0;
  }

  if (codec != "OPUS")
  {
    error = "Only OPUS federation streams are supported";
    return 0;
  }

  std::size_t policy_matches = 0;
  std::size_t ready_matches = 0;
  std::string last_error;

  for (std::vector<FederationPeerConnection*>::iterator
           it=m_peer_connections.begin();
       it!=m_peer_connections.end(); ++it)
  {
    FederationPeerConnection* connection = *it;

    if ((connection == 0) ||
        !mayExport(connection->peer(), tg))
    {
      continue;
    }

    ++policy_matches;

    if (!connection->isConnected() ||
        !connection->isUdpRegistered())
    {
      continue;
    }

    ++ready_matches;

    std::string peer_error;
    if (connection->startOutgoingStream(
            tg,
            stream_id,
            source_callsign,
            codec,
            peer_error))
    {
      started_peers.push_back(connection->peer());
    }
    else
    {
      last_error = peer_error;

      std::cerr << "*** WARNING: Federation peer "
                << connection->peer()
                << ": Could not start local stream:"
                << " tg=" << tg
                << " stream_id=" << stream_id
                << " detail=" << peer_error
                << std::endl;
    }
  }

  if (!started_peers.empty())
  {
    return started_peers.size();
  }

  if (policy_matches == 0)
  {
    error = "Talkgroup is denied by federation export policy";
  }
  else if (ready_matches == 0)
  {
    error = "No permitted federation peer is connected and UDP registered";
  }
  else if (!last_error.empty())
  {
    error = last_error;
  }
  else
  {
    error = "No federation peer accepted the local stream request";
  }

  return 0;
} /* ReflectorFederation::startLocalStream */


std::size_t ReflectorFederation::sendLocalAudio(
    std::uint32_t tg,
    std::uint64_t stream_id,
    const std::vector<std::uint8_t>& audio_data,
    std::vector<std::string>& sent_peers,
    std::string& error)
{
  sent_peers.clear();
  error.clear();

  if (!m_enabled)
  {
    error = "Federation is disabled";
    return 0;
  }

  if (tg == 0)
  {
    error = "Talkgroup zero is not a valid federation stream";
    return 0;
  }

  if (stream_id == 0)
  {
    error = "Stream ID zero is invalid";
    return 0;
  }

  if (audio_data.empty())
  {
    error = "Federation audio data is empty";
    return 0;
  }

  std::size_t matching_streams = 0;
  std::size_t sendable_streams = 0;
  std::size_t permitted_streams = 0;
  std::string last_error;

  for (std::vector<FederationPeerConnection*>::iterator
           it=m_peer_connections.begin();
       it!=m_peer_connections.end(); ++it)
  {
    FederationPeerConnection* connection = *it;
    if (connection == 0)
    {
      continue;
    }

    const FederationPeerConnection::OutgoingStream* stream =
        connection->findOutgoingStream(tg);

    if ((stream == 0) ||
        (stream->stream_id != stream_id))
    {
      continue;
    }

    ++matching_streams;

    if ((stream->state !=
         FederationPeerConnection::OUTGOING_STREAM_PENDING) &&
        (stream->state !=
         FederationPeerConnection::OUTGOING_STREAM_ACTIVE))
    {
      continue;
    }

    ++sendable_streams;

    if (!mayExport(connection->peer(), tg))
    {
      continue;
    }

    ++permitted_streams;

    std::string peer_error;
    if (connection->sendOutgoingAudio(
            tg,
            stream_id,
            audio_data,
            peer_error))
    {
      sent_peers.push_back(connection->peer());
    }
    else
    {
      last_error = peer_error;

      std::cerr << "*** WARNING: Federation peer "
                << connection->peer()
                << ": Could not send local audio:"
                << " tg=" << tg
                << " stream_id=" << stream_id
                << " detail=" << peer_error
                << std::endl;
    }
  }

  if (!sent_peers.empty())
  {
    return sent_peers.size();
  }

  if (matching_streams == 0)
  {
    error = "No outgoing federation stream matches the local stream";
  }
  else if (sendable_streams == 0)
  {
    error = "No matching outgoing federation stream can accept audio";
  }
  else if (permitted_streams == 0)
  {
    error = "No matching outgoing stream is permitted by export policy";
  }
  else if (!last_error.empty())
  {
    error = last_error;
  }
  else
  {
    error = "No federation peer accepted the local audio";
  }

  return 0;
} /* ReflectorFederation::sendLocalAudio */


std::size_t ReflectorFederation::stopLocalStream(
    std::uint32_t tg,
    std::uint64_t stream_id,
    std::vector<std::string>& stopped_peers,
    std::string& error)
{
  stopped_peers.clear();
  error.clear();

  if (!m_enabled)
  {
    error = "Federation is disabled";
    return 0;
  }

  if (tg == 0)
  {
    error = "Talkgroup zero is not a valid federation stream";
    return 0;
  }

  if (stream_id == 0)
  {
    error = "Stream ID zero is invalid";
    return 0;
  }

  std::size_t matching_streams = 0;
  std::string last_error;

  for (std::vector<FederationPeerConnection*>::iterator
           it=m_peer_connections.begin();
       it!=m_peer_connections.end(); ++it)
  {
    FederationPeerConnection* connection = *it;
    if (connection == 0)
    {
      continue;
    }

    const FederationPeerConnection::OutgoingStream* stream =
        connection->findOutgoingStream(tg);

    if ((stream == 0) ||
        (stream->stream_id != stream_id))
    {
      continue;
    }

    ++matching_streams;

    std::string peer_error;
    if (connection->stopOutgoingStream(
            tg,
            stream_id,
            peer_error))
    {
      stopped_peers.push_back(connection->peer());
    }
    else
    {
      last_error = peer_error;

      std::cerr << "*** WARNING: Federation peer "
                << connection->peer()
                << ": Could not stop local stream:"
                << " tg=" << tg
                << " stream_id=" << stream_id
                << " detail=" << peer_error
                << std::endl;
    }
  }

  if (!stopped_peers.empty())
  {
    return stopped_peers.size();
  }

  if (matching_streams == 0)
  {
    error = "No outgoing federation stream matches the local stream";
  }
  else if (!last_error.empty())
  {
    error = last_error;
  }
  else
  {
    error = "No federation peer stopped the local stream";
  }

  return 0;
} /* ReflectorFederation::stopLocalStream */


FederationPeerConnection* ReflectorFederation::peerConnection(
    const std::string& peer) const
{
  for (std::vector<FederationPeerConnection*>::const_iterator
           it=m_peer_connections.begin();
       it!=m_peer_connections.end(); ++it)
  {
    if (((*it) != 0) && ((*it)->peer() == peer))
    {
      return *it;
    }
  }

  return 0;
} /* ReflectorFederation::peerConnection */


void ReflectorFederation::startPeerConnections(void)
{
  if (!m_enabled)
  {
    return;
  }

  for (std::vector<FederationPeerConnection*>::iterator
           it=m_peer_connections.begin();
       it!=m_peer_connections.end(); ++it)
  {
    (*it)->start();
  }
} /* ReflectorFederation::startPeerConnections */


void ReflectorFederation::stopPeerConnections(void)
{
  for (std::vector<FederationPeerConnection*>::iterator
           it=m_peer_connections.begin();
       it!=m_peer_connections.end(); ++it)
  {
    (*it)->stop();
  }
} /* ReflectorFederation::stopPeerConnections */


bool ReflectorFederation::initialize(Async::Config& cfg)
{
  cfg.getValue("FEDERATION", "ENABLE", m_enabled);

  if (!m_enabled)
  {
    return true;
  }

  bool config_ok = true;

  if (!cfg.getValue("FEDERATION", "DOMAIN", m_domain) || m_domain.empty())
  {
    std::cerr << "*** ERROR: FEDERATION/DOMAIN is missing or empty"
              << std::endl;
    config_ok = false;
  }

  if (!cfg.getValue("FEDERATION", "REFLECTOR_ID", m_reflector_id) ||
      m_reflector_id.empty())
  {
    std::cerr << "*** ERROR: FEDERATION/REFLECTOR_ID is missing or empty"
              << std::endl;
    config_ok = false;
  }

  if (!cfg.getValue("FEDERATION", "CALLSIGN", m_callsign) ||
      m_callsign.empty())
  {
    std::cerr << "*** ERROR: FEDERATION/CALLSIGN is missing or empty"
              << std::endl;
    config_ok = false;
  }

  cfg.getValue("FEDERATION", "LIBRARY", m_library_path);
  cfg.getValue("FEDERATION", "PEERS", m_peers);

  if (!config_ok)
  {
    return false;
  }

  std::string library_error;
  if (!m_library.load(m_library_path, m_domain, library_error))
  {
    std::cerr << "*** ERROR: " << library_error << std::endl;
    return false;
  }

  std::vector<PeerConfig> candidate_peer_configs;

  for (std::vector<std::string>::const_iterator it=m_peers.begin();
       it!=m_peers.end(); ++it)
  {
    if (it->empty())
    {
      std::cerr << "*** ERROR: FEDERATION/PEERS contains an empty peer"
                << std::endl;
      return false;
    }

    if (!m_library.hasPeerPolicy(*it))
    {
      std::cerr << "*** ERROR: Federation peer " << *it
                << " has no entry in the library peer_policy"
                << std::endl;
      return false;
    }

    const std::string section("FEDERATION_PEER_" + *it);
    PeerConfig peer;
    peer.name = *it;

    if (!cfg.getValue(section, "HOST", peer.host) ||
        peer.host.empty())
    {
      std::cerr << "*** ERROR: " << section
                << "/HOST is missing or empty"
                << std::endl;
      return false;
    }

    // The stable federation identity defaults to the current endpoint.
    peer.reflector_id = peer.host;
    cfg.getValue(section, "REFLECTOR_ID", peer.reflector_id);

    if (peer.reflector_id.empty())
    {
      std::cerr << "*** ERROR: " << section
                << "/REFLECTOR_ID must not be empty"
                << std::endl;
      return false;
    }

    unsigned port = peer.port;
    cfg.getValue(section, "PORT", port);

    if ((port == 0) || (port > 65535))
    {
      std::cerr << "*** ERROR: " << section
                << "/PORT must be between 1 and 65535"
                << std::endl;
      return false;
    }
    peer.port = static_cast<std::uint16_t>(port);

    cfg.getValue(section, "PROTOCOL", peer.protocol);
    if (peer.protocol != 2)
    {
      std::cerr << "*** ERROR: " << section
                << "/PROTOCOL must currently be 2"
                << std::endl;
      return false;
    }

    if (!cfg.getValue(section, "AUTH_KEY", peer.auth_key) ||
        peer.auth_key.empty())
    {
      std::cerr << "*** ERROR: " << section
                << "/AUTH_KEY is missing or empty"
                << std::endl;
      return false;
    }

    cfg.getValue(section, "CONNECT", peer.connect);
    candidate_peer_configs.push_back(peer);
  }
  std::vector<TrustEntry> candidate_trust_entries;
  const std::list<std::string> trust_callsigns =
      cfg.listSection("FEDERATION_TRUST");

  for (std::list<std::string>::const_iterator
           it=trust_callsigns.begin();
       it!=trust_callsigns.end(); ++it)
  {
    TrustEntry trust;
    trust.callsign = *it;

    if (trust.callsign.empty())
    {
      std::cerr << "*** ERROR: FEDERATION_TRUST contains an empty callsign"
                << std::endl;
      return false;
    }

    for (std::string::const_iterator ch=trust.callsign.begin();
         ch!=trust.callsign.end(); ++ch)
    {
      if ((*ch >= 'a') && (*ch <= 'z'))
      {
        std::cerr << "*** ERROR: Federation trust callsign "
                  << trust.callsign
                  << " must use upper case"
                  << std::endl;
        return false;
      }
    }

    if (trust.callsign == m_callsign)
    {
      std::cerr << "*** ERROR: Federation trust callsign "
                << trust.callsign
                << " is the local federation callsign"
                << std::endl;
      return false;
    }

    if (!cfg.getValue("FEDERATION_TRUST",
                      trust.callsign,
                      trust.peer) ||
        trust.peer.empty())
    {
      std::cerr << "*** ERROR: FEDERATION_TRUST/"
                << trust.callsign
                << " is missing or empty"
                << std::endl;
      return false;
    }

    bool peer_exists = false;
    for (std::vector<PeerConfig>::const_iterator
             peer_it=candidate_peer_configs.begin();
         peer_it!=candidate_peer_configs.end(); ++peer_it)
    {
      if (peer_it->name == trust.peer)
      {
        peer_exists = true;
        break;
      }
    }

    if (!peer_exists)
    {
      std::cerr << "*** ERROR: Federation trust callsign "
                << trust.callsign
                << " refers to unknown peer "
                << trust.peer
                << std::endl;
      return false;
    }

    for (std::vector<TrustEntry>::const_iterator
             trust_it=candidate_trust_entries.begin();
         trust_it!=candidate_trust_entries.end(); ++trust_it)
    {
      if (trust_it->peer == trust.peer)
      {
        std::cerr << "*** ERROR: Federation peer "
                  << trust.peer
                  << " has more than one trusted callsign"
                  << std::endl;
        return false;
      }
    }

    candidate_trust_entries.push_back(trust);
  }

  for (std::vector<PeerConfig>::const_iterator
           peer_it=candidate_peer_configs.begin();
       peer_it!=candidate_peer_configs.end(); ++peer_it)
  {
    bool trust_exists = false;

    for (std::vector<TrustEntry>::const_iterator
             trust_it=candidate_trust_entries.begin();
         trust_it!=candidate_trust_entries.end(); ++trust_it)
    {
      if (trust_it->peer == peer_it->name)
      {
        trust_exists = true;
        break;
      }
    }

    if (!trust_exists)
    {
      std::cerr << "*** ERROR: Federation peer "
                << peer_it->name
                << " has no FEDERATION_TRUST callsign"
                << std::endl;
      return false;
    }
  }

  m_peer_configs.swap(candidate_peer_configs);
  m_trust_entries.swap(candidate_trust_entries);

  std::vector<FederationPeerConnection*> candidate_connections;

  for (std::vector<PeerConfig>::const_iterator
           it=m_peer_configs.begin();
       it!=m_peer_configs.end(); ++it)
  {
    if (!it->connect)
    {
      continue;
    }

    candidate_connections.push_back(
        new FederationPeerConnection(
            it->name,
            it->reflector_id,
            m_reflector_id,
            m_domain,
            m_library.generation(),
            m_callsign,
            it->host,
            it->port,
            it->auth_key));
  }

  for (std::vector<FederationPeerConnection*>::iterator
           it=m_peer_connections.begin();
       it!=m_peer_connections.end(); ++it)
  {
    delete *it;
  }

  m_peer_connections.clear();

  m_peer_connections.swap(candidate_connections);

  std::cout << "Reflector federation enabled:"
            << " domain=" << m_domain
            << " reflector_id=" << m_reflector_id
            << " callsign=" << m_callsign
            << " peers=" << m_peers.size()
            << " library=" << m_library_path
            << " generation=" << m_library.generation()
            << " routes=" << m_library.routeCount()
            << " outgoing_connections="
            << m_peer_connections.size()
            << std::endl;

  for (std::vector<PeerConfig>::const_iterator
          it=m_peer_configs.begin();
      it!=m_peer_configs.end(); ++it)
  {
    std::cout << "  Federation peer:"
              << " name=" << it->name
              << " reflector_id=" << it->reflector_id
              << " host=" << it->host
              << " port=" << it->port
              << " protocol=" << it->protocol
              << " connect=" << (it->connect ? "yes" : "no")
              << std::endl;
  }

  for (std::vector<TrustEntry>::const_iterator
           it=m_trust_entries.begin();
       it!=m_trust_entries.end(); ++it)
  {
    std::cout << "  Federation trust:"
              << " callsign=" << it->callsign
              << " peer=" << it->peer
              << std::endl;
  }

  return true;
} /* ReflectorFederation::initialize */
