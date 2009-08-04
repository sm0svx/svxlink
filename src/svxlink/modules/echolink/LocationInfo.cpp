/**
@file	 LocationInfo.cpp
@brief   Contains the infrastructure for APRS based EchoLink status updates
@author  Adi/DL1HRC and Steve/DH1DM
@date	 2009-03-12

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2009 Tobias Blomberg / SM0SVX

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

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <boost/tokenizer.hpp>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <version/SVXLINK.h>
#include <AsyncConfig.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "LocationInfo.h"
#include "AprsTcpClient.h"
#include "AprsUdpClient.h"


/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/

using namespace std;
using namespace Async;
using namespace EchoLink;


  // Put all locally defined types, functions and variables in an anonymous
  // namespace to make them file local. The "static" keyword has been
  // deprecated in C++ so it should not be used.
namespace
{

/****************************************************************************
 *
 * Defines & typedefs
 *
 ****************************************************************************/

typedef std::vector<std::string> StrVec;


/****************************************************************************
 *
 * Prototypes for local functions
 *
 ****************************************************************************/

int splitStr(StrVec &tokens, const std::string &str, const std::string &delims);


/****************************************************************************
 *
 * Local Global Variables
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Local class definitions
 *
 ****************************************************************************/



} // End of anonymous namespace

/****************************************************************************
 *
 * Exported Global Variables
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/

LocationInfo::LocationInfo(Async::Config &cfg, const std::string &name,
                           const std::string &callsign)
{
  StrVec aprs_servers;
  StrVec status_servers;
  string value;

  loc_cfg.mycall = callsign;

  if (cfg.getValue(name, "APRS_SERVER_LIST", value))
  {
    if (splitStr(aprs_servers, value, " ") == 0)
    {
      cerr << "*** ERROR: Illegal format for config variable " << name
            << "/APRS_SERVER_LIST. Should be host:port\n";
      return;
    }
  }

  if (cfg.getValue(name, "STATUS_SERVER_LIST", value))
  {
    if (splitStr(status_servers, value, " ") == 0)
    {
      cerr << "*** ERROR: Illegal format for config variable " << name
            << "/STATUS_SERVER_LIST. Should be host:port\n";
      return;
    }
  }

  if (!cfg.getValue(name, "LAT_POSITION", value) ||
      !parseLatitude(loc_cfg.lat_pos, value))
  {
    cerr << "*** ERROR: Config variable " << name
         << "/LAT_POSITION not set or wrong, example "
            "\"LAT_POSITION=51.20.00N\"\n";
    return;
  }

  if (!cfg.getValue(name, "LON_POSITION", value) ||
      !parseLongitude(loc_cfg.lon_pos, value))
  {
    cerr << "*** ERROR: Config variable " << name
         << "/LON_POSITION not set or wrong, example "
            "\"LON_POSITION=12.10.00E\"\n";
    return;
  }

  if (cfg.getValue(name, "FREQUENCY", value))
  {
    loc_cfg.frequency = lrintf(atof(value.c_str()) * 1000.0);
  }

  if (cfg.getValue(name, "TX_POWER", value))
  {
    loc_cfg.power = atoi(value.c_str());
  }

  if (cfg.getValue(name, "ANTENNA_GAIN", value))
  {
    loc_cfg.gain = atoi(value.c_str());
  }

  loc_cfg.range = 0;
  loc_cfg.range_unit = 'm';

  float range_factor = 1.0;
  if (cfg.getValue(name, "ANTENNA_HEIGHT", value))
  {
    char *suffix;
    loc_cfg.height = strtoul(value.c_str(), &suffix, 10);

      // Convert metric antenna height into feet
    if (suffix[0] == 'm' || suffix[0] == 'M')
    {
      loc_cfg.height = lrintf(loc_cfg.height * 3.2808);
      loc_cfg.range_unit = 'k';
      range_factor = 1.60934;
    }
  }

    // range calculation
  loc_cfg.range =
      lrintf(sqrt(2.0 * loc_cfg.height * sqrt((loc_cfg.power / 10.0) *
             pow10(loc_cfg.gain / 10.0) / 2)) * range_factor);
  
  loc_cfg.beam_dir = -1;
  if (cfg.getValue(name, "ANTENNA_DIR", value))
  {
    loc_cfg.beam_dir = atoi(value.c_str());
  }

  loc_cfg.tone = 0;
  if (cfg.getValue(name, "TONE", value))
  {
    loc_cfg.tone = strtoul(value.c_str(), NULL, 10);
  }

  loc_cfg.interval = 600000;
  if (cfg.getValue(name, "BEACON_INTERVAL", value))
  {
    loc_cfg.interval = strtoul(value.c_str(), NULL, 10) * 1000 * 60;
  }

  cfg.getValue(name, "PATH", loc_cfg.path);
  cfg.getValue(name, "COMMENT", loc_cfg.comment);

    // Iterate APRS server list
  for (StrVec::const_iterator it = aprs_servers.begin();
       it != aprs_servers.end(); it++)
  {
    StrVec server_args;

      // Try to split hostname:port
    if (splitStr(server_args, *it, ":") != 2)
    {
      cerr << "*** WARNING: Illegal format for APRS server configuration: "
           << *it << endl;
      continue;
    }
    
    string server = server_args[0];
    int port = strtoul(server_args[1].c_str(), NULL, 10);
    AprsTcpClient *client = new AprsTcpClient(loc_cfg, server, port);
    clients.push_back(client);
  }
  
    // Iterate EchoLink location server list
  for (StrVec::const_iterator it = status_servers.begin();
       it != status_servers.end(); it++)
  {
    StrVec server_args;

      // Try to split hostname:port
    if (splitStr(server_args, *it, ":") != 2)
    {
      cerr << "*** WARNING: Illegal format for EchoLink status server "
           << "configuration: " << *it << endl;
      continue;
    }
    
    string server = server_args[0];
    int port = strtoul(server_args[1].c_str(), NULL, 10);
    AprsUdpClient *client = new AprsUdpClient(loc_cfg, server, port);
    clients.push_back(client);
  }
} /* LocationInfo::LocationInfo */


LocationInfo::~LocationInfo(void)
{
  ClientList::const_iterator it;
  for (it = clients.begin(); it != clients.end(); it++)
  {
    delete *it;
  }
} /* LocationInfo::~LocationInfo */


void LocationInfo::updateDirectoryStatus(StationData::Status status)
{
  ClientList::const_iterator it;
  for (it = clients.begin(); it != clients.end(); it++)
  {
    (*it)->updateDirectoryStatus(status);
  }
} /* LocationInfo::updateDirectoryStatus */


void LocationInfo::updateQsoStatus(int action, const string& call,
  const std::string& info, list<string>& call_list)
{
  ClientList::const_iterator it;
  for (it = clients.begin(); it != clients.end(); it++)
  {
    (*it)->updateQsoStatus(action, call, info, call_list);
  }
} /* LocationInfo::updateQsoStatus */



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

bool LocationInfo::parseLatitude(Coordinate &lat_pos, const string &value)
{
  char *min, *sec, *end;
  
  lat_pos.deg = strtoul(value.c_str(), &min, 10);
  if ((lat_pos.deg > 89) || (min[0] != '.'))
  {
    return false;
  }
    
  lat_pos.min = strtoul(min + 1, &sec, 10);
  if ((lat_pos.min > 59) || (sec[0] != '.'))
  {
    return false;
  }
    
  lat_pos.sec = strtoul(sec + 1, &end, 10);
  if ((lat_pos.sec > 59) || (end[0] != 'N' && end[0] != 'S'))
  {
    return false;
  }
    
  lat_pos.dir = end[0];
  
  return true;
  
} /* LocationInfo::parseLatitude */


bool LocationInfo::parseLongitude(Coordinate &lon_pos, const string &value)
{
  char *min, *sec, *end;
  
  lon_pos.deg = strtoul(value.c_str(), &min, 10);
  if ((lon_pos.deg > 179) || (min[0] != '.'))
  {
    return false;
  }
  
  lon_pos.min = strtoul(min + 1, &sec, 10);
  if ((lon_pos.min > 59) || (sec[0] != '.'))
  {
    return false;
  }
  
  lon_pos.sec = strtoul(sec + 1, &end, 10);
  if ((lon_pos.sec > 59) || (end[0] != 'E' && end[0] != 'W'))
  {
    return false;
  }
  
  lon_pos.dir = end[0];
  
  return true;
  
} /* LocationInfo::parseLongitude */



/****************************************************************************
 *
 * Private local functions
 *
 ****************************************************************************/

  // Put all locally defined functions in an anonymous namespace to make them
  // file local. The "static" keyword has been deprecated in C++ so it
  // should not be used.
namespace
{

int splitStr(StrVec &tokens, const std::string &str, const std::string &delims)
{
  boost::char_separator<char> tok_func(delims.c_str());
  boost::tokenizer<boost::char_separator<char> > tok(str, tok_func);

  copy(tok.begin(), tok.end(), back_inserter(tokens));

  return tokens.size();
}


} // End of anonymous namespace

/*
 * This file has not been truncated
 */
