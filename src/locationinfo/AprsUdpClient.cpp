/**
@file	 AprsUdpClient.cpp
@brief   Contains an implementation of APRS updates via UDP
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

#include <iostream>
#include <cstdio>
#include <cmath>
#include <cstring>
#include <ctime>
#include <sys/time.h>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncTimer.h>
#include <rtp.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "version/SVXLINK.h"
#include "AprsUdpClient.h"


/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/

using namespace std;
using namespace Async;
using namespace EchoLink;


/****************************************************************************
 *
 * Defines & typedefs
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Local class definitions
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Public static functions
 *
 ****************************************************************************/

char AprsUdpClient::getPowerParam(unsigned int power)
{
  return '0' + std::min(lrintf(sqrtf(power)), 9L);
} /* AprsUdpClient::getPowerParam */


char AprsUdpClient::getHeightParam(unsigned int height)
{
  return '0' + lrintf(logf(height / 10.0f) / logf(2.0f));
} /* AprsUdpClient::getHeightParam */


char AprsUdpClient::getGainParam(unsigned int gain)
{
  return '0' + ((gain < 10) ? gain : 9);
} /* AprsUdpClient::getGainParam */


char AprsUdpClient::getDirectionParam(int beam_dir)
{
  if (beam_dir < 0)
  {
    return '0';
  }

  auto param = lrintf((beam_dir % 360) / 45.0f);
  if (param == 0)
  {
    return '8';
  }

  return '0' + param;
} /* AprsUdpClient::getDirectionParam */


std::string AprsUdpClient::phgStr(unsigned int power, unsigned int height,
                                  unsigned int gain, int beam_dir)
{
  std::ostringstream phg;
  phg << "PHG"
      << getPowerParam(power)
      << getHeightParam(height)
      << getGainParam(gain)
      << getDirectionParam(beam_dir)
      ;
  return phg.str();
} /* AprsUdpClient::phgStr */


/****************************************************************************
 *
 * Exported Global Variables
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Local Global Variables
 *
 ****************************************************************************/

#define HASH_KEY		0x73e2


/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/

AprsUdpClient::AprsUdpClient(LocationInfo::Cfg &loc_cfg,
            const std::string &server, int port)
  : loc_cfg(loc_cfg), server(server), port(port), dns(0), beacon_timer(0),
    curr_status(StationData::STAT_UNKNOWN), num_connected(0)
{
  if (!loc_cfg.prefix.empty())
  {
    beacon_timer = new Timer(loc_cfg.binterval * 60 * 1000,
                             Timer::TYPE_PERIODIC);
    beacon_timer->setEnable(false);
    beacon_timer->expired.connect(
        mem_fun(*this, &AprsUdpClient::sendLocationInfo));
  }
} /* AprsUdpClient::AprsUdpClient */


AprsUdpClient::~AprsUdpClient(void)
{
  updateDirectoryStatus(StationData::STAT_OFFLINE);
  delete beacon_timer;
} /* AprsUdpClient::~AprsUdpClient */


void AprsUdpClient::updateDirectoryStatus(StationData::Status status)
{
    // Check if system is properly initialized
  if (!beacon_timer)
  {
    return;
  }

    // Stop automatic beacon timer
  beacon_timer->reset();

    // Update status
  curr_status = status;

    // Build and send the packet
  sendLocationInfo();

    // Re-enable timer
  beacon_timer->setEnable(true);

} /* AprsUdpClient::updateDirectoryStatus */


void AprsUdpClient::updateQsoStatus(int action, const std::string& call,
                                    const std::string& info,
                                    const std::list<std::string>& calls)
{
    // Check if system is properly initialized
  if (!beacon_timer)
  {
    return;
  }

    // Stop automatic beacon timer
  beacon_timer->reset();

    // Update QSO connection status
  num_connected = calls.size();
  curr_call = num_connected ? calls.back() : "";

    // Build and send the packet
  sendLocationInfo();

    // Re-enable timer
  beacon_timer->setEnable(true);

} /* AprsUdpClient::updateQsoStatus */


void AprsUdpClient::update3rdState(const string& call, const string& info)
{
   // do nothing
} /* AprsUdpClient::update3rdState */



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

void AprsUdpClient::sendLocationInfo(Timer *t)
{
  if (ip_addr.isEmpty())
  {
    if (!dns)
    {
      dns = new DnsLookup(server);
      dns->resultsReady.connect(mem_fun(*this, &AprsUdpClient::dnsResultsReady));
    }
    return;
  }

  if (sock.initOk())
  {
    char sdes_packet[256];
    int sdes_len = buildSdesPacket(sdes_packet);

    //std::cout << "### AprsUdpClient::sendLocationInfo" << std::endl;
    sock.write(ip_addr, port, sdes_packet, sdes_len);
  }
} /* AprsUdpClient::sendLocationInfo */


void AprsUdpClient::dnsResultsReady(DnsLookup& dns_lookup)
{
  vector<IpAddress> result = dns->addresses();

  delete dns;
  dns = 0;

  if (result.empty() || result[0].isEmpty())
  {
    ip_addr.clear();
    return;
  }

  ip_addr = result[0];
  sendLocationInfo();

} /* AprsUdpClient::dnsResultsReady */


#define addText(block, text) \
  do { \
    int sl = strlen(text); \
    *block++ = sl; \
    memcpy(block, text, sl); \
    block += sl; \
  } while (0)


int AprsUdpClient::buildSdesPacket(char *p)
{
  time_t update;
  struct tm utc;
  char pos[128], info[80], tmp[256];
  char *ap;
  int ver, len;

    // Evaluate directory status
  switch(curr_status)
  {
    case StationData::STAT_OFFLINE:
    case StationData::STAT_UNKNOWN:
      sprintf(info, " Off @");
      break;

    case StationData::STAT_BUSY:
      sprintf(info, " Busy ");
      break;

    case StationData::STAT_ONLINE:
      switch(num_connected)
      {
        case 0:
          sprintf(info, " On  @");
          break;
        case 1:
          sprintf(info, "=%s ", curr_call.c_str());
          break;
        default:
          sprintf(info, "+%s ", curr_call.c_str());
          break;
      }
      break;
  }

    // Read update time
  time(&update);
  gmtime_r(&update, &utc);

    // Geographic position
  sprintf(pos, "%02d%02d.%02d%cE%03d%02d.%02d%c",
               loc_cfg.lat_pos.deg, loc_cfg.lat_pos.min,
               (loc_cfg.lat_pos.sec * 100) / 60, loc_cfg.lat_pos.dir,
               loc_cfg.lon_pos.deg, loc_cfg.lon_pos.min,
               (loc_cfg.lon_pos.sec * 100) / 60, loc_cfg.lon_pos.dir);

    // Set SDES version/misc data
  ver = (RTP_VERSION << 14) | RTCP_SDES | (1 << 8);
  p[0] = ver >> 8;
  p[1] = ver;

    // Set SDES source
  p[4] = 0;
  p[5] = 0;
  p[6] = 0;
  p[7] = 0;

    // At this point ap points to the beginning of the first SDES item
  ap = p + 8;

  *ap++ = RTCP_SDES_CNAME;
  sprintf(tmp, "%s-%s/%d", loc_cfg.mycall.c_str(), loc_cfg.prefix.c_str(),
                           getPasswd(loc_cfg.mycall));
  addText(ap, tmp);

  *ap++ = RTCP_SDES_LOC;
  sprintf(tmp, ")EL-%.6s!%s0PHG%c%c%c%c/%06d/%03d%6s%02d%02d\r\n",
               loc_cfg.mycall.c_str(), pos,
               getPowerParam(loc_cfg.power),
               getHeightParam(loc_cfg.height),
               getGainParam(loc_cfg.gain),
               getDirectionParam(loc_cfg.beam_dir),
               loc_cfg.frequency, getToneParam(),
               info, utc.tm_hour, utc.tm_min);
  addText(ap, tmp);

  *ap++ = RTCP_SDES_END;
  *ap++ = 0;

    // Some data padding for alignment
  while ((ap - p) & 3)
  {
    *ap++ = 0;
  }

    // Length of all items
  len = ((ap - p) / 4) - 1;
  p[2] = len >> 8;
  p[3] = len;

    // Size of the entire packet
  return (ap - p);

} /* AprsUdpClient::buildSdesPacket */


// generate passcode for the aprs-servers, copied from xastir-source...
// special tnx to:
// Copyright (C) 1999,2000  Frank Giannandrea
// Copyright (C) 2000-2008  The Xastir Group
short AprsUdpClient::getPasswd(const string& call)
{
  short hash = HASH_KEY;
  string::size_type i = 0;
  string::size_type len = call.length();
  const char *ptr = call.c_str();

  while (i < len)
  {
    hash ^= toupper(*ptr++) << 8;
    hash ^= toupper(*ptr++);
    i += 2;
  }
  return (hash & 0x7fff);
} /* AprsUdpClient::passwd_hash */


int AprsUdpClient::getToneParam()
{
  return (loc_cfg.tone < 1000) ? loc_cfg.tone : 0;
} /* AprsUdpClient::getToneParam */


/*
 * This file has not been truncated
 */
