/**
@file	 DapNetClient.cpp
@brief   Network connection manager for DapNet transceivers
@author  Tobias Blomberg / SM0SVX & Adi Bier / DL1HRC
@date	 2021-02-07

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



/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <cerrno>
#include <cstring>
#include <regex.h>
#include <algorithm>    // std::transform


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncTimer.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "DapNetClient.h"
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

#define DAPNETSOFT "SvxLink-TetraGw"
#define DAPNETVERSION "v07022021"

#define INVALID 0

#define DAP_NOP 1000
#define DAP_TYPE2 1001
#define DAP_TYPE3 1002
#define DAP_TYPE4 1003
#define DAP_TIMESYNC 1004
#define DAP_INVALID 1005
#define DAP_MESSAGE 1006


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




/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/

DapNetClient::DapNetClient(Config &cfg, const string& name)
  : cfg(cfg), name(name), dapcon(0), debug(false)
{
} /* DapNetClient */


DapNetClient::~DapNetClient(void)
{
  reconnect_timer = 0;
  delete dapcon;
  dapcon = 0;
} /* DapNetClient::~DapNetClient */


bool DapNetClient::initialize(void)
{
  bool isok = true;
  cfg.getValue(name, "DEBUG", debug);
  cfg.getValue(name, "DAPNET_CALLSIGN", callsign);
  
  if (cfg.getValue(name, "DAPNET_SERVER", dapnet_server))
  {
    if (!cfg.getValue(name, "DAPNET_PORT", dapnet_port))
    {
      cout << "*** ERROR: DAPNET_SERVER defined but no DAPNET_PORT in " 
           << name << endl;
      isok = false;
    }
    if (!cfg.getValue(name, "DAPNET_KEY", dapnet_key))
    {
      cout << "*** ERROR: DAPNET_SERVER defined but no DAPNET_KEY in " 
           << name << endl;
      isok = false;
    }
    
    string dn2ric_section;
    string value;
    list<string>::iterator slit;
    if (cfg.getValue(name, "DAPNET_RIC2ISSI", dn2ric_section))
    {
      list<string> dn2ric_list = cfg.listSection(dn2ric_section);
      
      for (slit=dn2ric_list.begin(); slit!=dn2ric_list.end(); slit++)
      {
        cfg.getValue(dn2ric_section, *slit, value);
        if (value.length() > 8)
        {
          cout << "*** ERROR: Issi (" << value << " ) must have " 
               << "8 digits or less, for RIC " << *slit 
               << " in section [" << dn2ric_section << "]" << endl;
        }
        else
        {
          ric2issi[atoi((*slit).c_str())] = value;
          cout << "RIC:" << *slit << "=ISSI:" << value << endl;
        }
      }
    }
    else
    {
      cout << "*** ERROR: You need a section DAPNET_RIC2ISSI=[xxx] in " 
           << name << endl;
      isok = false;
    }
    
    dapcon = new TcpClient<>(dapnet_server, dapnet_port);
    dapcon->connected.connect(mem_fun(*this, 
                            &DapNetClient::onDapnetConnected));
    dapcon->disconnected.connect(mem_fun(*this, 
                            &DapNetClient::onDapnetDisconnected));
    dapcon->dataReceived.connect(mem_fun(*this, 
                            &DapNetClient::onDapnetDataReceived));
    dapcon->connect();
    reconnect_timer = new Timer(5000);
    reconnect_timer->setEnable(false);
    reconnect_timer->expired.connect(mem_fun(*this,
                 &DapNetClient::reconnectDapnetServer));
  }
  
  return isok;
} /* DapNetClient::initialize */




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

void DapNetClient::onDapnetConnected(void)
{
  stringstream ss;
  transform(callsign.begin(), callsign.end(), callsign.begin(), ::tolower);
  cout << "DAPNET connection established to " << dapcon->remoteHost() << "\n";
  ss << "[" << DAPNETSOFT << " " << DAPNETVERSION << " " << callsign <<  " " 
     << dapnet_key << "]\r\n";
  if (debug) cout << ss.str() << endl;
  dapcon->write(ss.str().c_str(), ss.str().length());
  reconnect_timer->setEnable(false);
} /* DapNetClient::onDapnetConnected */
    

void DapNetClient::onDapnetDisconnected(TcpConnection *con, 
                TcpClient<>::DisconnectReason reason)
{
  cout << "Disconnected from Dapnetserver " << con->remoteHost() 
       << ":" << reason << "\n";
  reconnect_timer->reset();
  reconnect_timer->setEnable(true);
} /* DapNetClient::onDapnetDisconnected */


int DapNetClient::onDapnetDataReceived(TcpConnection *con, void *buf, int count)
{
  size_t found;
  char *str = static_cast<char *>(buf);
  string message(str, str+count);
  dapmessage += message;

  if (message[0] == 0x00)
  {
    dapmessage.erase();
  }
  
  while ((found = dapmessage.find("\n")) != string::npos)
  {
    if (found != 0)
    {
      handleDapMessage(dapmessage.substr(0, found));
      dapmessage.erase(0, found+1);
    }
  }
  return count;
} /* DapNetClient::onDapnetDataReceived */


void DapNetClient::reconnectDapnetServer(Timer *t)
{
  reconnect_timer->setEnable(false);
  dapcon->connect();
} /* DapNetClient::reconnectDapnetServer*/


void DapNetClient::handleDapMessage(std::string dapmessage)
{
  size_t found = dapmessage.find(0x0a);
  if (found != string::npos)
  {
    dapmessage.erase(found,1);
  }

  int m_msg = checkDapMessage(dapmessage);

  switch (m_msg)
  {
    case DAP_TYPE2:
      dapTanswer(dapmessage);
      break;

    case DAP_TYPE3:
      dapOK();
      break;

    case DAP_TYPE4:
      dapOK();
      handleDapType4(dapmessage);
      break;
      
    case DAP_TIMESYNC:
      handleTimeSync(dapmessage);
      break;

    case DAP_MESSAGE:
      handleDapText(dapmessage);
      break;

    default:
      if (debug) cout << "+++ unknown DAPNET message" << dapmessage << endl;
      break;
  }
} /* DapNetClient::handleDapMessage*/


void DapNetClient::handleTimeSync(std::string msg)
{
  std::string t_nr = msg.substr(1,2);
  int num = (int)strtol(t_nr.c_str(), NULL, 16);
  if (++num > 255) num = 0;
  
  stringstream t_answ;
  t_answ << "#" << ::uppercase << setfill('0')  << std::setw(2) 
         << hex << num << " +\r\n";
  dapcon->write(t_answ.str().c_str(), t_answ.str().length());
} /* DapNetClient::handleTimeSync */


void DapNetClient::handleDapType4(std::string msg)
{
  msg.erase(0,2);  // erase "4:"
  for (string::iterator ts = msg.begin(); ts!= msg.end(); ++ts)
  {
    cout << "+++ DAPNET: registered at time slot " << *ts << endl;
  }
} /* DapNetClient::handleDapType4 */


/*
structure of a dapnet message:
#06 6:1:1E6A6:3:Brand in Erfurt, BAB4, EF Kreuz - AS EF West 12:36 Im Bereich..
*/
void DapNetClient::handleDapText(std::string msg)
{
  stringstream t_answ;
  std::string t_nr = msg.substr(1,2);

  int num = (int)strtol(t_nr.c_str(), NULL, 16);
  if (++num > 255) num = 0;
  t_answ << "#" << ::uppercase << setfill('0') << std::setw(2) 
         << hex << num << " +\r\n";
  dapcon->write(t_answ.str().c_str(), t_answ.str().length());

  // check RIC
  StrList dapList;
  SvxLink::splitStr(dapList, msg, ":");

  unsigned int ric;
  std::stringstream ss;
  ss << std::hex << dapList[2];
  ss >> ric;

  // check if the user is stored? no -> default
  std::map<int, string>::iterator iu = ric2issi.find(ric);
  if (iu != ric2issi.end())
  {
    // find the 4th ocurence of :
    int j;
    size_t i = msg.find(":");
    for (j=1; j<4 && i != string::npos; ++j)
                i = msg.find(":", i+1);

    if (j == 4)
    {
      string t_mesg = msg.substr(i+1, msg.length()-i);
      if (debug)
      {
        cout << "+++ Forwarding message \"" << t_mesg << "\" from DAPnet " 
             << "to Tetra (RIC:" << ric << "->ISSI:" << iu->second << ")" 
            << endl;
      }
      dapnetMessageReceived(iu->second, t_mesg);
    }
  }
} /* DapNetClient::handleDapText */


void DapNetClient::dapTanswer(std::string msg)
{
  std::string m_msg = msg;
  m_msg += ":0000\r\n+\r\n";
  dapcon->write(m_msg.c_str(), m_msg.length());
} /* DapNetClient::dapanswer */


void DapNetClient::dapOK(void)
{
  std::string t_answ = "+\r\n";
  dapcon->write(t_answ.c_str(), t_answ.length()); 
} /* DapNetClient::dapOK */


int DapNetClient::checkDapMessage(std::string mesg)
{
  int retvalue = INVALID;
  typedef std::map<std::string, int> Mregex;
  Mregex mre;
  map<string, int>::iterator rt;

  mre["^2:[0-9A-F]{4}"]         = DAP_TYPE2;
  mre["^3:\\+[0-9A-F]{4}"]      = DAP_TYPE3;
  mre["^4:[0-9A-F]{1,}"]        = DAP_TYPE4;
  mre["^7:"]                    = DAP_INVALID;
  mre["^#[0-9A-F]{2} 5:"]       = DAP_TIMESYNC;
  mre["^#[0-9A-F]{2} 6:"]       = DAP_MESSAGE;

  for (rt = mre.begin(); rt != mre.end(); rt++)
  {
    if (rmatch(mesg, rt->first))
    {
      retvalue = rt->second;
      return retvalue;
    }
  }
  return retvalue;
} /* DapNetClient::handleDapMessage */


bool DapNetClient::rmatch(std::string tok, std::string pattern)
{
  regex_t re;
  int status = regcomp(&re, pattern.c_str(), REG_EXTENDED);
  if (status != 0)
  {
    return false;
  }

  bool success = (regexec(&re, tok.c_str(), 0, NULL, 0) == 0);
  regfree(&re);
  return success;
} /* DapNetClient::rmatch */


/*
 * This file has not been truncated
 */
