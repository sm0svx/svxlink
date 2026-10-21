/**
@file   FederationLibrary.cpp
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


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <cerrno>
#include <cstdlib>
#include <fstream>
#include <limits>
#include <sstream>
#include <json/json.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "FederationLibrary.h"

/****************************************************************************
 *
 * Local functions
 *
 ****************************************************************************/

namespace {
  bool parsePositiveUint32(const std::string& text, std::uint32_t& value)
  {
    if (text.empty())
    {
      return false;
    }

    for (std::string::const_iterator it=text.begin(); it!=text.end(); ++it)
    {
      if ((*it < '0') || (*it > '9'))
      {
        return false;
      }
    }

    errno = 0;
    char *end = 0;
    const unsigned long long parsed = std::strtoull(text.c_str(), &end, 10);

    if ((errno != 0) || (end == 0) || (*end != '\0') ||
        (parsed == 0) ||
        (parsed > std::numeric_limits<std::uint32_t>::max()))
    {
      return false;
    }

    value = static_cast<std::uint32_t>(parsed);
    return true;
  }


  bool validateRoute(const Json::Value& route,
                     Json::Value::ArrayIndex index,
                     FederationLibrary::Route& parsed_route,
                     std::string& error)
  {
    std::ostringstream prefix;
    prefix << "Federation route " << index << ": ";

    if (!route.isObject())
    {
      error = prefix.str() + "entry must be a JSON object";
      return false;
    }

    if (!route["type"].isString())
    {
      error = prefix.str() + "type is missing or invalid";
      return false;
    }

    const std::string type(route["type"].asString());

    if (type == "exact")
    {
      if (!route["value"].isUInt() || (route["value"].asUInt() == 0))
      {
        error = prefix.str() +
                "exact value must be a positive talkgroup number";
        return false;
      }

      parsed_route.type = FederationLibrary::Route::TYPE_EXACT;
      parsed_route.first = route["value"].asUInt();
      parsed_route.last = parsed_route.first;
    }
    else if (type == "prefix")
    {
      std::uint32_t value = 0;
      if (!route["value"].isString() ||
          !parsePositiveUint32(route["value"].asString(), value))
      {
        error = prefix.str() +
                "prefix value must be a string containing digits";
        return false;
      }

      parsed_route.type = FederationLibrary::Route::TYPE_PREFIX;
      parsed_route.prefix = route["value"].asString();
    }
    else if (type == "range")
    {
      if (!route["value"].isString())
      {
        error = prefix.str() +
                "range value must use the form \"first-last\"";
        return false;
      }

      const std::string range(route["value"].asString());
      const std::string::size_type separator = range.find('-');

      if ((separator == std::string::npos) ||
          (range.find('-', separator + 1) != std::string::npos))
      {
        error = prefix.str() +
                "range value must use the form \"first-last\"";
        return false;
      }

      std::uint32_t first = 0;
      std::uint32_t last = 0;

      if (!parsePositiveUint32(range.substr(0, separator), first) ||
          !parsePositiveUint32(range.substr(separator + 1), last) ||
          (first > last))
      {
        error = prefix.str() + "range value is invalid";
        return false;
      }

      parsed_route.type = FederationLibrary::Route::TYPE_RANGE;
      parsed_route.first = first;
      parsed_route.last = last;
    }
    else
    {
      error = prefix.str() + "unknown route type \"" + type + "\"";
      return false;
    }

    if (!route["home"].isString() || route["home"].asString().empty())
    {
      error = prefix.str() + "home is missing or empty";
      return false;
    }
    parsed_route.home = route["home"].asString();

    if (!route["scope"].isString() || route["scope"].asString().empty())
    {
      error = prefix.str() + "scope is missing or empty";
      return false;
    }
    parsed_route.scope = route["scope"].asString();

    if (route.isMember("service_anchor"))
    {
      if (!route["service_anchor"].isString() ||
          route["service_anchor"].asString().empty())
      {
        error = prefix.str() + "service_anchor must be a non-empty string";
        return false;
      }
      parsed_route.service_anchor = route["service_anchor"].asString();
    }

    if (route.isMember("description"))
    {
      if (!route["description"].isString())
      {
        error = prefix.str() + "description must be a string";
        return false;
      }
      parsed_route.description = route["description"].asString();
    }

    return true;
  }

  bool parseTalkgroupPattern(
      const Json::Value& value,
      FederationLibrary::TalkgroupPattern& pattern,
      std::string& error)
  {
    if (!value.isString())
    {
      error = "talkgroup pattern must be a string";
      return false;
    }

    const std::string text(value.asString());
    if (text.empty())
    {
      error = "talkgroup pattern must not be empty";
      return false;
    }

    const std::string::size_type wildcard = text.find('*');

    if (wildcard == std::string::npos)
    {
      std::uint32_t exact = 0;
      if (!parsePositiveUint32(text, exact))
      {
        error = "exact talkgroup pattern must contain only digits";
        return false;
      }

      pattern.type =
          FederationLibrary::TalkgroupPattern::TYPE_EXACT;
      pattern.exact = exact;
      return true;
    }

    if ((wildcard != (text.size() - 1)) ||
        (text.find('*', wildcard + 1) != std::string::npos))
    {
      error = "talkgroup wildcard must appear once at the end";
      return false;
    }

    const std::string prefix(text.substr(0, wildcard));
    std::uint32_t unused = 0;
    if (!parsePositiveUint32(prefix, unused))
    {
      error = "talkgroup prefix must contain digits before the wildcard";
      return false;
    }

    pattern.type =
        FederationLibrary::TalkgroupPattern::TYPE_PREFIX;
    pattern.prefix = prefix;
    return true;
  }
  bool matchesTalkgroupPattern(
      const FederationLibrary::TalkgroupPattern& pattern,
      std::uint32_t tg)
  {
    if (pattern.type ==
        FederationLibrary::TalkgroupPattern::TYPE_EXACT)
    {
      return pattern.exact == tg;
    }

    std::ostringstream tg_stream;
    tg_stream << tg;
    const std::string tg_text(tg_stream.str());

    return (pattern.prefix.size() <= tg_text.size()) &&
           (tg_text.compare(0,
                            pattern.prefix.size(),
                            pattern.prefix) == 0);
  }

  bool matchesAnyPattern(
      const std::vector<FederationLibrary::TalkgroupPattern>& patterns,
      std::uint32_t tg)
  {
    for (std::vector<FederationLibrary::TalkgroupPattern>::
             const_iterator it=patterns.begin();
         it!=patterns.end(); ++it)
    {
      if (matchesTalkgroupPattern(*it, tg))
      {
        return true;
      }
    }

    return false;
  }

  bool parsePatternList(
      const Json::Value& values,
      const std::string& peer,
      const std::string& direction,
      std::vector<FederationLibrary::TalkgroupPattern>& patterns,
      std::string& error)
  {
    if (!values.isArray())
    {
      error = "Peer policy " + peer + "/" + direction +
              " must be an array";
      return false;
    }

    for (Json::Value::ArrayIndex index=0;
         index<values.size(); ++index)
    {
      FederationLibrary::TalkgroupPattern pattern;
      std::string pattern_error;

      if (!parseTalkgroupPattern(values[index],
                                 pattern,
                                 pattern_error))
      {
        std::ostringstream message;
        message << "Peer policy " << peer << "/"
                << direction << " entry " << index
                << ": " << pattern_error;
        error = message.str();
        return false;
      }

      patterns.push_back(pattern);
    }

    return true;
  }


  bool validatePeerPolicies(
      const Json::Value& root,
      std::vector<FederationLibrary::PeerPolicy>& policies,
      std::string& error)
  {
    if (!root.isMember("peer_policy"))
    {
      return true;
    }

    const Json::Value& policy_root(root["peer_policy"]);
    if (!policy_root.isObject())
    {
      error = "Federation library peer_policy must be an object";
      return false;
    }

    const Json::Value::Members peers(policy_root.getMemberNames());

    for (Json::Value::Members::const_iterator it=peers.begin();
         it!=peers.end(); ++it)
    {
      if (it->empty())
      {
        error = "Federation peer policy name must not be empty";
        return false;
      }

      const Json::Value& value(policy_root[*it]);
      if (!value.isObject())
      {
        error = "Federation peer policy " + *it +
                " must be an object";
        return false;
      }

      FederationLibrary::PeerPolicy policy;
      policy.peer = *it;

      if (!parsePatternList(value["import"],
                            policy.peer,
                            "import",
                            policy.import_patterns,
                            error))
      {
        return false;
      }

      if (!parsePatternList(value["export"],
                            policy.peer,
                            "export",
                            policy.export_patterns,
                            error))
      {
        return false;
      }

      policies.push_back(policy);
    }

    return true;
  }
} /* namespace */


/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/

FederationLibrary::FederationLibrary(void)
  : m_schema(0), m_generation(0)
{
} /* FederationLibrary::FederationLibrary */


FederationLibrary::~FederationLibrary(void)
{
} /* FederationLibrary::~FederationLibrary */

const FederationLibrary::Route*
FederationLibrary::findRoute(std::uint32_t tg) const
{
  // An exact talkgroup always has the highest precedence.
  for (std::vector<Route>::const_iterator it=m_routes.begin();
       it!=m_routes.end(); ++it)
  {
    if ((it->type == Route::TYPE_EXACT) && (it->first == tg))
    {
      return &(*it);
    }
  }

  // Of all matching ranges, select the narrowest.
  const Route* best_route = 0;
  std::uint32_t best_span =
      std::numeric_limits<std::uint32_t>::max();

  for (std::vector<Route>::const_iterator it=m_routes.begin();
       it!=m_routes.end(); ++it)
  {
    if ((it->type == Route::TYPE_RANGE) &&
        (tg >= it->first) && (tg <= it->last))
    {
      const std::uint32_t span = it->last - it->first;
      if ((best_route == 0) || (span < best_span))
      {
        best_route = &(*it);
        best_span = span;
      }
    }
  }

  if (best_route != 0)
  {
    return best_route;
  }

  // Of all matching prefixes, select the longest.
  std::ostringstream tg_stream;
  tg_stream << tg;
  const std::string tg_text(tg_stream.str());
  std::size_t best_prefix_length = 0;

  for (std::vector<Route>::const_iterator it=m_routes.begin();
       it!=m_routes.end(); ++it)
  {
    if ((it->type == Route::TYPE_PREFIX) &&
        (it->prefix.size() <= tg_text.size()) &&
        (tg_text.compare(0, it->prefix.size(), it->prefix) == 0) &&
        ((best_route == 0) ||
         (it->prefix.size() > best_prefix_length)))
    {
      best_route = &(*it);
      best_prefix_length = it->prefix.size();
    }
  }

  return best_route;
} /* FederationLibrary::findRoute */


bool FederationLibrary::hasPeerPolicy(const std::string& peer) const
{
  for (std::vector<PeerPolicy>::const_iterator
           it=m_peer_policies.begin();
       it!=m_peer_policies.end(); ++it)
  {
    if (it->peer == peer)
    {
      return true;
    }
  }

  return false;
} /* FederationLibrary::hasPeerPolicy */


bool FederationLibrary::mayImport(const std::string& peer,
                                  std::uint32_t tg) const
{
  for (std::vector<PeerPolicy>::const_iterator
           it=m_peer_policies.begin();
       it!=m_peer_policies.end(); ++it)
  {
    if (it->peer == peer)
    {
      return matchesAnyPattern(it->import_patterns, tg);
    }
  }

  return false;
} /* FederationLibrary::mayImport */


bool FederationLibrary::mayExport(const std::string& peer,
                                  std::uint32_t tg) const
{
  for (std::vector<PeerPolicy>::const_iterator
           it=m_peer_policies.begin();
       it!=m_peer_policies.end(); ++it)
  {
    if (it->peer == peer)
    {
      return matchesAnyPattern(it->export_patterns, tg);
    }
  }

  return false;
} /* FederationLibrary::mayExport */



bool FederationLibrary::load(const std::string& path,
                             const std::string& expected_domain,
                             std::string& error)
{
  std::ifstream stream(path.c_str());
  if (!stream.is_open())
  {
    error = "Could not open federation library: " + path;
    return false;
  }

  Json::CharReaderBuilder builder;
  builder["collectComments"] = false;

  Json::Value root;
  std::string parse_errors;
  if (!Json::parseFromStream(builder, stream, &root, &parse_errors))
  {
    error = "Could not parse federation library: " + parse_errors;
    return false;
  }

  if (!root.isObject())
  {
    error = "Federation library root must be a JSON object";
    return false;
  }

  if (!root["schema"].isUInt() || (root["schema"].asUInt() != 1))
  {
    error = "Federation library schema must be 1";
    return false;
  }

  if (!root["generation"].isUInt64() ||
      (root["generation"].asUInt64() == 0))
  {
    error = "Federation library generation must be a positive integer";
    return false;
  }

  const std::uint64_t candidate_generation =
      root["generation"].asUInt64();

  if ((m_generation != 0) && (candidate_generation <= m_generation))
  {
    std::ostringstream message;
    message << "Federation library generation "
            << candidate_generation
            << " is not newer than active generation "
            << m_generation;
    error = message.str();
    return false;
  }

  if (!root["domain"].isString() || root["domain"].asString().empty())
  {
    error = "Federation library domain is missing or empty";
    return false;
  }

  const std::string candidate_domain(root["domain"].asString());
  if (!expected_domain.empty() && (candidate_domain != expected_domain))
  {
    error = "Federation library domain does not match FEDERATION/DOMAIN";
    return false;
  }

  if (!root["routes"].isArray())
  {
    error = "Federation library routes must be an array";
    return false;
  }

  const Json::Value& routes(root["routes"]);
  std::vector<Route> candidate_routes;
  candidate_routes.reserve(routes.size());

  for (Json::Value::ArrayIndex index=0; index<routes.size(); ++index)
  {
    Route parsed_route;
    if (!validateRoute(routes[index], index, parsed_route, error))
    {
      return false;
    }

    candidate_routes.push_back(parsed_route);
  }

  std::vector<PeerPolicy> candidate_peer_policies;
  if (!validatePeerPolicies(root, candidate_peer_policies, error))

  {
    return false;
  }
  // Commit metadata only after the complete candidate has passed validation.
  m_schema = root["schema"].asUInt();
  m_generation = candidate_generation;
  m_domain = candidate_domain;
  m_routes.swap(candidate_routes);
  m_peer_policies.swap(candidate_peer_policies);

  error.clear();
  return true;
} /* FederationLibrary::load */
