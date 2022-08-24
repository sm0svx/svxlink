/**
@file	 LocationInfo.h
@brief   Contains the infrastructure for APRS based EchoLink status updates
@author  Adi/DL1HRC and Steve/DH1DM
@date	 2009-03-12

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2015 Tobias Blomberg / SM0SVX

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
#include <vector>
#include <list>
#include <sys/time.h>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncConfig.h>
#include <EchoLinkStationData.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "AprsPty.h"


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
    static LocationInfo* instance()
    {
       if (!_instance)
        return NULL; // illegal pointer, if not initialized
       return _instance;
    }

    static bool has_instance()
    {
        return _instance;
    }

    static void deleteInstance(void)
    {
      delete _instance;
      _instance = 0;
    }

    struct Coordinate
    {
       explicit Coordinate(char d = 'N') : deg(0), min(0), sec(0), dir(d) {};

        unsigned int deg, min, sec;
        char dir;
    };

    std::string getCallsign();

    struct AprsStatistics
    {
      std::string logic_name;
      unsigned    rx_on_nr;
      unsigned    tx_on_nr;
      float       rx_sec;
      float       tx_sec;
      struct timeval last_rx_sec;
      struct timeval last_tx_sec;
      bool tx_on;
      bool squelch_on;

      AprsStatistics(void) : rx_on_nr(0), tx_on_nr(0), rx_sec(0), tx_sec(0),
                             last_rx_sec(), last_tx_sec(), tx_on(false),
                             squelch_on(false) {}
      void reset(void)
      {
        rx_on_nr = 0;
        tx_on_nr = 0;
        rx_sec = 0;
        tx_sec = 0;
        last_tx_sec.tv_sec = 0;
        last_rx_sec.tv_sec = 0;
        last_tx_sec.tv_usec = 0;
        last_rx_sec.tv_usec = 0;
      }
    };

    typedef std::map<std::string, AprsStatistics> aprs_struct;
    aprs_struct aprs_stats;

    struct Cfg
    {
      Cfg() : interval(600000), frequency(0), power(0), tone(0), height(10),
              gain(0), beam_dir(-1), range(0), range_unit('m'), lat_pos('N'),
              lon_pos('E') {};

      unsigned int interval;
      unsigned int frequency;
      unsigned int power;
      unsigned int tone;
      unsigned int height;
      unsigned int gain;
      int          beam_dir;
      unsigned int range;
      char         range_unit;

      Coordinate  lat_pos;
      Coordinate  lon_pos;

      std::string mycall;
      std::string prefix;
      std::string path;
      std::string comment;
    };

    static bool initialize(const Async::Config &cfg, const std::string &cfg_name);

    void updateDirectoryStatus(EchoLink::StationData::Status new_status);
    void igateMessage(const std::string& info);
    void update3rdState(const std::string& call, const std::string& info);
    void updateQsoStatus(int action, const std::string& call,
                         const std::string& name,
			 std::list<std::string>& call_list);
    bool getTransmitting(const std::string &name);
    void setTransmitting(const std::string &name, struct timeval tv, bool state);
    void setReceiving(const std::string &name, struct timeval tv, bool state);

  private:
    static LocationInfo* _instance;
    LocationInfo() : sequence(0), aprs_stats_timer(0), sinterval(0) {}
    LocationInfo(const LocationInfo&);
    ~LocationInfo(void) { delete aprs_stats_timer; };

    typedef std::list<AprsClient*> ClientList;

    Cfg         loc_cfg; // weshalb?
    ClientList  clients;
    int         sequence;
    Async::Timer *aprs_stats_timer;
    unsigned int sinterval;

    bool parsePosition(const Async::Config &cfg, const std::string &name);
    bool parseLatitude(Coordinate &pos, const std::string &value);
    bool parseLongitude(Coordinate &pos, const std::string &value);

    bool parseStationHW(const Async::Config &cfg, const std::string &name);
    bool parsePath(const Async::Config &cfg, const std::string &name);
    int calculateRange(const Cfg &cfg);
    bool parseAntennaHeight(Cfg &cfg, const std::string value);
    bool parseClientStr(std::string &host, int &port, const std::string &val);
    bool parseClients(const Async::Config &cfg, const std::string &name);
    void startStatisticsTimer(int interval);
    void sendAprsStatistics(Async::Timer *t);
    void initExtPty(std::string ptydevice);
    void mesReceived(std::string message);

};  /* class LocationInfo */

#endif /* LOCATION_INFO_INCLUDED */

/*
 * This file has not been truncated
 */
