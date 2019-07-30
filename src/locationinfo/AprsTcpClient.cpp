/**
@file	 AprsTcpClient.cpp
@brief   Contains an implementation of APRS updates via TCP
@author  Adi Bier / DL1HRC
@date	 2008-12-01

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2008 Tobias Blomberg / SM0SVX

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
#include <cmath>
#include <cstring>
#include <sys/time.h>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncTimer.h>
#include <AsyncConfig.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "version/SVXLINK.h"
#include "AprsTcpClient.h"
#include "common.h"


/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/

using namespace std;
using namespace Async;
using namespace SvxLink;


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
 * Prototypes
 *
 ****************************************************************************/



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

#define HASH_KEY	0x73e2 			// This is the seed for the key


/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/

AprsTcpClient::AprsTcpClient(LocationInfo::Cfg &loc_cfg,
                            const std::string &server, int port)
  : loc_cfg(loc_cfg), server(server), port(port), con(0), beacon_timer(0),
    reconnect_timer(0), offset_timer(0), num_connected(0)
{
   StrList str_list;

   destination = "APRS";

   el_call = loc_cfg.mycall;  // the EchoLink callsign
   el_prefix = "E" + loc_cfg.prefix + "-"; // the EchoLink prefix ER- od EL-

   con = new TcpClient<>(server, port);
   con->connected.connect(mem_fun(*this, &AprsTcpClient::tcpConnected));
   con->disconnected.connect(mem_fun(*this, &AprsTcpClient::tcpDisconnected));
   con->dataReceived.connect(mem_fun(*this, &AprsTcpClient::tcpDataReceived));
   con->connect();

   beacon_timer = new Timer(loc_cfg.interval, Timer::TYPE_PERIODIC);
   beacon_timer->setEnable(false);
   beacon_timer->expired.connect(mem_fun(*this, &AprsTcpClient::sendAprsBeacon));

   offset_timer = new Timer(10000, Timer::TYPE_ONESHOT);
   offset_timer->setEnable(false);
   offset_timer->expired.connect(mem_fun(*this,
                 &AprsTcpClient::startNormalSequence));

   reconnect_timer = new Timer(5000);
   reconnect_timer->setEnable(false);
   reconnect_timer->expired.connect(mem_fun(*this,
                 &AprsTcpClient::reconnectAprsServer));
} /* AprsTcpClient::AprsTcpClient */


AprsTcpClient::~AprsTcpClient(void)
{
   delete con;
   delete reconnect_timer;
   delete offset_timer;
   delete beacon_timer;
} /* AprsTcpClient::~AprsTcpClient */


void AprsTcpClient::updateQsoStatus(int action, const string& call,
  const string& info, list<string>& call_list)
{
  num_connected = call_list.size();

  char msg[80];
  switch(action)
  {
    case 0:
      sprintf(msg, "connection to %s closed", call.c_str());
      break;
    case 1:
      sprintf(msg, "connection to %s (%s)", call.c_str(), info.c_str());
      break;
    case 2:
      sprintf(msg, "incoming connection %s (%s)", call.c_str(), info.c_str());
      break;
  }

  // Format for "object from..."
  // DL1HRC>;EL-242660*111111z4900.05NE00823.29E0434.687MHz T123 R10k   DA0AAA

    // Geographic position
  char pos[128];
  posStr(pos);

    // APRS message
  char aprsmsg[256];
  sprintf(aprsmsg, "%s>%s,%s:;%s%-6.6s*111111z%s%s\r\n",
          el_call.c_str(), destination.c_str(), loc_cfg.path.c_str(),
          el_prefix.c_str(), el_call.c_str(), pos, msg);
  sendMsg(aprsmsg);

  // APRS status message, connected calls
  string status = el_prefix + el_call+">"+destination+","+loc_cfg.path+":>";

  list<string>::const_iterator it;
  for (it = call_list.begin(); it != call_list.end(); ++it)
  {
    status += *it + " ";
  }
  status += "\r\n";
  sendMsg(status.c_str());

} /* AprsTcpClient::updateQsoStatus */


// updates state of a 3rd party
void AprsTcpClient::update3rdState(const string& call, const string& info)
{
   char aprsmsg[200];
   sprintf(aprsmsg, "%s>%s\n\r", call.c_str(), info.c_str());
   sendMsg(aprsmsg);
} /* AprsTcpClient::update3rdState */


void AprsTcpClient::igateMessage(const string& info)
{
  sendMsg(info.c_str());
} /* AprsTcpClient::igateMessage */



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

void AprsTcpClient::posStr(char *pos)
{
  char num_connected_overlay;
  if (num_connected > 0)
  {
    num_connected_overlay = (num_connected < 10) ? '0' + num_connected : '9';
  }
  else
  {
    num_connected_overlay = 'E';
  }
  sprintf(pos, "%02d%02d.%02d%c%c%03d%02d.%02d%c0",
               loc_cfg.lat_pos.deg, loc_cfg.lat_pos.min,
               (loc_cfg.lat_pos.sec * 100) / 60, loc_cfg.lat_pos.dir,
               num_connected_overlay,
               loc_cfg.lon_pos.deg, loc_cfg.lon_pos.min,
               (loc_cfg.lon_pos.sec * 100) / 60, loc_cfg.lon_pos.dir);
}


void AprsTcpClient::sendAprsBeacon(Timer *t)
{
    // Geographic position
  char pos[128];
  posStr(pos);

    // CTCSS/1750Hz tone
  char tone[5];
  sprintf(tone, (loc_cfg.tone < 1000) ? "T%03d" : "%04d", loc_cfg.tone);

    // APRS message
  char aprsmsg[200];
  sprintf(aprsmsg, "%s>%s,%s:;%s%-6.6s*111111z%s%03d.%03dMHz %s R%02d%c %s\r\n",
            el_call.c_str(), destination.c_str(), loc_cfg.path.c_str(),
            el_prefix.c_str(), el_call.c_str(), pos, loc_cfg.frequency / 1000,
            loc_cfg.frequency % 1000, tone, loc_cfg.range,
            loc_cfg.range_unit, loc_cfg.comment.c_str());
  //cout << aprsmsg;

  sendMsg(aprsmsg);

} /* AprsTcpClient::sendAprsBeacon*/


void AprsTcpClient::sendMsg(const char *aprsmsg)
{
   //cout << aprsmsg << endl;

  if (!con->isConnected())
  {
    return;
  }

  int written = con->write(aprsmsg, strlen(aprsmsg));
  if (written < 0)
  {
    cerr << "*** ERROR: TCP write error" << endl;
  }
  else if ((size_t)written != strlen(aprsmsg))
  {
    cerr << "*** ERROR: TCP transmit buffer overflow, reconnecting." << endl;
    con->disconnect();
  }
} /* AprsTcpClient::sendMsg */



void AprsTcpClient::aprsLogin(void)
{
   char loginmsg[150];
   const char *format = "user %s pass %d vers SvxLink %s filter m/10\n";

   sprintf(loginmsg, format, el_call.c_str(), getPasswd(el_call),
           SVXLINK_VERSION);
   //cout << loginmsg;
   sendMsg(loginmsg);

} /* AprsTcpClient::aprsLogin */


// generate passcode for the aprs-servers, copied from xastir-source...
// special tnx to:
// Copyright (C) 1999,2000  Frank Giannandrea
// Copyright (C) 2000-2008  The Xastir Group

short AprsTcpClient::getPasswd(const string& call)
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
} /* AprsTcpClient::callpass */


void AprsTcpClient::tcpConnected(void)
{
  cout << "Connected to APRS server " << con->remoteHost() <<
          " on port " << con->remotePort() << endl;

  aprsLogin();                    // login
  offset_timer->reset();          // reset the offset_timer
  offset_timer->setEnable(true);  // restart the offset_timer
} /* AprsTcpClient::tcpConnected */


void AprsTcpClient::startNormalSequence(Timer *t)
{
  sendAprsBeacon(t);
  beacon_timer->setEnable(true);		// start the beaconinterval
} /* AprsTcpClient::startNormalSequence */



// ToDo: possible interaction of SvxLink on commands sended via
//       APRS-net
int AprsTcpClient::tcpDataReceived(TcpClient<>::TcpConnection *con,
                                   void *buf, int count)
{
   return count;                                // do nothing...
} /* AprsTcpClient::tcpDataReceived */



void AprsTcpClient::tcpDisconnected(TcpClient<>::TcpConnection *con,
                                    TcpClient<>::DisconnectReason reason)
{
  cout << "*** WARNING: Disconnected from APRS server" << endl;

  beacon_timer->setEnable(false);		// no beacon while disconnected
  reconnect_timer->setEnable(true);		// start the reconnect-timer
  offset_timer->setEnable(false);
  offset_timer->reset();
} /* AprsTcpClient::tcpDisconnected */


void AprsTcpClient::reconnectAprsServer(Async::Timer *t)
{
  reconnect_timer->setEnable(false);		// stop the reconnect-timer
  cout << "*** WARNING: Trying to reconnect to APRS server" << endl;
  con->connect();
} /* AprsTcpClient::reconnectNextAprsServer */



/*
 * This file has not been truncated
 */
