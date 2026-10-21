/**
@file   FederationLibrary.h
@brief  Hot-reloadable talkgroup library for reflector federation
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

#ifndef FEDERATION_LIBRARY_INCLUDED
#define FEDERATION_LIBRARY_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

/****************************************************************************
 *
 * Class definitions
 *
 ****************************************************************************/

/**
@brief  Load and validate a federation talkgroup library

A failed load leaves the previously accepted library metadata unchanged.
*/
class FederationLibrary
{
  public:
    struct Route
    {
      enum Type
      {
        TYPE_EXACT,
        TYPE_RANGE,
        TYPE_PREFIX
      };

      Type          type;
      std::uint32_t first;
      std::uint32_t last;
      std::string   prefix;
      std::string   home;
      std::string   scope;
      std::string   service_anchor;
      std::string   description;

      Route(void)
        : type(TYPE_EXACT), first(0), last(0)
      {
      }
    };

    struct TalkgroupPattern
    {
      enum Type
      {
        TYPE_EXACT,
        TYPE_PREFIX
      };

      Type          type;
      std::uint32_t exact;
      std::string   prefix;

      TalkgroupPattern(void)
        : type(TYPE_EXACT), exact(0)
      {
      }
    };

    struct PeerPolicy
    {
      std::string                   peer;
      std::vector<TalkgroupPattern> import_patterns;
      std::vector<TalkgroupPattern> export_patterns;
    };

    FederationLibrary(void);
    ~FederationLibrary(void);

    bool load(const std::string& path,
              const std::string& expected_domain,
              std::string& error);

    unsigned schema(void) const { return m_schema; }
    std::uint64_t generation(void) const { return m_generation; }
    const std::string& domain(void) const { return m_domain; }

    std::size_t routeCount(void) const
    {
      return m_routes.size();
    }

    const std::vector<Route>& routes(void) const
    {
      return m_routes;
    }

    const Route* findRoute(std::uint32_t tg) const;
    bool hasPeerPolicy(const std::string& peer) const;
    bool mayImport(const std::string& peer, std::uint32_t tg) const;
    bool mayExport(const std::string& peer, std::uint32_t tg) const;

    const std::vector<PeerPolicy>& peerPolicies(void) const
    {
      return m_peer_policies;
    }

  private:
    unsigned       m_schema;
    std::uint64_t  m_generation;
    std::string    m_domain;
    std::vector<Route>  m_routes;
    std::vector<PeerPolicy>  m_peer_policies;

    FederationLibrary(const FederationLibrary&);
    FederationLibrary& operator=(const FederationLibrary&);
};  /* class FederationLibrary */


#endif /* FEDERATION_LIBRARY_INCLUDED */