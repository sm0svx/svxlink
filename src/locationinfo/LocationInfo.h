/**
@file	 LocationInfo.h
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
       static CGuard g;
       if (!_instance)
        return NULL; // illegal pointer, if not initialized
       return _instance;
    }

    static bool has_instance()
    {
        return _instance;
    }

    struct Coordinate
    {
       explicit Coordinate(char d = 'N') : deg(0), min(0), sec(0), dir(d) {};

        unsigned int deg, min, sec;
        char dir;
    };

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
    void update3rdState(const std::string& call, const std::string& info);
    void updateQsoStatus(int action, const std::string& call,
                         const std::string& name,
			 std::list<std::string>& call_list);

    class CGuard
    {
       public:
         ~CGuard()
         {
           if(NULL != LocationInfo::_instance)
           {
              delete LocationInfo::_instance;
              LocationInfo::_instance = NULL;
           }
         }
    };
    friend class CGuard;

  private:
    static LocationInfo* _instance;
    LocationInfo() {};
    LocationInfo(const LocationInfo&);
    ~LocationInfo(void) {};

    typedef std::list<AprsClient*> ClientList;

    Cfg         loc_cfg; // weshalb?
    ClientList  clients;

    bool parsePosition(const Async::Config &cfg, const std::string &name);
    bool parseLatitude(Coordinate &pos, const std::string &value);
    bool parseLongitude(Coordinate &pos, const std::string &value);

    bool parseStationHW(const Async::Config &cfg, const std::string &name);
    bool parsePath(const Async::Config &cfg, const std::string &name);
    int calculateRange(const Cfg &cfg);
    bool parseAntennaHeight(Cfg &cfg, const std::string value);
    bool parseClientStr(std::string &host, int &port, const std::string &val);
    bool parseClients(const Async::Config &cfg, const std::string &name);

};  /* class LocationInfo */

#endif /* LOCATION_INFO_INCLUDED */

/*
 * This file has not been truncated
 */
