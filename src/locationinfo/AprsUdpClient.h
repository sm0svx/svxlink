/**
@file	 AprsUdpClient.h
@brief   Contains an implementation of APRS updates via UDP
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


#ifndef APRS_UDP_CLIENT
#define APRS_UDP_CLIENT


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <string>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncUdpSocket.h>
#include <AsyncDnsLookup.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "LocationInfo.h"
#include "AprsClient.h"


/****************************************************************************
 *
 * Forward declarations
 *
 ****************************************************************************/

namespace Async
{
  class Config;
  class Timer;
  class DnsLookup;
};
namespace EchoLink
{
  class StationData;
};

/****************************************************************************
 *
 * Namespace
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Forward declarations of classes inside of the declared namespace
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Defines & typedefs
 *
 ****************************************************************************/


/****************************************************************************
 *
 * Exported Global Variables
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Class definitions
 *
 ****************************************************************************/

class AprsUdpClient : public AprsClient, public sigc::trackable
{
  public:
    static char getPowerParam(unsigned int power);
    static char getHeightParam(unsigned int height);
    static char getGainParam(unsigned int gain);
    static char getDirectionParam(int beam_dir);
    static std::string phgStr(unsigned int power, unsigned int height,
                              unsigned int gain, int beam_dir);

    AprsUdpClient(LocationInfo::Cfg &loc_cfg, const std::string &server,
                  int port);
    ~AprsUdpClient(void);

    void updateDirectoryStatus(EchoLink::StationData::Status status) override;
    void updateQsoStatus(int action, const std::string& call,
                         const std::string& info,
                         const std::list<std::string>& calls) override;
    void update3rdState(const std::string& call,
                        const std::string& info) override;

  private:
    LocationInfo::Cfg	&loc_cfg;
    std::string		server;
    int			port;
    Async::UdpSocket	sock;
    Async::IpAddress	ip_addr;
    Async::DnsLookup	*dns;
    Async::Timer        *beacon_timer;

    EchoLink::StationData::Status	curr_status;

    int			num_connected;
    std::string		curr_call;

    void  sendLocationInfo(Async::Timer *t = 0);
    void  dnsResultsReady(Async::DnsLookup &dns_lookup);

    int   buildSdesPacket(char *p);

    short getPasswd(const std::string& call);

    int   getToneParam();

};  /* class LocationInfoClient */


#endif /* APRS_UDP_CLIENT */

/*
 * This file has not been truncated
 */
