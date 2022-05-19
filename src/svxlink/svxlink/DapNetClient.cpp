/**
@file	 DapNetClient.cpp
@brief   Network connection manager for DapNet
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
#define DAPNETVERSION "v19052022"

#define INVALID 0

#define DAP_NOP 1000
#define DAP_TYPE2 1001
#define DAP_TYPE3 1002
#define DAP_TYPE4 1003
#define DAP_TIMESYNC 1004
#define DAP_INVALID 1005
#define DAP_MESSAGE 1006

#define LOGERROR 0
#define LOGWARN 1
#define LOGINFO 2
#define LOGDEBUG 3
#define LOGTRACE 4

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
  : cfg(cfg), name(name), dapcon(0), debug(0)
{
} /* DapNetClient */


DapNetClient::~DapNetClient(void)
{
  reconnect_timer = 0;
  delete dapcon;
  dapcon = 0;
  delete dapwebcon;
  dapwebcon = 0;
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
    
    string ric_section;
    string value;
    list<string>::iterator slit;
    if (cfg.getValue(name, "DAPNET_RIC2ISSI", ric_section))
    {
      list<string> dn2ric_list = cfg.listSection(ric_section);
      std::vector<string> issiList;
      for (slit=dn2ric_list.begin(); slit!=dn2ric_list.end(); slit++)
      {
        cfg.getValue(ric_section, *slit, value);
        if (rmatch(value, "[^0-9,]"))
        {
          cout << "*** ERROR: Config line in section [" << ric_section
               << "] for RIC " << *slit << " contains invalid characters. Only"
               << " the numbers 0-9 and comma are allowed." << endl;
          isok = false;
        }

        size_t listlen = SvxLink::splitStr(issiList, value, ",");
        if (listlen < 1)
        {
          cout << "*** ERROR: ISSI expected in section [" << ric_section
               << "] for RIC " << *slit << endl;
          isok = false;
        }
        else
        {
          ric2issi[atoi((*slit).c_str())] = issiList;
          dapnetLogmessage(LOGINFO, "RIC:" + *slit + "=ISSI:" + value);
        }
      }
    }
    else
    {
      cout << "*** ERROR: You need a section DAPNET_RIC2ISSI=[xxx] in "
           << name << endl;
      isok = false;
    }

    // RIC=Rubric1,Rubric2,Rubric22
    if (cfg.getValue(name, "DAPNET_RUBRIC_REGISTRATION", ric_section))
    {
      list<string> dn2ric_list = cfg.listSection(ric_section);
      
      for (slit=dn2ric_list.begin(); slit!=dn2ric_list.end(); slit++)
      {
        cfg.getValue(ric_section, *slit, value);
        ric2rubrics[atoi((*slit).c_str())] = value;
        dapnetLogmessage(LOGINFO, "RIC:" + *slit + "=rubrics:" + value);
      }
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
  
    // connector for sending messages over DAPNET web api
  if (cfg.getValue(name, "DAPNET_USERNAME", dapnet_username))
  {
    if (!cfg.getValue(name, "DAPNET_PASSWORD", dapnet_password))
    {
      cout << "*** ERROR: " << name << "/DAPNET_PASSWORD not set" << endl;
      isok = false;
    }
    if (!cfg.getValue(name, "DAPNET_WEBHOST", dapnet_webhost))
    {
      cout << "*** ERROR: " << name << "/DAPNET_WEBHOST not set" << endl;
      isok = false;
    }
    if (!cfg.getValue(name, "DAPNET_WEBPORT", dapnet_webport))
    {
      dapnet_webport=8080;
    }
    cfg.getValue(name, "DAPNET_WEBPATH", dapnet_webpath);
    if (!cfg.getValue(name, "DAPNET_TXGROUP", txgroup))
    {
      dapnetLogmessage(LOGWARN, "+++ DAPNET_TXGROUP not set, take 'dl-all'.");
      txgroup = "dl-all";
    }
    dapwebcon = new TcpClient<>(dapnet_webhost, dapnet_webport);
    dapwebcon->connected.connect(mem_fun(*this, 
                            &DapNetClient::onDapwebConnected));
    dapwebcon->disconnected.connect(mem_fun(*this, 
                            &DapNetClient::onDapwebDisconnected));
    dapwebcon->dataReceived.connect(mem_fun(*this, 
                            &DapNetClient::onDapwebDataReceived));
  }

  return isok;
} /* DapNetClient::initialize */


bool DapNetClient::sendDapMessage(std::string call, std::string message)
{
  transform(call.begin(), call.end(), call.begin(), ::tolower);
  destcall = call;
  destmessage = message;
  
  dapnetLogmessage(LOGTRACE, "DapNetClient::sendDapMessage: destcall="
        + destcall + ", message=" + destmessage);
  dapwebcon->connect();
  return true;
} /* DapNetClient::sendDapMessage */


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
  transform(callsign.begin(), callsign.end(), callsign.begin(), ::tolower);
  stringstream log;
  log << "DAPNET connection established to " << dapcon->remoteHost() << ":"
      << dapcon->remotePort();
  dapnetLogmessage(LOGINFO, log.str());
    
  stringstream ss;
  ss << "[" << DAPNETSOFT << " " << DAPNETVERSION << " " << callsign <<  " "
     << dapnet_key << "]\r\n";
  dapnetLogmessage(LOGDEBUG, ss.str());
  dapcon->write(ss.str().c_str(), ss.str().length());
  reconnect_timer->setEnable(false);
} /* DapNetClient::onDapnetConnected */
    

void DapNetClient::onDapnetDisconnected(TcpConnection *con, 
                TcpClient<>::DisconnectReason reason)
{
  stringstream ss;
  ss << "Disconnected from Dapnetserver " << con->remoteHost() << ":" << reason;
  dapnetLogmessage(LOGINFO, ss.str());
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


void DapNetClient::onDapwebConnected(void)
{
  dapnetLogmessage(LOGINFO, "connected to Dapnet " + dapnet_webhost
      + ":" + to_string(dapnet_webport));
  
  string auth = dapnet_username;
  auth += ":";
  auth += dapnet_password;

  stringstream content;
  content << "{ \"text\": \"" << destmessage 
          << "\", \"callSignNames\": [\"" << destcall
          << "\"], \"transmitterGroupNames\": [\"" << txgroup 
          << "\"], \"emergency\": false }";

  int clen = content.str().length();

  stringstream ss;
  ss << "POST " << dapnet_webpath << " HTTP/1.1\r\n"
     << "Host: " << dapnet_webhost << ":" 
     << dapnet_webport << "\r\n"
     << "Authorization: Basic " 
     << encodeBase64(auth.c_str(), auth.length()) << "\r\n"
     << "User-Agent: " << DAPNETSOFT << "/" << DAPNETVERSION << "\r\n"
     << "Accept: */*\r\n"
     << "Content-Type: application/json\r\n"
     << "Content-Length: " << clen << "\r\n\r\n" 
     << content.str();
  
  dapnetLogmessage(LOGDEBUG, ss.str());
  dapwebcon->write(ss.str().c_str(), ss.str().length());
} /* DapNetClient::onDapwebConnected */


// This method has copyed from:
// https://www.geeksforgeeks.org/encode-ascii-string-base-64-format/
char* DapNetClient::encodeBase64(const char input_str[], int len_str)
{
  // Character set of base64 encoding scheme
  char char_set[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  char *res_str = (char *) malloc(1000 * sizeof(char));
     
  int index, no_of_bits = 0, padding = 0, val = 0, count = 0, temp;
  int i, j, k = 0;
      
  // Loop takes 3 characters at a time from 
  // input_str and stores it in val
  for (i = 0; i < len_str; i += 3)
  {
    val = 0, count = 0, no_of_bits = 0;
  
    for (j = i; j < len_str && j <= i + 2; j++)
    {
      val = val << 8; 
      val = val | input_str[j]; 
      count++;
    }
  
    no_of_bits = count * 8; 
    padding = no_of_bits % 3; 
    while (no_of_bits != 0) 
    {
      if (no_of_bits >= 6)
      {
        temp = no_of_bits - 6;
        index = (val >> temp) & 63; 
        no_of_bits -= 6;         
      }
      else
      {
        temp = 6 - no_of_bits;
        index = (val << temp) & 63; 
        no_of_bits = 0;
      }
      res_str[k++] = char_set[index];
    }
  }
  for (i = 1; i <= padding; i++) 
  {
    res_str[k++] = '=';
  }
  res_str[k] = '\0';
  return res_str;  
} /* DapNetClient::encode_base64 */


void DapNetClient::onDapwebDisconnected(TcpConnection *con, 
                TcpClient<>::DisconnectReason reason)
{
  dapnetLogmessage(LOGDEBUG, "+++ disconnected from Dapnetweb, reason="
                   + reason);
} /* DapNetClient::onDapwebDisconnected */


int DapNetClient::onDapwebDataReceived(TcpConnection *con, void *buf, 
                                                      int count)
{
  char *str = static_cast<char *>(buf);
  string message(str, str+count);
  dapnetLogmessage(LOGINFO, "From DAPNET:" + message);
  return count;
} /* DapNetClient::onDapwebDataReceived */


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
      dapnetLogmessage(LOGINFO, "+++ unknown DAPNET message:" + dapmessage);
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
    dapnetLogmessage(LOGINFO, "+++ DAPNET: registered at time slot " + *ts);
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

  // find the 4th occurrence of :
  int j;
  size_t i = msg.find(":");
  for (j=1; j<4 && i != string::npos; ++j)
              i = msg.find(":", i+1);

  if (j == 4)
  {
    // check if the user is stored? no -> default
    string t_mesg = msg.substr(i+1, msg.length()-i);

    if (ric == 4512 || ric == 4520)
    {
      dapnetLogmessage(LOGDEBUG, "---" + rot1code(t_mesg)); 
    }
    else
    {
      dapnetLogmessage(LOGDEBUG, "---" + t_mesg);
    }

    std::vector<string>::iterator il;
    std::map<int, std::vector<string> >::iterator iu = ric2issi.find(ric);
    if (iu != ric2issi.end())
    {
      for(il = (iu->second).begin(); il != (iu->second).end(); il++)
      {
        dapnetLogmessage(LOGINFO, "+++ Forwarding message \"" + t_mesg
           + "\" from DAPnet to Tetra (RIC:" + to_string(ric) + "->ISSI:"
           + *il);
        dapnetMessageReceived(*il, t_mesg);
      }
    }

    // look into ric2rubric-section
    // RIC    =  Rubric
    // 12345  =  1024,1028,199 <-rubriclist
    StrList rubricList;
    std::map<int, string>::iterator it;
    for (it=ric2rubrics.begin(); it!= ric2rubrics.end();it++)
    {
      // *it is one line from Ric2Rubrics
      SvxLink::splitStr(rubricList, it->second, ",");
      unsigned int rubric;
      for(StrList::const_iterator rl=rubricList.begin(); rl!=rubricList.end(); rl++)
      {
        // *rl is one rubric
        rubric = atoi((*rl).c_str());
        if(rubric == ric)
        {
          iu = ric2issi.find(it->first);
          if (iu != ric2issi.end())
          {
            for(il = (iu->second).begin(); il != (iu->second).end(); il++)
            {
              dapnetLogmessage(LOGINFO, "+++ Forwarding message \"" + t_mesg
                + "\" from DAPnet to Tetra (RIC:" + to_string(ric)
                + "->ISSI:" + *il);
              dapnetMessageReceived(*il, t_mesg);
            }
          }
        }
      }
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


string DapNetClient::rot1code(string inmessage)
{
  std::string outmessage;
 
  for (string::iterator it= inmessage.begin(); it!=inmessage.end(); it++)
  {
    outmessage += (*it - 0x01);
  }
  return outmessage;
} /* DapNetClient::rot1code */

/*
 * This file has not been truncated
 */
