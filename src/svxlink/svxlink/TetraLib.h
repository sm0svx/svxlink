/**
@file	 TetraLib.h
@brief   Contains methods for Pei communication
@author  Adi Bier / DL1HRC
@date	 2020-05-28

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2021 Tobias Blomberg / SM0SVX

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
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <iomanip>
#include <math.h>


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

# define M_PI           3.14159265358979323846  /* pi */
# define RADIUS         6378.16                 /* Earth radius */

/****************************************************************************
 *
 * Exported Global Variables
 *
 ****************************************************************************/

/* 8.5.221 Transmission grant
   The transmission grant information element shall inform the MS/LS about 
   permission to transmit
*/
std::string TxGrant[] = {
   "0 - Transmission granted",
   "1 - Transmission not granted",
   "2 - Transmission queued",
   "3 - Transmission granted to another"
};

/* 6.17.8 Call status
   This parameter is used to indicate the status of either incoming or outgoing
   circuit mode call set up. The values are sent in D-Call Proceeding and 
   D-info on the air interface.
*/
std::string CallStatus[] = {
  "0 - Call progressing",
  "1 - Call queued",
  "2 - Called party paged",
  "3 - Call continue",
  "4 - Hang time expired"
};

/* 6.17.11 Called party identity type
   This parameter is used to indicate the type of identity to be used as the
   called party address. The associated identity used in signalling will be 
   interpreted differently according to this parameter. External subscriber 
   number addresses are used in association with a PSTN or PABX gateway.
*/
std::string CalledPartyIdentityType[] = {
  "0 - SSI",
  "1 - TSI",
  "2 - SNA (V+D only)",
  "3 - PABX external subscriber number (V+D or DMO if via a gateway)",
  "4 - PSTN external subscriber number (V+D or DMO if via a gateway)",
  "5 - Extended TSI"
};

/* 6.17.4 AI mode
   This parameter is used to indicate the mode of operation or the air 
   interface protocol stack.
*/
std::string AiMode[] = {
   "0 - V+D (trunked mode operation)",
   "1 - DMO",
   "2 - V+D with dual watch of DMO",
   "3 - DMO with dual watch of V+D",
   "4 - V+D and DMO (used in conjunction CTSP command)",
   "5 - NN",
   "6 - DMO Repeater mode"
};

/* 8.5.227 TX demand priority
   The TX demand priority information element shall inform the SwMI about 
   the importance of a TX-Demand
*/
std::string TxDemandPriority[] = {
   "0 - Low",
   "1 - High",
   "2 - Pre-emptive",
   "3 - Emergency"
};

/* 6.17.63 Transient communication type
   This parameter is used to indicate the communication type in either V+D 
   or DMO.
*/
std::string TransientComType[] = {
   "0 - Voice + Data",
   "1 - DMO-Direct MS-MS",
   "2 - DMO-Via DM-REP",
   "3 - DMO-Via DM-GATE",
   "4 - DMO-Via DM-REP/GATE",
   "5 - Reserved",
   "6 - Direct MS-MS, but maintain gateway registration"
};

/* 6.17.46 Reg stat
   This parameter is used to indicate the registration status of the MT to the
   TEI.
*/
std::string RegStat[] = {
   "0 - Registering or searching a network, one or more networks are available",
   "1 - Registered, home network",
   "2 - Not registered, no network currently available",
   "3 - System reject, no other network available",
   "4 - Unknown",
   "5 - Registered, visited network"
};

/* 6.17.3 AI Service
   This parameter is used to determine the type of service to be used in air 
   interface call set up signalling. The services are all defined in 
   EN 300 392-2 [3] or EN 300 396-3 [25].
*/
std::string AiService[] = {
   "0 - TETRA speech",
   "1 - 7,2 kbit/s unprotected data",
   "2 - Low protection 4,8 kbit/s short interleaving depth = 1",
   "3 - Low protection 4,8 kbit/s medium interleaving depth = 4",
   "4 - Low protection 4,8 kbit/s long interleaving depth = 8",
   "5 - High protection 2,4 kbit/s short interleaving depth = 1",
   "6 - High protection 2,4 kbit/s medium interleaving depth = 4",
   "7 - High protection 2,4 kbit/s high interleaving depth = 8",
   "8 - Packet Data (V+D only)",
   "9 - SDS type 1 (16 bits)",
   "10 - SDS type 2 (32 bits)",
   "11 - SDS type 3 (64 bits)",
   "12 - SDS type 4 (0 - 2 047 bits)",
   "13 - Status (16 bits, some values are reserved in EN 300 392-2 [3])"
};

/* 6.17.18 Disconnect cause
   This parameter is given in the disconnect message from the MT  when a 
   circuit-mode call is cleared by the other end, the SwMI, the gateway 
   (in the case of DMO) or the MT itself. The latter may have been requested
   by the TE. The TE could use the information in MMI or to initiate retries.
*/
std::string DisconnectCause[] = {
   "0 - Not defined or unknown",
   "1 - User request",
   "2 - Called party busy",
   "3 - Called party not reachable",
   "4 - Called party does not support encryption",
   "5 - Network congestion",
   "6 - Not allowed traffic",
   "7 - Incompatible traffic",
   "8 - Service not available",
   "9 - Pre-emption",
   "10 - Invalid call identifier",
   "11 - Called party rejection",
   "12 - No idle CC entity",
   "13 - Timer expiry",
   "14 - SwMI disconnect",
   "15 - No acknowledgement",
   "16 - Unknown TETRA identity",
   "17 - Supplementary Service dependent",
   "18 - Unknown external subscriber number",
   "19 - Call restoration failed",
   "20 - Called party requires encryption",
   "21 - Concurrent set-up not supported",
   "22 - Called party is under the same DM-GATE as the calling party",
   "23 - Reserved",
   "24 - Reserved",
   "25 - Reserved",
   "26 - Reserved",
   "27 - Reserved",
   "28 - Reserved",
   "29 - Reserved",
   "30 - Reserved",
   "31 - Called party offered unacceptable service",
   "32 - Pre-emption by late entering gateway",
   "33 - Link to DM-REP not established or failed",
   "34 - Link to gateway failed",
   "35 - Call rejected by gateway",
   "36 - V+D call set-up failure",
   "37 - V+D resource lost or call timer expired",
   "38 - Transmit authorization lost",
   "39 - Channel has become occupied by other users",
   "40 - Security parameter mismatch"
};

/* 6.17.20 DM communication type
   This parameter is used to indicate the communication type for outgoing
   calls in DMO, i.e. whether it is a direct communication between MSs, 
   or whether it is being routed via a DM-REP, DM-GATE or DM-REP/GATE.
*/
std::string DmCommunicationType[] = {
   "0 - Any, MT decides",
   "1 - Direct MS-MS",
   "2 - Via DM-REP",
   "3 - Via DM-GATE",
   "4 - Via DM-REP/GATE",
   "5 - Reserved",
   "6 - Direct MS-MS, but maintain gateway registration"
};

/* 6.17.38 Num type
   This parameter is used to indicate the type of identity returned by the
   +CNUM command. The service centre is used as (an optional) part of the 
   SDS-TL transport service protocol. The gateways returned are those used 
   in the SwMI for access to PSTN and PABX services.
*/
std::string NumType[] = {
   "0 - Individual (ISSI or ITSI)",
   "1 - Group (GSSI or GTSI)",
   "2 - PSTN Gateway (ISSI or ITSI)",
   "3 - PABX Gateway (ISSI or ITSI)",
   "4 - Service Centre (ISSI or ITSI)",
   "5 - Service Centre (E.164 number)",
   "6 - Individual (extended TSI)",
   "7 - Group (extended TSI)"
};

/* 6.3.64 ETSI TS 100 392-18-1 V1.3.1 (2007-04)
   Reason for sending
   The reason for sending shall indicate the reason why location information 
   was sent from the location determination entity
*/
std::string ReasonForSending[] = {
  "0 - Subscriber unit is powered ON",
  "1 - Subscriber unit is powered OFF",
  "2 - Emergency condition is detected",
  "3 - Push-to-talk condition is detected",
  "4 - Status",
  "5 - Transmit inhibit mode ON",
  "6 - Transmit inhibit mode OFF",
  "7 - System access (TMO ON)",
  "8 - DMO ON",
  "9 - Enter service (after being out of service)",
  "10 - Service loss",
  "11 - Cell reselection or change of serving cell",
  "12 - Low battery",
  "13 - Subscriber unit is connected to a car kit",
  "14 - Subscriber unit is disconnected from a car kit",
  "15 - Subscriber unit asks for transfer initialization configuration",
  "16 - Arrival at destination",
  "17 - Arrival at a defined location",
  "18 - Approaching a defined location",
  "19 - SDS type-1 entered",
  "20 - User application initiated",
  "21 - Reserved"
};

/* 6.17.25 Group type
   This parameter is used when setting the MT groups for use in V+D. A selected
   group will be used for outgoing calls. Either selected or scanned groups 
   will receive incoming calls. Only incoming group calls with a priority higher
   than the scan level will interrupt ongoing group calls of a lower level.
   If the group type is "none" all groups will be detached from the SwMI.
*/
std::string GroupType[] = {
  "0 - None",
  "1 - Select",
  "2 - Scan priority 1",
  "3 - Scan priority 2",
  "4 - Scan priority 3",
  "5 - Scan priority 4",
  "6 - Scan priority 5",
  "7 - Scan priority 6"
};

/* 6.17.51 SDS Status
   This parameter is used to indicate the status of outgoing and incoming SDS 
   messages.
*/
std::string sdsStatus[] {
  "0 - Incoming message stored and unread",
  "1 - Incoming message stored and read",
  "2 - Outgoing message stored and unsent",
  "3 - Outgoing message stored and sent"
};

std::string GrUnsolic[] {

};

// contain decoded data of a lip sds
struct LipInfo {
  int time_elapsed;
  float longitude;
  float latitude;
  int positionerror;
  float horizontalvelocity;
  float directionoftravel;
  short reasonforsending;
};

unsigned short msgcnt;

/****************************************************************************
 *
 * Class definitions
 *
 ****************************************************************************/

std::string dec2nmea_lat(float latitude)
{
  char lat[10];
  float degrees;
  float normalizedLat = std::fmod(latitude, 180.0f);
  float fractpart = std::modf(std::abs(normalizedLat), &degrees);
  float minute = fractpart * 60.0;
  char dir = normalizedLat > 0.0 ? 'N' : 'S';
  sprintf(lat, "%02.0f%05.2f%c", degrees, minute, dir);
  return std::string(lat);;
} /* dec2nmea_lat */


std::string dec2nmea_lon(float longitude)
{
  char lon[10];
  float degrees;
  float normalizedLon = std::fmod(longitude, 360.0f);
  float fractpart = std::modf(std::abs(normalizedLon), &degrees);
  float minute = fractpart * 60.0;
  char dir = normalizedLon > 0.0 ? 'E' : 'W';
  sprintf(lon, "%03.0f%05.2f%c", degrees, minute, dir);
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
  // 0A06B8ACA67F1822FFE810
  std::string m_lon = lip.substr(13,6);
  ss << std::hex << m_lon;
  ss >> mlonl;
  mlonl *= 360;
  lon = mlonl / 16777216;

 // lt1 = sprintf("%06.3fN",($lat - int($lat))*60);
 // lat = int($lat).$lt1;
  return true;
} /* handle_LIP_compact */


void handleLipSds(std::string in, LipInfo &lipinfo)
{
  /* Protocol identifier
     0x02 = Simple Text Messaging
     0x03 = Simple location system
     0x06 = M-DMO (Managed DMO)
     0x09 = Simple immediate text messaging
     0x0A = LIP (Location Information Protocol)
     0x0C = Concatenated SDS message
     0x82 = Text Messaging
     0x83 = Complex SDS-TL GPS message transfer
  */
  double tla;
  double tlo;

  int t_velo;
  int t_dot;

  /* 0A0088BD4247F737FFE810 - short position report PDU
     0A4E73DDA841F55809493CC081 - long position report PDU

     There is a small problem with the PEI answer, the length of
     SDS is specified as 84 bits:
     +CTSDSR: 12,2269001,0,9999,0,84
     0A112853A9FF4D4FFFE810 <- 22 digits
     but 22 chars are 88 bits long, so the output in the first part
     of the PEI response is incorrect (84 != 88). Could it well be
     a cps problem in the Motorola MS?

     0A112853A9FF4D4FFFE810 - from YO9ION (Motorola MTP6650+MTM800E)

     In N5UWU's PEI response, the last character (0) is missing and the
     length of 21 chars corresponds to the specified bit length of 84 bits.

     0A0BA7D5B95BC50AFFE16 - from N5UWU (Sepura SEG3900 and STP9240)
  */
  if (in.substr(0,2) == "0A") // LIP
  {
    // check that is a shor position report
    if ((std::stoi(in.substr(2,1),nullptr,16) & 0x0c) != 0)
    {
       std::cout << "*** ERROR: Only PDU type=0 supported at the moment,"
        << " check that you have configured \"short location report PDU\""
        << " in your codeplug." << std::endl;
       return;
    }

    lipinfo.time_elapsed = (std::stoi(in.substr(2,1),nullptr,16) & 0x03);

    tlo =  std::stol(in.substr(3,1),nullptr,16) << 21;
    tlo += std::stol(in.substr(4,1),nullptr,16) << 17;
    tlo += std::stol(in.substr(5,1),nullptr,16) << 13;
    tlo += std::stol(in.substr(6,1),nullptr,16) << 9;
    tlo += std::stol(in.substr(7,1),nullptr,16) << 5;
    tlo += std::stol(in.substr(8,1),nullptr,16) << 1;
    tlo += (std::stol(in.substr(9,1),nullptr,16) & 0x08) >> 3;

    tla = (std::stol(in.substr(9,1),nullptr,16) & 0x07) << 21;
    tla += std::stol(in.substr(10,1),nullptr,16) << 17;
    tla += std::stol(in.substr(11,1),nullptr,16) << 13;
    tla += std::stol(in.substr(12,1),nullptr,16) << 9;
    tla += std::stol(in.substr(13,1),nullptr,16) << 5;
    tla += std::stol(in.substr(14,1),nullptr,16) << 1;
    tla += (std::stol(in.substr(15,1),nullptr,16) & 0x08) >> 3;

    if (tlo > 16777216)
    {
      lipinfo.longitude = tlo * 360.0 / 33554432 - 360.0;
    }
    else
    {
      lipinfo.longitude = tlo * 360.0 / 33554432;
    }

    if (tla > 8388608)
    {
      lipinfo.latitude = tla * 360.0 / 33554432 - 360.0;
    }
    else
    {
      lipinfo.latitude = tla * 360.0 / 33554432;
    }

    // position error in meter
    lipinfo.positionerror = 2*pow(10,(std::stoi(in.substr(15,1),nullptr,16) & 0x03));

    /*
      Horizontal velocity shall be encoded for speeds 0 km/h to 28 km/h in 1 km/h
      steps and from 28 km/h onwards using equation:
      v = C × (1 + x)^(K-A) + B where:
        • C = 16
        • x = 0,038
        • A = 13
        • K = Horizontal velocity information element value
        • B = 0
    */
    t_velo  = std::stoi(in.substr(16,1),nullptr,16) << 3;
    t_velo += (std::stoi(in.substr(17,1),nullptr,16) & 0x0e) >> 1;
    if (t_velo < 29)
    {
      lipinfo.horizontalvelocity = t_velo;
    }
    else
    {
      lipinfo.horizontalvelocity =  16 * pow(1.038, (t_velo - 13));
    }

    /*
     definition can be expressed also by equation:
     Direction of travel value = trunc((direction + 11,25)/22,5), when
     direction is given in degrees.
    */
    t_dot = (std::stoi(in.substr(17,1),nullptr,16) & 0x01) << 3;
    t_dot += (std::stoi(in.substr(18,1),nullptr,16) & 0x0e) >> 1;
    lipinfo.directionoftravel = t_dot * 22.5;

    /*
    reason for sending
    */
    lipinfo.reasonforsending = std::stoi(in.substr(19,1),nullptr, 16);

  }
  // (NMEA) 0183 over SDS-TL
  // $GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47
  else if (in.substr(0,2) == "03")
  {
    // to do
  }
} /* handleLipSds */


unsigned short getSdsMesgId(void)
{
  if (++msgcnt > 255) msgcnt=1;
  return msgcnt;
} /* getSdsMesgId */


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
  if (message.length() > 120 || issi.length() > 8 || issi.length() == 0)
  {
    return false;
  }
  
  std::stringstream ss;
  ss << "8204" << std::setfill('0') << std::setw(sizeof(short))
     << std::hex << getSdsMesgId() << "01";

  for (unsigned int a=0; a<message.length(); a++)
  {
    ss << std::hex << (int)message[a];
  }

  char f[ss.str().length() + issi.length() + 20];
  sprintf(f, "AT+CMGS=%s,%03d\r\n%s%c",
             std::to_string(std::stoi(issi)).c_str(),
             (int)ss.str().length() * 4,
             ss.str().c_str(), 0x1a);
  sds = f;
  return true;
} /* createSDS */


bool createRawSDS(std::string & sds, std::string issi, std::string message)
{
  if (message.length() > 220 || issi.length() > 8) return false;
  char f[message.length() + issi.length() + 20];
  sprintf(f, "AT+CMGS=%s,%03d\r\n%s%c",
             std::to_string(std::stoi(issi)).c_str(),
             (int)message.length() * 4,
             message.c_str(), 0x1a);
  sds = f;
  return true;
} /* createRawSDS */


bool createCfmSDS(std::string & sds, std::string issi, std::string msg)
{
  char f[issi.length()+20 + msg.length()];
  sprintf(f, "AT+CMGS=%s,%03d\r\n%s%c",
             std::to_string(std::stoi(issi)).c_str(),
             (int)msg.length() * 4,
             msg.c_str(), 0x1a);
  sds = f;
  return true;
} /* createCfmSDS */


bool createStateSDS(std::string & sds, std::string issi)
{
  if (issi.length() > 8) return false;
  std::stringstream ss;
  ss << "821000" << std::setfill('0') << std::setw(sizeof(short))
     << std::hex << getSdsMesgId();

  char f[issi.length() + ss.str().length()+20];
  sprintf(f, "AT+CMGS=%s,%03d\r\n%s%c",
             std::to_string(std::stoi(issi)).c_str(),
             (int)ss.str().length() * 4,
             ss.str().c_str(), 0x1a);
  sds = f;
  return true;
} /* createStateSDS */


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


float radians(float degrees)
{
  return (degrees * M_PI) / 180.0;
} /* radians */


float degrees(float radians)
{
  return (radians * 180.0) / M_PI;
} /* degrees */


float calcDistance(float lat1, float lon1, float lat2, float lon2)
{

  double dlon = M_PI * (lon2 - lon1) / 180.0;
  double dlat = M_PI * (lat2 - lat1) / 180.0;

  double a = (sin(dlat / 2) * sin(dlat / 2)) + cos(M_PI*lat1/180.0) *
              cos(M_PI*lat2/180) * (sin(dlon / 2) * sin(dlon / 2));
  double angle = 2 * atan2(sqrt(a), sqrt(1 - a));
  return static_cast<float>(static_cast<int>(angle * RADIUS * 100.))/100.;
} /* calcDistance */


float calcBearing(float lat1, float lon1, float lat2, float lon2)
{
  float teta1 = radians(lat1);
  float teta2 = radians(lat2);
  float delta2 = radians(lon2-lon1);

  float y = sin(delta2) * cos(teta2);
  float x = cos(teta1)*sin(teta2) - sin(teta1)*cos(teta2)*cos(delta2);
  float br = fmod(degrees(atan2(y,x)) + 360., 360.);
  return static_cast<float>(static_cast<int>(br * 10.))/10.;
} /* calcBearing */


float getDecimalDegree(LocationInfo::Coordinate pos)
{
  float degree = 0.0;
  degree = static_cast<float>(pos.deg + (pos.min + (pos.sec/60.0))/60.0);
  return degree;

} /* getDecimalDegree */


// return the ISSI as part of the TEI
std::string getISSI(std::string tsi)
{
  std::stringstream t_issi;
  size_t len = tsi.length();
  
  if (len < 8)
  {
    t_issi << "00000000" << tsi;
    return t_issi.str().substr(t_issi.str().length()-8, 8);
  }
  t_issi << tsi.substr(len-8, 8);
  return t_issi.str();
} /* getISSI */


bool splitTsi(std::string tsi, int &mcc, int &mnc, int &issi)
{
  bool ret = false;
  size_t len = tsi.length();

  if (len < 9)
  {
    issi = atoi(tsi.c_str());
    mcc = 0;
    mnc = 0;
    ret = true;
  }
  else
  {
    issi = atoi(tsi.substr(len-8,8).c_str());
    std::string t = tsi.substr(0, len-8);

    if (t.length() == 7)
    {
      mcc = atoi(t.substr(0,3).c_str());
      mnc = atoi(t.substr(3,4).c_str());
      ret = true;
    }
    else if (t.length() == 8)
    {
      mcc = atoi(t.substr(0,3).c_str());
      mnc = atoi(t.substr(3,5).c_str());
      ret = true;
    }
    else if (t.length() == 9)
    {
      mcc = atoi(t.substr(0,4).c_str());
      mnc = atoi(t.substr(4,5).c_str());
      ret = true;
    }
    else ret = false;
  }
  return ret;  
} /* splitTsi */


unsigned int hex2int(std::string sds)
{
  unsigned int t;
  std::stringstream ss;
  ss << std::hex << sds;
  ss >> t;
  return t;
} /* hex2int */


int getNextVal(std::string& h)
{
  size_t f;
  int t = atoi(h.substr(0, f = h.find(',')).c_str());
  if (f != std::string::npos)
  {
    h.erase(0, f + 1);
  }
  return t;
} /* getNextVal */


std::string getNextStr(std::string& h)
{
  size_t f;
  std::string t = h.substr(0, f = h.find(','));
  if (f != std::string::npos)
  {
    h.erase(0, f + 1);    
  }
  return t;
} /* getNextStr */


std::string getPeiError(int errorcode)
{

  if (errorcode > 45)
  {
    std::stringstream ss;
    ss << "Unknown Error occured (" << errorcode << ")" << std::endl;
    return ss.str();
  }
std::string error[] = {
"0 - The MT was unable to send the data over the air (e.g. to the SwMI)",
"1 - The MT can not establish a reliable communication with the TE",
"2 - The PEI link of the MT is being used already",
"3 - This is a general error report code which indicates that the MT supports\n \
the command but not in its current state. This code shall be used when no\n \
other code is more appropriate for the specific context",
"4 - The MT does not support the command",
"5 - The MT can not process any command until the PIN for the SIM is provided",
"6 - Reserved",
"7 - Reserved",
"8 - Reserved",
"9 - Reserved",
"10 - The MT can not process the command due to the absence of a SIM",
"11 - The SIM PIN1 is required for the MT to execute the command",
"12 - MMI unblocking of the SIM PIN1 is required",
"13 - The MT failed to access the SIM",
"14 - The MT can not currently execute the command due to the SIM not being\n \
ready to proceed",
"15 - The MT does not recognize this SIM",
"16 - The entered PIN for the SIM is incorrect",
"17 - The SIM PIN2 is required for the MT to execute the command",
"18 - MMI unblocking of the SIM PIN2 is required",
"19 - Reserved",
"20 - The MT message stack is full",
"21 - The requested message index in the message stack does not exist",
"22 - The requested message index does not correspond to any message",
"23 - The MT failed to store or access to its message stack",
"24 - The text string associated with a status value is too long",
"25 - The text string associated with a status value contains invalid characters",
"26 - The <dial string> is longer than 25 digits",
"27 - The <dial string> contains invalid characters",
"28 - Reserved",
"29 - Reserved",
"30 - The MS is currently out of service and can not process the command",
"31 - The MT did not receive any Layer 2 acknowledgement from the SwMI",
"32 - <user data> decoding failed",
"33 - At least one of the parameters is of the wrong type e.g. string instead\n \
of number or vice-versa",
"34 - At least one of the supported parameters in the command is out of range",
"35 - Syntax error. The syntax of the command is incorrect e.g. mandatory\n \
parameters are missing or are exceeding Data received without command",
"36 - The MT received <user data> without AT+CMGS= ...<CR>",
"37 - AT+CMGS command received, but timeout expired waiting for <userdata>",
"38 - The TE has already registered the Protocol Identifier with the MT",
"39 - Registration table in SDS-TL is full. The MT can no longer register\n \
a new Protocol Identifier until a registered Protocol identifier is\n \
deregistered",
"40 - The MT supports the requested service but not while it is in DMO",
"41 - The MT is in Transmit inhibit mode and is not able to process the\n \
command in this state",
"42 - The MT is involved in a signalling activity and is not able to process\n \
the available command until the current transaction ends. In V+D,\n \
the signalling activity could be e.g. group attachment, group report, SDS\n \
processing, processing of DGNA, registration, authentication or any\n \
transaction requiring a response from the MS or the SwMI. In DMO, the\n \
signalling activity could be e.g. Call or SDS processing.",
"43 - The MT supports the requested service but not while it is in V+D",
"44 - The MT supports handling of unknown parameters",
"45 - Reserved"
};

return error[errorcode];

} /* getPeiError */


//} /* namespace */

#endif /* TETRA_LIB_INCLUDED */



/*
 * This file has not been truncated
 */

