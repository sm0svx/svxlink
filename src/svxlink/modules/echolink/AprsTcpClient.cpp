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

#include <cstdio>
#include <cerrno>
#include <cstring>
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <stdio.h>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncTimer.h>
#include <AsyncConfig.h>
#include <version/SVXLINK.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "AprsTcpClient.h"


/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/

using namespace std;
using namespace Async;


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

#define kKey 0x73e2 				// This is the seed for the key


/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/

AprsTcpClient::AprsTcpClient(Async::Config &cfg, const std::string &name): 
  cfg(cfg), beacon_timer(0), tcp_port(14580)
{
   string value;
   destination = "APRS";				// Aprs-destination

   if (!cfg.getValue(name, "APRSHOST", value))
   {
      cerr << "*** ERROR: Config variable " << name << "/APRSHOST not set\n";
      return;
   }
   aprshost = value.c_str();

  
   cfg.getValue(name, "APRSPORT", value);		// tcp-port
   tcp_port = atoi(value.c_str());

   cfg.getValue(name, "ELCALLSIGN", value); 		// callsign for 
   el_tok = strtok((char*)value.c_str(),"-");
   el_call= strtok(NULL,"\0");
   
   cfg.getValue(name, "APRS_LAT_POSITION", aprslat); 	// lat-position   
   cfg.getValue(name, "APRS_LON_POSITION", aprslon); 	// lon-position
   cfg.getValue(name, "APRSPATH", aprspath);		// aprspath
   cfg.getValue(name, "TONE", value);			// ctcss or tone
   if (value.length() != 4) {
     cerr << "*** ERROR: Config variable " << name 
          << "/TONE not set or wrong, example \"TONE=T136\" or \"TONE=1750\"\n";
     return;
   } 
   tone = value;

   cfg.getValue(name, "RANGE", rng);			// range-param
   if (rng.length() != 3) {
     cerr << "*** ERROR: Config variable " << name 
          << "/RANGE not set or wrong, example \"RANGE=30k\", ignoring\n";
   }
   
   cfg.getValue(name, "APRS_COMMENT", aprs_comment); 	// APRS-comment
   cfg.getValue(name, "BEACONOFFSET", value);		// offset timer
   timeroffset = atoi(value.c_str())*1000;		// in seconds
   
   cfg.getValue(name, "APRSBEACONINTERVAL", value);	// time between two
							// beacons in minute-
							// intervals
							
   beaconinterval = atoi(value.c_str())*1000*60;	// make 1 minute inter-
							// vals, at least
   if (beaconinterval < 60000) {			// every 10 minutes
     beaconinterval = 600000;
   }
							
   cfg.getValue(name, "FREQUENCY", frequency);  	// frequency of the
   							// SvxLink-station
   if (frequency.length() != 7) {
     cerr << "*** ERROR: Config variable " << name 
          << "/FREQUENCY wrong value, example \"FREQUENCY=430.050\" \n";
     return;
   }
   							

   con = new TcpClient(aprshost, tcp_port);
   con->connected.connect(slot(*this, &AprsTcpClient::tcpConnected));
   con->disconnected.connect(slot(*this, &AprsTcpClient::tcpDisconnected));
   con->dataReceived.connect(slot(*this, &AprsTcpClient::tcpDataReceived));
   con->connect();

   beacon_timer = new Timer(beaconinterval, Timer::TYPE_PERIODIC);
   beacon_timer->setEnable(false);
   beacon_timer->expired.connect(slot(*this, &AprsTcpClient::sendAprsBeacon));

   offset_timer = new Timer(timeroffset, Timer::TYPE_ONESHOT);
   offset_timer->setEnable(false);
   offset_timer->expired.connect(slot(*this,
                 &AprsTcpClient::startNormalSequence));

   reconnect_timer = new Timer(5000);
   reconnect_timer->setEnable(false);
   reconnect_timer->expired.connect(slot(*this,
                 &AprsTcpClient::reconnectNextAprsServer));
   
} /* AprsTcpClient::AprsTcpClient */


AprsTcpClient::~AprsTcpClient(void)
{
   delete con;
   delete reconnect_timer;
   delete offset_timer;
   delete beacon_timer;
} /* AprsTcpClient::~AprsTcpClient */


void AprsTcpClient::sendAprsInfo(std::string info, int numberofstations)
{
  // Format for "object from..."
  // DL1HRC>;EL-242660*111111z4900.05NE00823.29E0434.687MHz T123 R10k   DA0AAA

  const char *format = "%s>%s,%s:;%s-%s*111111z%s/%s%d%s\r\n";
  sprintf(aprsmsg, format, el_call.c_str(), destination.c_str(), 
          aprspath.c_str(), el_tok.c_str(), el_call.c_str(), aprslon.c_str(), 
	  aprslat.c_str(), numberofstations, info.c_str());

  AprsTcpClient::sendMsg();
  
} /* void AprsTcpClient::sendAprsInfo */


void AprsTcpClient::sendAprsBeacon(Async::Timer *t)
{

  if (rng.length() == 3)
  {
    const char *format = "%s>%s,%s:;%s-%s*111111z%sE%s0%sMHz %s R%s %s\r\n";
    sprintf(aprsmsg, format, el_call.c_str(), destination.c_str(), 
            aprspath.c_str(), el_tok.c_str(), el_call.c_str(), aprslon.c_str(),
            aprslat.c_str(), frequency.c_str(), tone.c_str(), rng.c_str(), 
            aprs_comment.c_str());
  }
  else
  {
    const char *format = "%s>%s,%s:;%s-%s*111111z%sE%s0%sMHz %s %s\r\n";
    sprintf(aprsmsg, format, el_call.c_str(), destination.c_str(), 
            aprspath.c_str(), el_tok.c_str(), el_call.c_str(), aprslon.c_str(), 
	    aprslat.c_str(), frequency.c_str(), tone.c_str(), aprs_comment.c_str());
  }

  AprsTcpClient::sendMsg();

} /* AprsTcpClient::sendAprsBeacon*/


void AprsTcpClient::sendMsg(void)
{

  cout << aprsmsg << "\n";
  
  int written = con->write(aprsmsg, strlen(aprsmsg));  
  if (written != static_cast<int>(strlen(aprsmsg)))
  {
    if (written == -1)
    {
        cerr << "*** ERROR: TCP write error\n";
    }
    else
    {
        cerr << "*** ERROR: TCP transmit buffer overflow, reconnecting.\n";
        con->disconnect();
    }
  }
   
} /* AprsTcpClient::sendMsg */



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

void AprsTcpClient::aprsLogin(void)
{
   char loginmsg[150];
   const char *format = "user %s pass %d vers SvxLink %s filter m/10\n";
   const char *ecall = el_call.c_str();
   short passcode = callpass(ecall);

   sprintf(loginmsg, format, ecall, passcode, SVXLINK_VERSION);
   con->write(loginmsg, strlen(loginmsg));

} /* AprsTcpClient::aprsLogin */


// generate passcode for the aprs-servers, copied from xastir-source...
// special tnx to:
// Copyright (C) 1999,2000  Frank Giannandrea
// Copyright (C) 2000-2008  The Xastir Group
short AprsTcpClient::callpass(const char *theCall)
{
    char rootCall[10];           // need to copy call to remove ssid from parse
    char *p1 = rootCall;
    short hash;
    short i,len;
    char *ptr = rootCall;

    while ((*theCall != '-') && (*theCall != '\0')) 
        *p1++ = toupper((int)(*theCall++));
        *p1 = '\0';

    hash = kKey;                 // Initialize with the key value
    i = 0;
    len = (short)strlen(rootCall);

    while (i<len) {		 // Loop through the string two bytes at a time
        hash ^= (unsigned char)(*ptr++)<<8; // xor high byte with accumulated 
					    // hash
        hash ^= (*ptr++);	 // xor low byte with accumulated hash
        i += 2;
    }
    return (short)(hash & 0x7fff); // mask off the high bit so number is always 
				   // positive

} /* AprsTcpClient::callpass */



void AprsTcpClient::tcpConnected(void)
{

  cerr << "Aprsserver connected " << con->remoteHost() << 
          " on port " << con->remotePort() << "\n";

  AprsTcpClient::aprsLogin();			// login
  offset_timer->reset();			// reset th offset_timer
  offset_timer->setEnable(true);		// restart the offset_timer

} /* AprsTcpClient::tcpConnected */



void AprsTcpClient::startNormalSequence(Async::Timer *t)
{
  sendAprsBeacon(t);
  beacon_timer->setEnable(true);		// start the beaconinterval

} /* AprsTcpClient::startNormalSequence */



// ToDo: possible interaction of SvxLink on commands sended via
//       APRS-net
int AprsTcpClient::tcpDataReceived(Async::TcpClient::TcpConnection *con, 
     void *buf, int count)
{
   return count;                                // do nothing...

} /* AprsTcpClient::tcpDataReceived */



void AprsTcpClient::tcpDisconnected(Async::TcpClient::TcpConnection *con, 
  Async::TcpClient::DisconnectReason reason)
{
   cout << "**** WARN: disconnected from Aprsserver\n";
   
   beacon_timer->setEnable(false);		// no beacon while disconnected
   reconnect_timer->setEnable(true);		// start the reconnect-timer

} /* AprsTcpClient::tcpDisconnected */


/* ToDo list connect to next Aprsserver if the 1st one fails */
void AprsTcpClient::reconnectNextAprsServer(Async::Timer *t)
{
   reconnect_timer->setEnable(false);		// stop the reconnect-timer
   cout << "**** WARN: try to reconnect Aprsserver\n";
   con->connect();
   
} /* AprsTcpClient::reconnectNextAprsServer */


/*
 * This file has not been truncated
 */
