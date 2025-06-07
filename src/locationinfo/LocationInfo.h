/**
@file	 LocationInfo.h
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


#ifndef LOCATION_INFO_INCLUDED
#define LOCATION_INFO_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <string>
#include <list>
#include <chrono>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncConfig.h>
#include <AsyncTimer.h>
#include <AsyncPty.h>
#include <EchoLinkStationData.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Forward declarations
 *
 ****************************************************************************/

class AprsClient;


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
 * Exported Global Types
 *
 ****************************************************************************/


/****************************************************************************
 *
 * Class definitions
 *
 ****************************************************************************/

class LocationInfo
{
  public:
    using Clock     = std::chrono::steady_clock;
    using Timepoint = Clock::time_point;

    struct Coordinate
    {
      unsigned int  deg {0};
      unsigned int  min {0};
      unsigned int  sec {0};
      char          dir {'N'};
    };

    struct Cfg
    {
      unsigned int binterval    {10}; // Beacon interval in minutes
      unsigned int frequency    {0};
      std::string  freq_sep     {"/"};
      unsigned int power        {0};
      unsigned int tone         {0};
      unsigned int height       {10};
      unsigned int gain         {0};
      int          beam_dir     {-1};
      unsigned int range        {0};
      char         range_unit   {'m'};

      Coordinate  lat_pos;
      Coordinate  lon_pos;

      std::string objectname;
      std::string sourcecall;
      std::string logincall;
      std::string loginssid;
      std::string statscall;
      std::string mycall;
      std::string prefix;
      std::string path          {"TCPIP*"};
      std::string comment       {"SvxLink Node"};
      std::string destination   {"APSVX1"};
      bool        debug         {false};
      std::string filter;
      std::string symbol        {"S0"};
      int         tx_offset_khz {10000};
      bool        narrow        {false};
    };

    static LocationInfo* instance(void)
    {
      return _instance;
    }

    static bool has_instance(void)
    {
      return _instance != nullptr;
    }

    static void deleteInstance(void)
    {
      delete _instance;
      _instance = 0;
    }

    static bool initialize(Async::Config& cfg, const std::string& cfg_name);

    LocationInfo(void);
    LocationInfo(const LocationInfo&) = delete;
    ~LocationInfo(void);

    void updateDirectoryStatus(EchoLink::StationData::Status new_status);
    void igateMessage(const std::string& info);
    void update3rdState(const std::string& call, const std::string& info);
    void updateQsoStatus(int action, const std::string& call,
                         const std::string& name,
                         const std::list<std::string>& calls);
    bool getTransmitting(const std::string &name);
    void setTransmitting(const std::string& name, bool is_transmitting,
                         Timepoint tp=Clock::now());
    void setReceiving(const std::string& name, bool is_receiving,
                      const Timepoint& tp=Clock::now());

  private:
    static LocationInfo* _instance;

    using ClientList = std::list<AprsClient*>;
    using Duration  = std::chrono::duration<double>;
    struct AprsStatistics
    {
      unsigned    rx_on_nr        {0};
      unsigned    tx_on_nr        {0};
      Duration    rx_sec          {0};
      Duration    tx_sec          {0};
      Timepoint   last_rx_tp;
      Timepoint   last_tx_tp;
      bool        is_transmitting {false};
      bool        is_receiving    {false};

      void reset(void)
      {
        rx_on_nr = 0;
        tx_on_nr = 0;
        rx_sec = Duration::zero();
        tx_sec = Duration::zero();
      }
    };
    using AprsStatsMap = std::map<std::string, AprsStatistics>;

    Cfg           loc_cfg; // weshalb?
    ClientList    clients;
    int           sequence          {0};
    Async::Timer  aprs_stats_timer  {-1, Async::Timer::TYPE_PERIODIC};
    unsigned int  sinterval         {10}; // Minutes
    std::string   slogic;
    Timepoint     last_tlm_metadata {-std::chrono::hours(1)};
    AprsStatsMap  aprs_stats;
    Async::Pty*   aprspty           {nullptr};

    bool parsePosition(const Async::Config &cfg, const std::string &name);
    bool parseLatitude(Coordinate &pos, const std::string &value);
    bool parseLongitude(Coordinate &pos, const std::string &value);

    bool parseStationHW(const Async::Config &cfg, const std::string &name);
    bool parsePath(const Async::Config &cfg, const std::string &name);
    int calculateRange(const Cfg &cfg);
    bool parseAntennaHeight(Cfg &cfg, const std::string value);
    bool parseClientStr(std::string &host, int &port, const std::string &val);
    bool parseClients(const Async::Config &cfg, const std::string &name);
    void startStatisticsTimer(int sinterval);
    void sendAprsStatistics(void);
    void initExtPty(std::string ptydevice);
    void mesReceived(const void* buf, size_t len);
    AprsStatistics& aprsStats(const std::string& logic_name);

};  /* class LocationInfo */

#endif /* LOCATION_INFO_INCLUDED */

/*
 * This file has not been truncated
 */
