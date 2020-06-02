/**
@file	 TetraLib.h
@brief   Contains methods for Pei communication
@author  Adi Bier / DL1HRC
@date	 2020-05-28

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2020 Tobias Blomberg / SM0SVX

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


#ifndef TETRA_LIB_INCLUDED
#define TETRA_LIB_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <string>
#include <sstream>
#include <iostream>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/




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



/****************************************************************************
 *
 * Namespace
 *
 ****************************************************************************/

//namespace MyNameSpace
//{


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

/**
@brief	This class provide some methods for handling Pei messages and
        calculations
@author Adi Bier / DL1HRC
@date   2020-05-28
*/
class TetraLib
{
  public:

  std::string dec2nmea_lat(float latitude)
  {
    char lat[10];
    int minute = (latitude - int(latitude)) * 60;
    int second = (minute - int(minute)) * 100;
    sprintf(lat, "%02d%02d.%02d", int(latitude), minute, second);
    return std::string(lat);
  }

  std::string dec2nmea_lon(float longitude)
  {
    char lon[10];
    int minute = (longitude - int(longitude)) * 60;
    int second = (minute - int(minute)) * 100;
    sprintf(lon, "%03d%02d.%02d", int(longitude), minute, second);
    return std::string(lon);
  }

  void get_LIP_latlon(std::string in, float &lat, float &lon)
  {
    lat = 0;
    lon = 0;
    long tla;
    long tlo;
    
    /* Protocol identifier
       0x03 = simple GPS
       0x0A = LIP
       0x83 = Complex SDS-TL GPS message transfer */
 
    if (in.substr(0,2) == "0A") 
    {
      tlo = std::stol(in.substr(2,1),nullptr,16) << 25;
      tlo +=std::stol(in.substr(3,1),nullptr,16) << 21;
      tlo +=std::stol(in.substr(4,1),nullptr,16) << 17;
      tlo +=std::stol(in.substr(5,1),nullptr,16) << 13;
      tlo +=std::stol(in.substr(6,1),nullptr,16) << 9;
      tlo +=std::stol(in.substr(7,1),nullptr,16) << 5;
      tlo +=(std::stol(in.substr(8,1),nullptr,16) & 0x08) << 1;

      tla =  (std::stol(in.substr(9,1),nullptr,16) & 0x7) << 22;
      tla += std::stol(in.substr(10,1),nullptr,16) << 18;
      tla += std::stol(in.substr(11,1),nullptr,16) << 14;
      tla += std::stol(in.substr(12,1),nullptr,16) << 10;
      tla += std::stol(in.substr(13,1),nullptr,16) << 6;
      tla += std::stol(in.substr(14,1),nullptr,16) << 2;
      tla += (std::stol(in.substr(15,1),nullptr,16) & 0x0C) >> 2;
      
      lat = tla*180/33554432;
      lon = tlo*360/33554432;
    }
  }
  
  protected:

  private:


};  /* class TetraLib */


//} /* namespace */

#endif /* TETRA_LIB_INCLUDED */



/*
 * This file has not been truncated
 */

