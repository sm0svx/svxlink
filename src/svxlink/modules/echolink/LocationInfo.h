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


#ifndef LOCATION_INFO
#define LOCATION_INFO


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
    typedef struct
    {
      int deg, min, sec;
      char dir;
    } Coordinate;

    typedef struct
    {
      int         interval;
      int         frequency;
      int         power;
      int         tone;
      int         height;
      int         gain;
      int         beam_dir;
      int         range;
      char        range_unit;

      Coordinate  lat_pos;
      Coordinate  lon_pos;    

      std::string mycall;
      std::string path;
      std::string comment;
    } Cfg;

    LocationInfo(Async::Config &cfg, const std::string &name,
                 const std::string &callsign);
    ~LocationInfo(void);

    void updateDirectoryStatus(EchoLink::StationData::Status new_status);
    void updateQsoStatus(int action, const std::string& call,
      const std::string& name, std::list<std::string>& call_list);

  private:
    typedef std::list<AprsClient*>    ClientList;

    Cfg         loc_cfg;
    ClientList  clients;

    bool parseLatitude(Coordinate &lat_pos, const std::string &value);
    bool parseLongitude(Coordinate &lon_pos, const std::string &value);

};  /* class LocationInfo */


#endif /* LOCATION_INFO */

/*
 * This file has not been truncated
 */
