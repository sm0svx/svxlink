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


std::string dec2nmea_lat(float latitude)
{
  char lat[10];
  int minute = (latitude - int(latitude)) * 60;
  int second = (minute - int(minute)) * 100;
  sprintf(lat, "%02d%02d.%02d", int(latitude), minute, second);
  return std::string(lat);
} /* dec2nmea_lat */

std::string dec2nmea_lon(float longitude)
{
  char lon[10];
  int minute = (longitude - int(longitude)) * 60;
  int second = (minute - int(minute)) * 100;
  sprintf(lon, "%03d%02d.%02d", int(longitude), minute, second);
  return std::string(lon);
} /* dec2nmea_lon */

bool handle_LIP_compact(std::string lip, float & lat, float & lon)
{
  if (lip.length() < 18) return false;
  std::stringstream ss;
  long mlatl, mlonl;

  // calculate latitude
  std::string m_lat = lip.substr(7,6);
  ss << std::hex << m_lat;
  ss >> mlatl;
  mlatl *= 180;
  lat = mlatl / 16777216;
  ss.clear();

  // calculate longitude
  std::string m_lon = lip.substr(13,6);
  ss << std::hex << m_lon;
  ss >> mlonl;
  mlonl *= 360;
  lon = mlonl / 16777216;

 // lt1 = sprintf("%06.3fN",($lat - int($lat))*60);
 // lat = int($lat).$lt1;
  return true;
} /* handle_LIP_compact */


void handle_LIP_short(std::string in, float &lat, float &lon)
{
  lat = 0;
  lon = 0;
  long tla;
  long tlo;

  /* Protocol identifier
     0x03 = simple GPS
     0x0A = LIP
     0x83 = Complex SDS-TL GPS message transfer
  */
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
} /* handle_LIP_latlon */


/*
  Creates a sds from text, format of a command send to the Pei device must be:

  AT+CTSDS=RxISSI,len<0x0D><0x0A>
  message<0x1A>

  Where:
  RxISSI  - the ISSI (not TEI) of the destination MS
  len     - the length of the following message in byte
  message - the message as hex characters
*/
bool createSDS(std::string & sds, std::string issi, std::string message)
{
  if (message.length() > 120 || issi.length() > 8) return false;

  char len[4];
  std::stringstream ss;

  for (unsigned int a=0; a<message.length(); a++)
  {
    ss << std::hex << (int)message[a];
  }

  sds = "AT+CMGS=";
  sds += std::to_string(std::stoi(issi));
  sds += ",";

  sprintf(len, "%03d", (int)ss.str().length() * 4);
  std::string s(len);
  sds += s;
  sds += "\r\n";
  sds += ss.str();
  sds += 0x1a;

  return true;
} /* createSDS */


std::string decodeSDS(std::string hexSDS)
{
  std::string sds_text;
  unsigned int a;
  char byte[2];
  unsigned int x;
  std::stringstream ss;

  for (a=0; a < hexSDS.length(); a+=2)
  {
    ss << std::hex << hexSDS.substr(a,2);
    ss >> x;
    sprintf(byte, "%c", x);
    sds_text += byte;
    ss.clear();
  }
  return sds_text;
} /* decodeSDS */



//} /* namespace */

#endif /* TETRA_LIB_INCLUDED */



/*
 * This file has not been truncated
 */

