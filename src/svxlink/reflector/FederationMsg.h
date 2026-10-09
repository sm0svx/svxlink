/**
@file   FederationMsg.h
@brief  SVXReflector federation protocol messages
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

#ifndef FEDERATION_MSG_INCLUDED
#define FEDERATION_MSG_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <cstdint>
#include <string>
#include <vector>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "ReflectorMsg.h"


/****************************************************************************
 *
 * Protocol definitions
 *
 ****************************************************************************/

namespace FederationProtocol
{
  enum
  {
    VERSION_MAJOR = 1,
    VERSION_MINOR = 0,

        CAP_MULTIPLEXED_OPUS = 1U << 0,

    STREAM_ACCEPTED              = 0,
    STREAM_REJECT_POLICY         = 1,
    STREAM_REJECT_INVALID_ORIGIN = 2,
    STREAM_REJECT_DUPLICATE      = 3,
    STREAM_REJECT_CODEC          = 4,
    STREAM_REJECT_LOCAL_BUSY     = 5,
    STREAM_REJECT_PROTOCOL       = 6
  };
}


/****************************************************************************
 *
 * TCP message definitions
 *
 ****************************************************************************/

/**
@brief  Advertise an authenticated reflector's federation identity
*/
class MsgFederationHello : public ReflectorMsgBase<200>
{
  public:
    MsgFederationHello(void)
      : m_major(FederationProtocol::VERSION_MAJOR),
        m_minor(FederationProtocol::VERSION_MINOR),
        m_generation(0),
        m_capabilities(FederationProtocol::CAP_MULTIPLEXED_OPUS)
    {
    }

    MsgFederationHello(std::uint16_t major,
                       std::uint16_t minor,
                       const std::string& reflector_id,
                       const std::string& domain,
                       std::uint64_t generation,
                       std::uint32_t capabilities)
      : m_major(major),
        m_minor(minor),
        m_reflector_id(reflector_id),
        m_domain(domain),
        m_generation(generation),
        m_capabilities(capabilities)
    {
    }

    std::uint16_t major(void) const { return m_major; }
    std::uint16_t minor(void) const { return m_minor; }

    const std::string& reflectorId(void) const
    {
      return m_reflector_id;
    }

    const std::string& domain(void) const
    {
      return m_domain;
    }

    std::uint64_t generation(void) const
    {
      return m_generation;
    }

    std::uint32_t capabilities(void) const
    {
      return m_capabilities;
    }

    ASYNC_MSG_MEMBERS(m_major,
                      m_minor,
                      m_reflector_id,
                      m_domain,
                      m_generation,
                      m_capabilities)

  private:
    std::uint16_t m_major;
    std::uint16_t m_minor;
    std::string   m_reflector_id;
    std::string   m_domain;
    std::uint64_t m_generation;
    std::uint32_t m_capabilities;
}; /* MsgFederationHello */


/**
@brief  Accept a federation hello and report negotiated capabilities
*/
class MsgFederationHelloAck : public ReflectorMsgBase<201>
{
  public:
    MsgFederationHelloAck(void)
      : m_major(FederationProtocol::VERSION_MAJOR),
        m_minor(FederationProtocol::VERSION_MINOR),
        m_capabilities(FederationProtocol::CAP_MULTIPLEXED_OPUS)
    {
    }

    MsgFederationHelloAck(std::uint16_t major,
                          std::uint16_t minor,
                          const std::string& reflector_id,
                          const std::string& domain,
                          std::uint32_t capabilities)
      : m_major(major),
        m_minor(minor),
        m_reflector_id(reflector_id),
        m_domain(domain),
        m_capabilities(capabilities)
    {
    }

    std::uint16_t major(void) const { return m_major; }
    std::uint16_t minor(void) const { return m_minor; }

    const std::string& reflectorId(void) const
    {
      return m_reflector_id;
    }

    const std::string& domain(void) const
    {
      return m_domain;
    }

    std::uint32_t capabilities(void) const
    {
      return m_capabilities;
    }

    ASYNC_MSG_MEMBERS(m_major,
                      m_minor,
                      m_reflector_id,
                      m_domain,
                      m_capabilities)

  private:
    std::uint16_t m_major;
    std::uint16_t m_minor;
    std::string   m_reflector_id;
    std::string   m_domain;
    std::uint32_t m_capabilities;
}; /* MsgFederationHelloAck */

/**
@brief  Request permission to begin one federated talkgroup stream
*/
class MsgFederationStreamStart : public ReflectorMsgBase<202>
{
  public:
    MsgFederationStreamStart(void)
      : m_tg(0), m_stream_id(0)
    {
    }

    MsgFederationStreamStart(const std::string& origin_reflector_id,
                             std::uint32_t tg,
                             std::uint64_t stream_id,
                             const std::string& source_callsign,
                             const std::string& codec)
      : m_origin_reflector_id(origin_reflector_id),
        m_tg(tg),
        m_stream_id(stream_id),
        m_source_callsign(source_callsign),
        m_codec(codec)
    {
    }

    const std::string& originReflectorId(void) const
    {
      return m_origin_reflector_id;
    }

    std::uint32_t tg(void) const { return m_tg; }
    std::uint64_t streamId(void) const { return m_stream_id; }

    const std::string& sourceCallsign(void) const
    {
      return m_source_callsign;
    }

    const std::string& codec(void) const
    {
      return m_codec;
    }

    ASYNC_MSG_MEMBERS(m_origin_reflector_id,
                      m_tg,
                      m_stream_id,
                      m_source_callsign,
                      m_codec)

  private:
    std::string   m_origin_reflector_id;
    std::uint32_t m_tg;
    std::uint64_t m_stream_id;
    std::string   m_source_callsign;
    std::string   m_codec;
}; /* MsgFederationStreamStart */


/**
@brief  Accept or reject a federated talkgroup stream
*/
class MsgFederationStreamResult : public ReflectorMsgBase<203>
{
  public:
    MsgFederationStreamResult(void)
      : m_tg(0),
        m_stream_id(0),
        m_accepted(0),
        m_reason(FederationProtocol::STREAM_REJECT_PROTOCOL)
    {
    }

    MsgFederationStreamResult(
        const std::string& origin_reflector_id,
        std::uint32_t tg,
        std::uint64_t stream_id,
        bool accepted,
        std::uint16_t reason,
        const std::string& detail="")
      : m_origin_reflector_id(origin_reflector_id),
        m_tg(tg),
        m_stream_id(stream_id),
        m_accepted(accepted ? 1 : 0),
        m_reason(reason),
        m_detail(detail)
    {
    }

    const std::string& originReflectorId(void) const
    {
      return m_origin_reflector_id;
    }

    std::uint32_t tg(void) const { return m_tg; }
    std::uint64_t streamId(void) const { return m_stream_id; }
    bool accepted(void) const { return m_accepted != 0; }
    std::uint16_t reason(void) const { return m_reason; }

    const std::string& detail(void) const
    {
      return m_detail;
    }

    ASYNC_MSG_MEMBERS(m_origin_reflector_id,
                      m_tg,
                      m_stream_id,
                      m_accepted,
                      m_reason,
                      m_detail)

  private:
    std::string   m_origin_reflector_id;
    std::uint32_t m_tg;
    std::uint64_t m_stream_id;
    std::uint8_t  m_accepted;
    std::uint16_t m_reason;
    std::string   m_detail;
}; /* MsgFederationStreamResult */


/**
@brief  Close one federated talkgroup stream
*/
class MsgFederationStreamStop : public ReflectorMsgBase<204>
{
  public:
    MsgFederationStreamStop(void)
      : m_tg(0), m_stream_id(0)
    {
    }

    MsgFederationStreamStop(const std::string& origin_reflector_id,
                            std::uint32_t tg,
                            std::uint64_t stream_id)
      : m_origin_reflector_id(origin_reflector_id),
        m_tg(tg),
        m_stream_id(stream_id)
    {
    }

    const std::string& originReflectorId(void) const
    {
      return m_origin_reflector_id;
    }

    std::uint32_t tg(void) const { return m_tg; }
    std::uint64_t streamId(void) const { return m_stream_id; }

    ASYNC_MSG_MEMBERS(m_origin_reflector_id,
                      m_tg,
                      m_stream_id)

  private:
    std::string   m_origin_reflector_id;
    std::uint32_t m_tg;
    std::uint64_t m_stream_id;
}; /* MsgFederationStreamStop */

/****************************************************************************
 *
 * UDP message definitions
 *
 ****************************************************************************/

/**
@brief  Carry one Opus frame for one federated talkgroup stream
*/
class MsgUdpFederationAudio : public ReflectorUdpMsgBase<201>
{
  public:
    MsgUdpFederationAudio(void)
      : m_tg(0), m_stream_id(0), m_sequence(0)
    {
    }

    MsgUdpFederationAudio(
        const std::string& origin_reflector_id,
        std::uint32_t tg,
        std::uint64_t stream_id,
        std::uint32_t sequence,
        const std::vector<std::uint8_t>& audio_data)
      : m_origin_reflector_id(origin_reflector_id),
        m_tg(tg),
        m_stream_id(stream_id),
        m_sequence(sequence),
        m_audio_data(audio_data)
    {
    }

    const std::string& originReflectorId(void) const
    {
      return m_origin_reflector_id;
    }

    std::uint32_t tg(void) const { return m_tg; }
    std::uint64_t streamId(void) const { return m_stream_id; }
    std::uint32_t sequence(void) const { return m_sequence; }

    std::vector<std::uint8_t>& audioData(void)
    {
      return m_audio_data;
    }

    const std::vector<std::uint8_t>& audioData(void) const
    {
      return m_audio_data;
    }

    ASYNC_MSG_MEMBERS(m_origin_reflector_id,
                      m_tg,
                      m_stream_id,
                      m_sequence,
                      m_audio_data)

  private:
    std::string               m_origin_reflector_id;
    std::uint32_t             m_tg;
    std::uint64_t             m_stream_id;
    std::uint32_t             m_sequence;
    std::vector<std::uint8_t> m_audio_data;
}; /* MsgUdpFederationAudio */

#endif /* FEDERATION_MSG_INCLUDED */
