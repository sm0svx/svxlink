/**
@file	 LocationInfo.cpp
@brief   Contains the infrastructure for APRS based EchoLink status updates
@author  Adi/DL1HRC and Steve/DH1DM
@date	 2009-03-12

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2025 Tobias Blomberg / SM0SVX

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
#include <sstream>
#include <limits>
#include <string>
#include <regex>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncConfig.h>
#include <AsyncTimer.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "version/SVXLINK.h"
#include "LocationInfo.h"
#include "AprsTcpClient.h"
#include "AprsUdpClient.h"


/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/

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


/****************************************************************************
 *
 * Prototypes for local functions
 *
 ****************************************************************************/

void print_error(const std::string& name, const std::string& variable,
                 const std::string& value, const std::string& example = "");
std::string addrStr(const std::string& source, const std::string& dest,
                    const std::string& path);


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

LocationInfo* LocationInfo::_instance = nullptr;


LocationInfo::LocationInfo(void)
{
  aprs_stats_timer.expired.connect(sigc::hide(
      sigc::mem_fun(*this, &LocationInfo::sendAprsStatistics)));
} /* LocationInfo::LocationInfo */


LocationInfo::~LocationInfo(void)
{
  for (const auto client : clients)
  {
    delete client;
  }
  clients.clear();

  delete aprspty;
  aprspty = nullptr;
} /* LocationInfo::~LocationInfo */


bool LocationInfo::initialize(Async::Config& cfg, const std::string& cfg_name)
{
    // Check if already initialized
  if (has_instance()) return false;

  bool init_ok = true;

  _instance = new LocationInfo;
  auto& loc_cfg = _instance->loc_cfg;

  std::string sourcecall;
  std::string logincall;
  const std::regex el_call_re("E([LR])-([0-9A-Z]{4,6})");
  //const std::regex call_re("([0-9A-Z]{4,6})(-(?:[1-9]|1[0-5]))?");
  const std::regex ax25_call_re("([0-9A-Z]{4,6})(-(?:[1-9]|1[0-5]))?");
  const std::regex source_call_re("([A-Za-z0-9]{3,6})(-[A-Za-z0-9]{1,2})?");
  const std::regex object_re("[\x21-\x7E][\x20-\x7E]{0,8}");
  std::smatch m;
  cfg.getValue(cfg_name, "CALLSIGN", loc_cfg.objectname);
  //if (cfg.getValue(cfg_name, "CALLSIGN", loc_cfg.objectname))
  //{
  //  std::cerr << "*** WARNING: The '" << cfg_name << "/CALLSIGN' configuration "
  //               "variable is deprecated and will be removed in the future. "
  //               "Use the '" << cfg_name << "/OBJECT_NAME' configuration "
  //               "variable instead." << std::endl;
  //}
  //cfg.getValue(cfg_name, "OBJECT_NAME", loc_cfg.objectname);
  if (std::regex_match(loc_cfg.objectname, m, el_call_re))
  {
    loc_cfg.prefix = m[1];
    loc_cfg.mycall = m[2];
    loc_cfg.statscall = loc_cfg.objectname;
    sourcecall = loc_cfg.mycall;
    loc_cfg.symbol = "E0";
  }
  else if (std::regex_match(loc_cfg.objectname, m, ax25_call_re))
  {
    loc_cfg.statscall = loc_cfg.objectname;
    sourcecall = m[1].str() + m[2].str();
  }
  else if (std::regex_match(loc_cfg.objectname, m, source_call_re))
  {
    loc_cfg.statscall = loc_cfg.objectname;
  }
  else if (!std::regex_match(loc_cfg.objectname, m, object_re))
  {
    std::cerr << "*** ERROR: Variable " << cfg_name << "/CALLSIGN is malformed."
              << " Either it must be an APRS object name or it need to be a "
                 "valid APRS source callsign. If it starts with EL- or ER- it "
                 "will be interpreted as an Echolink APRS object\n"
              << "Example: CALLSIGN=EL-DL1ABC"
              << std::endl;
    return false;
  }

  if (!cfg.getValue(cfg_name, "SOURCE_CALLSIGN", sourcecall, true) ||
      !std::regex_match(sourcecall, m, source_call_re))
  {
    std::cerr << "*** ERROR: The APRS source callsign '"
              << sourcecall << "' is invalid. Use the '" << cfg_name
              << "/SOURCE_CALLSIGN' configuration variable to set a valid "
              << "callsign." << std::endl;
    return false;
  }
  loc_cfg.sourcecall = sourcecall;
  logincall = sourcecall;
  if (loc_cfg.statscall.empty())
  {
    loc_cfg.statscall = sourcecall;
  }

  const std::regex login_call_re("([A-Za-z0-9]{3,9})(-[A-Za-z0-9]{1,2})?");
  if (!cfg.getValue(cfg_name, "LOGIN_CALLSIGN", logincall, true) ||
      !std::regex_match(logincall, m, login_call_re) ||
      (logincall.size() > 9) || (m[2] == "-0"))
  {
    std::cerr << "*** ERROR: The APRS-IS login callsign '"
              << logincall << "' is invalid. Use the '" << cfg_name
              << "/LOGIN_CALLSIGN' configuration variable to set a valid "
              << "callsign." << std::endl;
    return false;
  }
  loc_cfg.logincall = m[1];
  loc_cfg.loginssid = m[2];

  cfg.getValue(cfg_name, "COMMENT", loc_cfg.comment);
  if (loc_cfg.comment.size() > 36)
  {
    std::cerr << "*** WARNING: The APRS comment specified in " << cfg_name
              << "/COMMENT is too long. The maximum length is 36 characters."
              << std::endl;
    loc_cfg.comment.erase(36);
  }

  loc_cfg.debug = false;
  cfg.subscribeValue(cfg_name, "DEBUG", loc_cfg.debug,
      [](bool debug)
      {
        LocationInfo::_instance->loc_cfg.debug = debug;
      });

  //unsigned dest_num = 1;
  //cfg.getValue(cfg_name, "DESTINATION_NUM", 1U, 9U, dest_num);
  //std::stringstream dest_ss;
  //dest_ss << "APSVX" << dest_num;
  //loc_cfg.destination = dest_ss.str();

  init_ok &= _instance->parsePosition(cfg, cfg_name);
  init_ok &= _instance->parseStationHW(cfg, cfg_name);
  init_ok &= _instance->parsePath(cfg, cfg_name);
  init_ok &= _instance->parseClients(cfg, cfg_name);

  //cfg.getValue(cfg_name, "STATISTICS_CALLSIGN", loc_cfg.statscall);
  if (!loc_cfg.statscall.empty())
  {
    cfg.getValue(cfg_name, "STATISTICS_LOGIC", _instance->slogic);
    cfg.getValue(cfg_name, "STATISTICS_INTERVAL", 5U, 60U,
        _instance->sinterval);
    _instance->startStatisticsTimer(_instance->sinterval * 60 * 1000);
  }

  cfg.getValue(cfg_name, "SYMBOL", loc_cfg.symbol);
  if (loc_cfg.symbol.size() != 2)
  {
    std::cerr << "*** ERROR: The APRS symbol specified in " << cfg_name
              << "/SYMBOL must be exactly two characters"
              << std::endl;
    return false;
  }

  cfg.getValue(cfg_name, "FILTER", loc_cfg.filter);

  _instance->initExtPty(cfg.getValue(cfg_name, "PTY_PATH"));

  if(!init_ok)
  {
    deleteInstance();
  }

  return init_ok;
} /* LocationInfo::initialize */


void LocationInfo::updateDirectoryStatus(StationData::Status status)
{
  for (const auto client : clients)
  {
    client->updateDirectoryStatus(status);
  }
} /* LocationInfo::updateDirectoryStatus */


void LocationInfo::updateQsoStatus(int action, const std::string& call,
                                   const std::string& info,
                                   const std::list<std::string>& calls)
{
  for (const auto client : clients)
  {
    client->updateQsoStatus(action, call, info, calls);
  }
} /* LocationInfo::updateQsoStatus */


void LocationInfo::update3rdState(const std::string& call,
                                  const std::string& info)
{
  for (const auto client : clients)
  {
    client->update3rdState(call, info);
  }
} /* LocationInfo::update3rdState */


void LocationInfo::igateMessage(const std::string& info)
{
  for (const auto client : clients)
  {
    client->igateMessage(info);
  }
} /* LocationInfo::igateMessage */


bool LocationInfo::getTransmitting(const std::string &name)
{
   return aprsStats(name).is_transmitting;
} /* LocationInfo::getTransmitting */


void LocationInfo::setTransmitting(const std::string& name,
                                   bool is_transmitting,
                                   LocationInfo::Timepoint tp)
{
  AprsStatistics& stats = aprsStats(name);
  if (is_transmitting == stats.is_transmitting)
  {
    return;
  }
  stats.is_transmitting = is_transmitting;
  if (is_transmitting)
  {
    stats.tx_on_nr++;
    stats.last_tx_tp = tp;
  }
  else
  {
    stats.tx_sec += tp - stats.last_tx_tp;
  }
} /* LocationInfo::setTransmitting */


void LocationInfo::setReceiving(const std::string& name,
                                bool is_receiving,
                                const LocationInfo::Timepoint& tp)
{
  AprsStatistics& stats = aprsStats(name);
  if (is_receiving == stats.is_receiving)
  {
    return;
  }
  stats.is_receiving = is_receiving;
  if (is_receiving)
  {
    stats.rx_on_nr += 1;
    stats.last_rx_tp = tp;
  }
  else
  {
    stats.rx_sec += tp - stats.last_rx_tp;
  }
} /* LocationInfo::setReceiving */


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

bool LocationInfo::parsePosition(const Config &cfg, const std::string& name)
{
  bool success = true;

  std::string pos_str(cfg.getValue(name, "LAT_POSITION"));
  if (!parseLatitude(loc_cfg.lat_pos, pos_str)) // weshalb wird loc_cfg benötigt (und nicht cfg??)
  {
    print_error(name, "LAT_POSITION", pos_str, "LAT_POSITION=51.20.10N");
    success = false;
  }

  pos_str = cfg.getValue(name,"LON_POSITION");
  if (!parseLongitude(loc_cfg.lon_pos, pos_str))
  {
    print_error(name, "LON_POSITION", pos_str, "LON_POSITION=12.10.30E");
    success = false;
  }

  return success;

} /* LocationInfo::parsePosition */


bool LocationInfo::parseLatitude(Coordinate &pos, const std::string& value)
{
  unsigned int deg, min, sec;
  char dir, sep[2];
  std::stringstream ss(value);

  ss >> deg >> sep[0] >> min >> sep[1] >> sec >> dir >> std::ws;

  if (ss.fail() || !ss.eof())
  {
    return false;
  }
  if (sep[0] != '.'|| sep[1] != '.')
  {
    return false;
  }
  if ((deg > 90) || (min > 59) || (sec > 59) ||
      ((deg == 90) && ((min > 0) || (sec > 0))))
  {
    return false;
  }
  if ((dir != 'N') && (dir != 'S'))
  {
    return false;
  }

  pos.deg = deg;
  pos.min = min;
  pos.sec = sec;
  pos.dir = dir;

  return true;

} /* LocationInfo::parseLatitude */


bool LocationInfo::parseLongitude(Coordinate &pos, const std::string& value)
{
  unsigned int deg, min, sec;
  char dir, sep[2];
  std::stringstream ss(value);

  ss >> deg >> sep[0] >> min >> sep[1] >> sec >> dir >> std::ws;

  if (ss.fail() || !ss.eof())
  {
    return false;
  }
  if (sep[0] != '.'|| sep[1] != '.')
  {
    return false;
  }
  if ((deg > 180) || (min > 59) || (sec > 59) ||
      ((deg == 180) && ((min > 0) || (sec > 0))))
  {
    return false;
  }
  if ((dir != 'E') && (dir != 'W'))
  {
    return false;
  }

  pos.deg = deg;
  pos.min = min;
  pos.sec = sec;
  pos.dir = dir;

  return true;

} /* LocationInfo::parseLongitude */


bool LocationInfo::parseStationHW(const Async::Config &cfg,
                                  const std::string& name)
{
  bool success = true;

  double frequency = 0.0;
  if (!cfg.getValue(name, "FREQUENCY", 0.001, 24299.999, frequency))
  {
    print_error(name, "FREQUENCY", cfg.getValue(name, "FREQUENCY"),
                "FREQUENCY=438.875");
    success = false;
  }
  else
  {
    loc_cfg.frequency = lrint(1000.0 * frequency);
  }

  if (!cfg.getValue(name, "FREQUENCY_SEPARATOR", loc_cfg.freq_sep, true) ||
      (loc_cfg.freq_sep.size() > 1))
  {
    print_error(name, "FREQUENCY_SEPARATOR",
        cfg.getValue(name, "FREQUENCY_SEPARATOR"), "FREQUENCY_SEPARATOR=\"/\"");
    success = false;
  }

  if (!cfg.getValue(name, "TX_OFFSET", -9990, 9990, loc_cfg.tx_offset_khz, true))
  {
    print_error(name, "TX_OFFSET", cfg.getValue(name, "TX_OFFSET"),
                "TX_OFFSET=-600");
    success = false;
  }

  cfg.getValue(name, "NARROW", loc_cfg.narrow);

  if (!cfg.getValue(name, "TX_POWER", 1U,
                    std::numeric_limits<unsigned int>::max(), loc_cfg.power))
  {
    print_error(name, "TX_POWER", cfg.getValue(name, "TX_POWER"),
                "TX_POWER=8");
    success = false;
  }

  if (!cfg.getValue(name, "ANTENNA_GAIN", loc_cfg.gain, true))
  {
    print_error(name, "ANTENNA_GAIN", cfg.getValue(name, "ANTENNA_GAIN"),
                "ANTENNA_GAIN=6");
    success = false;
  }

  if (!parseAntennaHeight(loc_cfg, cfg.getValue(name, "ANTENNA_HEIGHT")))
  {
    print_error(name, "ANTENNA_HEIGHT", cfg.getValue(name, "ANTENNA_HEIGHT"),
                "ANTENNA_HEIGHT=10m");
    success = false;
  }

  if (!cfg.getValue(name, "ANTENNA_DIR", loc_cfg.beam_dir, true))
  {
    print_error(name, "ANTENNA_DIR", cfg.getValue(name, "ANTENNA_DIR"),
                "ANTENNA_DIR=-1");
    success = false;
  }

  float tone = 0.0f;
  if (cfg.getValue(name, "TONE", 0.0f, 9999.0f, tone, true))
  {
    loc_cfg.tone = floorf(tone);
  }
  else
  {
    print_error(name, "TONE", cfg.getValue(name, "TONE"), "TONE=0");
    success = false;
  }

  int binterval = 10;
  int max = std::numeric_limits<int>::max();
  if (!cfg.getValue(name, "BEACON_INTERVAL", 10, max, binterval, true))
  {
    print_error(name, "BEACON_INTERVAL", cfg.getValue(name, "BEACON_INTERVAL"),
                "BEACON_INTERVAL=10");
    success = false;
  }
  else
  {
    loc_cfg.binterval = binterval;
  }

  loc_cfg.range = calculateRange(loc_cfg);

  return success;

} /* LocationInfo::parseStationHW */


bool LocationInfo::parsePath(const Async::Config &cfg, const std::string& name)
{
    // FIXME: Verify the path syntax!
    //        http://www.aprs.org/newN/new-eu-paradigm.txt
  cfg.getValue(name, "PATH", loc_cfg.path);
  return true;
} /* LocationInfo::parsePath */


int LocationInfo::calculateRange(const Cfg &cfg)
{
  float range_factor(1.0);

  if (cfg.range_unit == 'k')
  {
    range_factor = 1.60934;
  }

  double tmp = sqrt(2.0 * loc_cfg.height * sqrt((loc_cfg.power / 10.0) *
                        pow(10, loc_cfg.gain / 10.0) / 2)) * range_factor;

  return lrintf(tmp);

} /* LocationInfo::calculateRange */


bool LocationInfo::parseAntennaHeight(Cfg &cfg, const std::string value)
{
  char unit;
  unsigned int height;

  if (value.empty())
  {
    return true;
  }

  std::stringstream ss(value);
  ss >> height >> unit >> std::ws;
  if (ss.fail() || !ss.eof())
  {
    return false;
  }
  cfg.height = height;

  if (unit == 'm' || unit == 'M')
  {
      // Convert metric antenna height into feet
    cfg.height = lrintf(loc_cfg.height * 3.2808);
    cfg.range_unit = 'k';
  }

  return true;

} /* LocationInfo::parseAntennaHeight */


bool LocationInfo::parseClients(const Async::Config &cfg,
                                const std::string& name)
{
  std::string aprs_server_list(cfg.getValue(name, "APRS_SERVER_LIST"));
  std::stringstream clientStream(aprs_server_list);
  std::string client, host;
  int port;
  bool success = true;

  while (clientStream >> client)
  {
    if (!parseClientStr(host, port, client))
    {
      print_error(name, "APRS_SERVER_LIST", aprs_server_list,
                  "APRS_SERVER_LIST=euro.aprs2.net:14580");
      success = false;
    }
    else
    {
      AprsTcpClient *client = new AprsTcpClient(loc_cfg, host, port);
      clients.push_back(client);
    }
  }

  clientStream.clear();
  std::string status_server_list(cfg.getValue(name, "STATUS_SERVER_LIST"));
  clientStream.str(status_server_list);
  while (clientStream >> client)
  {
    if (!parseClientStr(host, port, client))
    {
      print_error(name, "STATUS_SERVER_LIST", status_server_list,
                  "STATUS_SERVER_LIST=aprs.echolink.org:5199");
      success = false;
    }
    else
    {
      AprsUdpClient *client = new AprsUdpClient(loc_cfg, host, port);
      clients.push_back(client);
    }
  }

  return success;

} /* LocationInfo::parseClients */


bool LocationInfo::parseClientStr(std::string& host, int &port,
                                  const std::string& val)
{
  if (val.empty())
  {
    return false;
  }

  int tmpPort;
  std::string::size_type hostEnd;

  hostEnd = val.find_last_of(':');
  if (hostEnd == std::string::npos)
  {
    return false;
  }

  std::string portStr(val.substr(hostEnd+1, std::string::npos));
  std::stringstream ssval(portStr);
  ssval >> tmpPort;
  if (!ssval)
  {
    return false;
  }

  if (tmpPort < 0 || tmpPort > 0xffff)
  {
    return false;
  }

  host = val.substr(0, hostEnd);
  port = tmpPort;

  return true;

} /* LocationInfo::parseClientStr */


void LocationInfo::startStatisticsTimer(int sinterval)
{
  aprs_stats_timer.setTimeout(sinterval);
  aprs_stats_timer.setEnable(true);
} /* LocationInfo::statStatisticsTimer */


void LocationInfo::sendAprsStatistics(void)
{
  // https://github.com/PhirePhly/aprs_notes/blob/master/telemetry_format.md

    // FROM>APSVXn,VIA1,VIA2,VIAn:
  std::ostringstream addr;
  addr << addrStr(loc_cfg.sourcecall, loc_cfg.destination, loc_cfg.path);

    // :ADDRESSEE:
  std::ostringstream addressee;
  addressee << ":" << std::left << std::setw(9) << loc_cfg.statscall << ":";

  const auto now = Clock::now();
  bool send_metadata = (now - last_tlm_metadata > std::chrono::minutes(59));
  if (send_metadata)
  {
    last_tlm_metadata = now;

      // PARM.A1,A2,A3,A4,A5,B1,B2,B3,B4,B5,B6,B7,B8
    std::ostringstream parm;
    parm << addr.str()
         << addressee.str()
         << "PARM."
         << "RX Avg " << sinterval << "m"     // A1
         << ",TX Avg " << sinterval << "m"    // A2
         << ",RX Count " << sinterval << "m"  // A3
         << ",TX Count " << sinterval << "m"  // A4
         << ","                               // A5
         << ",RX"                             // B1
         << ",TX"                             // B2
         ;
    igateMessage(parm.str());

      // UNIT.A1,A2,A3,A4,A5,B1,B2,B3,B4,B5,B6,B7,B8
    std::ostringstream unit;
    unit << addr.str()
         << addressee.str()
         << "UNIT."
         << "erlang"          // A1
         << ",erlang"         // A2
         << ",receptions"     // A3
         << ",transmissions"  // A4
         ;
    igateMessage(unit.str());
  }

    // Loop for each logic
  for (auto& entry : LocationInfo::instance()->aprs_stats)
  {
    const std::string& logic_name = entry.first;
    AprsStatistics& stats = entry.second;

    if (!slogic.empty() && (logic_name != slogic))
    {
      continue;
    }

      // Remember squelch and tx state then force inactive to finish statistics
      // interval
    bool is_receiving = stats.is_receiving;
    bool is_transmitting = stats.is_transmitting;
    setReceiving(logic_name, false, now);
    setTransmitting(logic_name, false, now);

    const double erlang_b = 1.0 / 999.0;
    if (send_metadata)
    {
        // BITS.XXXXXXXX,Project Title
      std::ostringstream bits;
      bits << addr.str()
           << addressee.str()
           << "BITS.11111111,SvxLink " << logic_name;
      igateMessage(bits.str());

        // EQNS.a,b,c,a,b,c,a,b,c,a,b,c,a,b,c
      std::ostringstream eqns;
      eqns << addr.str()
           << addressee.str()
           << "EQNS."
           << "0"                                                   // A1a
           << "," << std::fixed << std::setprecision(5) << erlang_b // A1b
           << ",0"                                                  // A1c
           << ",0"                                                  // A2a
           << "," << std::fixed << std::setprecision(5) << erlang_b // A2b
           << ",0"                                                  // A2c
           ;
      igateMessage(eqns.str());
    }

      // T#nnn,nnn,nnn,nnn,nnn,nnn,nnnnnnnn
    auto rx_erlang = stats.rx_sec.count() / (60.0 * sinterval);
    auto tx_erlang = stats.tx_sec.count() / (60.0 * sinterval);
    std::ostringstream tlm;
    tlm << addrStr(loc_cfg.statscall, loc_cfg.destination, loc_cfg.path)
        << "T#" << std::setw(3) << std::setfill('0') << sequence  // Sequence
        << "," << std::setw(3) << std::setfill('0')               // A1
               << std::min(std::lrint(rx_erlang / erlang_b), 999L)
        << "," << std::setw(3) << std::setfill('0')               // A2
               << std::min(std::lrint(tx_erlang / erlang_b), 999L)
        << "," << std::setw(3) << std::setfill('0')               // A3
          << std::min(stats.rx_on_nr, 999U)
        << "," << std::setw(3) << std::setfill('0')               // A4
          << std::min(stats.tx_on_nr, 999U)
        << ",000"                                                 // A5
        << ","
        << (is_receiving ? 1 : 0)                                 // B1
        << (is_transmitting ? 1 : 0)                              // B2
        << "000000"                                               // B3-B8
        ;
    igateMessage(tlm.str());

      // Reset statistics
    stats.reset();

      // Restore squelch and tx state
    setReceiving(logic_name, is_receiving, now);
    setTransmitting(logic_name, is_transmitting, now);

      // Advance sequence number
    sequence = (sequence < 999) ? sequence+1 : 0;
  }
} /* LocationInfo::sendAprsStatistics */


void LocationInfo::initExtPty(std::string ptydevice)
{
  aprspty = new Async::Pty(ptydevice);
  aprspty->setLineBuffered(true);
  if (!aprspty || !aprspty->open())
  {
    std::cerr << "*** ERROR: Failed to open the aprs pty device "
              << ptydevice << std::endl;
  }
  else
  {
     aprspty->dataReceived.connect(
         sigc::mem_fun(*this, &LocationInfo::mesReceived));
  }
} /* LocationInfo::initExtPty */


void LocationInfo::mesReceived(const void* buf, size_t len)
{
  const char* ptr = static_cast<const char*>(buf);
  std::string message(ptr, ptr+len);
  std::string loc_call = loc_cfg.sourcecall;
  size_t found = message.find("XXXXXX");

  if (found != std::string::npos)
  {
    message.replace(found, 6, loc_call);
  }

  std::cout << "APRS PTY: " << message << std::endl;

  igateMessage(message);
} /* LocationInfo::mesReceived */


// This function was added to get around an issue that might cause problems on
// an older gcc (<7.1) on ARM platforms. The problem will cause this warning to
// be issued on the older compilers:
// "note: parameter passing for argument of type ‘...’ will change in GCC 7.1"
LocationInfo::AprsStatistics&
LocationInfo::aprsStats(const std::string& logic_name)
{
  auto it = aprs_stats.find(logic_name);
  if (it == aprs_stats.end())
  {
    auto emplace_res = aprs_stats.emplace(logic_name, AprsStatistics());
    assert(emplace_res.second);
    it = emplace_res.first;
  }
  return it->second;
} /* LocationInfo::aprsStats */


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

void print_error(const std::string& name, const std::string& variable,
                 const std::string& value, const std::string& example)
{
  std::cerr << "*** ERROR: Config variable [" << name << "]/" << variable << "="
            << value << " wrong or not set.";

  if (!example.empty())
  {
    std::cerr << "\n*** Example: " <<  example;
  }
  std::cerr << std::endl;
} /* print_error */


std::string addrStr(const std::string& source, const std::string& dest,
                    const std::string& path)
{
    // MYCALL>APSVXn,path:
  std::string addr = source + ">" + dest;
  if (!path.empty())
  {
    addr += std::string(",") + path;
  }
  addr += ":";
  return addr;
} /* addrStr */

} // End of anonymous namespace


/*
 * This file has not been truncated
 */
