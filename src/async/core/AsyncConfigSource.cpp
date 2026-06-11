/**
@file   AsyncConfigSource.cpp
@brief  Implementation of configuration source URL parser
@author Rui Barreiros / CR7BPM
@date   2025-10-20

This file contains the implementation of the ConfigSource class that is used to
parse configuration source URLs and detect the backend type.

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2004-2026 Tobias Blomberg / SM0SVX

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it is useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
\endverbatim
*/

#include "AsyncConfigSource.h"
#include "AsyncConfigBackend.h"
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace Async
{

std::optional<ConfigSource> ConfigSource::parse(const std::string& url)
{
  if (url.empty())
  {
    std::cerr << "*** ERROR: Empty configuration source URL" << std::endl;
    return std::nullopt;
  }

  ConfigSource source;
  source.backend_type = detectBackendType(url);
  source.backend_type_name = getBackendTypeName(source.backend_type);

  if (source.backend_type == BACKEND_UNKNOWN)
  {
    std::cerr << "*** ERROR: Unknown backend type in configuration source URL"
              << std::endl;
    std::cerr << "*** Available backends: " << ConfigBackendFactory::validFactories() << std::endl;
    return std::nullopt;
  }

  // Check if backend is compiled in
  if (!isBackendAvailable(source.backend_type))
  {
    std::cerr << "*** ERROR: Backend '" << source.backend_type_name
              << "' not compiled in. Available: " << ConfigBackendFactory::validFactories() << std::endl;
    return std::nullopt;
  }

  // Extract connection info
  if (source.backend_type == BACKEND_FILE)
  {
    // For file backend, strip "file://" prefix
    if (url.substr(0, 7) == "file://")
    {
      source.connection_info = url.substr(7);
    }
    else
    {
      source.connection_info = url;
    }
  }
  else
  {
    // For database backends, parse URL
    if (!parseDatabaseURL(url, source.connection_info))
    {
      std::cerr << "*** ERROR: Failed to parse database URL" << std::endl;
      return std::nullopt;
    }
  }

  return source;
} /* ConfigSource::parse */

ConfigSource::BackendType ConfigSource::detectBackendType(const std::string& url)
{
  if (url.substr(0, 7) == "file://")
  {
    return BACKEND_FILE;
  }
  else if (url.substr(0, 9) == "sqlite://")
  {
    return BACKEND_SQLITE;
  }
  else if (url.substr(0, 8) == "mysql://")
  {
    return BACKEND_MYSQL;
  }
  else if (url.substr(0, 13) == "postgresql://")
  {
    return BACKEND_POSTGRESQL;
  }
  else if (url.find("://") == std::string::npos)
  {
    // No scheme means file backend
    return BACKEND_FILE;
  }

  return BACKEND_UNKNOWN;
} /* ConfigSource::detectBackendType */

std::string ConfigSource::getBackendTypeName(BackendType type)
{
  switch (type)
  {
    case BACKEND_FILE:       return "file";
    case BACKEND_SQLITE:     return "sqlite";
    case BACKEND_MYSQL:      return "mysql";
    case BACKEND_POSTGRESQL: return "postgresql";
    case BACKEND_UNKNOWN:    return "unknown";
    default:                 return "unknown";
  }
} /* ConfigSource::getBackendTypeName */

bool ConfigSource::parseDatabaseURL(const std::string& url,
                                    std::string& connection_string)
{
  // Expected formats:
  //   SQLite:     sqlite:///path/to/file.db
  //   MySQL:      mysql://[user:pass@]host[:port]/database
  //   PostgreSQL: postgresql://[user:pass@]host[:port]/database
  
  size_t scheme_end = url.find("://");
  if (scheme_end == std::string::npos)
  {
    return false;
  }

  std::string scheme = url.substr(0, scheme_end);
  std::string remainder = url.substr(scheme_end + 3);
  
  // Special case for SQLite - it's just a file path
  if (scheme == "sqlite")
  {
    connection_string = remainder;
    return true;
  }
  
  // Parse user:pass@ if present
  std::string user, pass, host, database;
  int port = -1;

  size_t at_pos = remainder.find('@');
  if (at_pos != std::string::npos)
  {
    // User/pass present
    std::string userpass = remainder.substr(0, at_pos);
    remainder = remainder.substr(at_pos + 1);

    size_t colon_pos = userpass.find(':');
    if (colon_pos != std::string::npos)
    {
      user = userpass.substr(0, colon_pos);
      pass = userpass.substr(colon_pos + 1);
    }
    else
    {
      user = userpass;
    }
  }

  // Parse host[:port]/database
  size_t slash_pos = remainder.find('/');
  if (slash_pos == std::string::npos)
  {
    std::cerr << "*** ERROR: Database URL missing database name" << std::endl;
    return false;
  }

  std::string hostport = remainder.substr(0, slash_pos);
  database = remainder.substr(slash_pos + 1);

  // Parse host:port
  size_t colon_pos = hostport.find(':');
  if (colon_pos != std::string::npos)
  {
    host = hostport.substr(0, colon_pos);
    try
    {
      port = std::stoi(hostport.substr(colon_pos + 1));
    }
    catch (const std::exception&)
    {
      std::cerr << "*** ERROR: Invalid port in database URL" << std::endl;
      return false;
    }
  }
  else
  {
    host = hostport;
  }

  // Build connection string in format: host,port,database,user,pass
  std::ostringstream oss;
  oss << host << ",";
  oss << (port > 0 ? std::to_string(port) : "") << ",";
  oss << database << ",";
  oss << user << ",";
  oss << pass;

  connection_string = oss.str();
  return true;
} /* ConfigSource::parseDatabaseURL */

bool ConfigSource::isBackendAvailable(const std::string& backend_type_name)
{
  if (backend_type_name == "file")
  {
    return true; // Always available
  }
#ifdef HAS_SQLITE_SUPPORT
  if (backend_type_name == "sqlite")
  {
    return true;
  }
#endif
#ifdef HAS_MYSQL_SUPPORT
  if (backend_type_name == "mysql")
  {
    return true;
  }
#endif
#ifdef HAS_POSTGRESQL_SUPPORT
  if (backend_type_name == "postgresql")
  {
    return true;
  }
#endif
  return false;
} /* ConfigSource::isBackendAvailable */

bool ConfigSource::isBackendAvailable(BackendType type)
{
  return isBackendAvailable(getBackendTypeName(type));
} /* ConfigSource::isBackendAvailable */

} /* namespace */

