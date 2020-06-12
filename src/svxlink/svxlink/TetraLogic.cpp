/**
@file	 TetraLogic.cpp
@brief   Contains a Tetra logic SvxLink core implementation
@author  Tobias Blomberg / SM0SVX & Adi Bier / DL1HRC
@date	 2020-05-27

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



/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <cstdio>
#include <cstdlib>
#include <string>
#include <cstring>
#include <stdio.h>
#include <iostream>
#include <regex.h>
#include <algorithm>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <Rx.h>
#include <Tx.h>
#include <common.h>
#include <AsyncTimer.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "TetraLogic.h"
#include "TetraLib.h"
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

#define OK 0
#define ERROR 1
#define ERROR3 2
#define GROUPCALL_BEGIN 3
#define GROUPCALL_END 4

#define SDS 6
#define TEXT_SDS 7
#define ERROR35 8
#define CALL_CONNECT 9
#define CALL_END 10
#define GROUPCALL_RELEASED 11
#define LIP_SDS 12
#define REGISTER_TEI 13
#define STATE_SDS 14
#define OP_MODE 15
#define TX_GRANT 16
#define TX_DEMAND 17
#define TX_WAIT 18
#define TX_INTERRUPT 19
#define SIMPLE_LIP_SDS 20
#define COMPLEX_SDS 21

#define INVALID 254
#define TIMEOUT 255

/****************************************************************************
 *
 * Local class definitions
 *
 ****************************************************************************/

class TetraLogic::Call
{

  public:

  Call()
  {

  }

  ~Call()
  {

  }

  private:

};


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


TetraLogic::TetraLogic(Async::Config& cfg, const string& name)
  : Logic(cfg, name), mute_rx_on_tx(true), mute_tx_on_rx(true),
  rgr_sound_always(false), mcc(""), mnc(""), issi(""), gssi(1),
  port("/dev/ttyUSB0"), baudrate(115200), initstr(""), peistream(""),
  debug(false), talkgroup_up(false),
  peiComTimer(1000, Timer::TYPE_ONESHOT, false),
  peiActivityTimer(10000, Timer::TYPE_ONESHOT, true),
  tgUpTimer(50000, Timer::TYPE_ONESHOT, false)
{
  peiComTimer.expired.connect(mem_fun(*this, &TetraLogic::onComTimeout));
  peiActivityTimer.expired.connect(mem_fun(*this, &TetraLogic::onPeiActivityTimeout));
  tgUpTimer.expired.connect(mem_fun(*this, &TetraLogic::tgUpTimeout));
  
} /* TetraLogic::TetraLogic */


TetraLogic::~TetraLogic(void)
{
  delete pei;
  pei = 0;
  peiComTimer = 0;
  peiActivityTimer = 0;
  tgUpTimer = 0;
  delete call;
} /* TetraLogic::~TetraLogic */


bool TetraLogic::initialize(void)
{
  bool isok = true;
  if (!Logic::initialize())
  {
    isok = false;
  }

  cfg().getValue(name(), "MUTE_RX_ON_TX", mute_rx_on_tx);
  cfg().getValue(name(), "MUTE_TX_ON_RX", mute_tx_on_rx);
  cfg().getValue(name(), "RGR_SOUND_ALWAYS", rgr_sound_always);

  string value;
  if (!cfg().getValue(name(), "ISSI", issi))
  {
     cerr << "*** ERROR: Missing parameter " << name() << "/ISSI" << endl;
     isok = false;
  }

  if (!cfg().getValue(name(), "MCC", mcc))
  {
     cerr << "*** ERROR: Missing parameter " << name() << "/MCC" << endl;
     isok = false;
  }
  if (atoi(mcc.c_str()) > 901)
  {
     cerr << "*** ERROR: Country code (MCC) must be 901 or less" << endl;
     isok = false;
  }
  if (mcc.length() < 4)
  {
    value = "0000";
    value += mcc;
    mcc = value.substr(value.length()-4,4);
  }

  if (!cfg().getValue(name(), "APRSPATH", aprspath))
  {
    aprspath = "APRS,qAR,";
    aprspath += callsign();
    aprspath += "-10:";
  }
  if (!cfg().getValue(name(), "MNC", mnc))
  {
    cerr << "*** ERROR: Missing parameter " << name() << "/MNC" << endl;
    isok = false;
  }
  if (atoi(mnc.c_str()) > 16383)
  {
    cerr << "*** ERROR: Network code (MNC) must be 16383 or less" << endl;
    isok = false;
  }
  if (mnc.length() < 5)
  {
    value = "00000";
    value += mcc;
    mcc = value.substr(value.length()-5,5);
  }

  cfg().getValue(name(), "DEBUG", debug);

  if (!cfg().getValue(name(), "PORT", port))
  {
     cout << "Warning: Missing parameter " << name() << "/PORT"
          << endl;
  }

  if (!cfg().getValue(name(), "BAUD", baudrate))
  {
     cout << "Warning: Missing parameter " << name() << "/BAUD, guess "
          << baudrate << endl;
  }

  char t_aprs_sym, t_aprs_tab;
  if (cfg().getValue(name(), "DEFAULT_APRS_ICON", value))
  {
    if (value.length() != 2)
    {
      isok = false;
      cout << "*** ERROR: " << name() << "/DEFAULT_APRS_ICON "
           << "must have exactly 2 characters, e.g. '/e'" << endl;
    }
    else 
    {
      t_aprs_sym = value[0];
      t_aprs_tab = value[1];
    }
  }
    
  // the pty path: inject messages to send by Sds
  string sds_pty_path;
  cfg().getValue(name(), "SDS_PTY", sds_pty_path);
  if (!sds_pty_path.empty())
  {
    sds_pty = new Pty(sds_pty_path);
    if (!sds_pty->open())
    {
      cerr << "*** ERROR: Could not open Sds PTY "
           << sds_pty_path << " as specified in configuration variable "
           << name() << "/" << "SDS_PTY" << endl;
      isok = false;
    }
    sds_pty->dataReceived.connect(
        mem_fun(*this, &TetraLogic::sdsPtyReceived));
  }

  // read infos of tetra users configured in svxlink.conf
  string user_section;
  if (cfg().getValue(name(), "TETRA_USERS", user_section))
  {
    list<string> user_list = cfg().listSection(user_section);
    list<string>::iterator ulit;


    for (ulit=user_list.begin(); ulit!=user_list.end(); ++ulit)
    {
      cfg().getValue(user_section, *ulit, value);
      if ((*ulit).length() != 17)
      {
        cout << "*** ERROR: Wrong length of TEI in TETRA_USERS definition, "
             << "should have 17 digits (MCC[4] MNC[5] ISSI[8]), e.g. "
             << "09011638312345678" << endl;
        isok = false;
      }
      else
      {
        User m_user;
        m_user.call = getNextStr(value);
        m_user.name = getNextStr(value);
        std::string m_aprs = getNextStr(value);
        if (m_aprs.length() != 2)
        {
          cout << "*** ERROR: Check Aprs icon definition for " << m_user.call
               << " in section " << user_section 
               << ". It must have exactly 2 characters, e.g.: 'e\'" << endl;
          isok = false;
        }
        else
        {
          m_user.aprs_sym = m_aprs[0];
          m_user.aprs_tab = m_aprs[1];
        }
        m_user.comment = getNextStr(value);
        userdata[*ulit] = m_user;
      }
    }
  }

  // read info of tetra state to receive SDS's
  std::string status_section;
  if (cfg().getValue(name(), "TETRA_STATUS", status_section))
  {
    list<string> state_list = cfg().listSection(status_section);
    list<string>::iterator slit;
    for (slit=state_list.begin(); slit!=state_list.end(); ++slit)
    {
      cfg().getValue(status_section, *slit, value);
      state_sds[*slit] = value;
    }
  }

  if (!cfg().getValue(name(), "INIT_PEI", initstr))
  {
    cout << "Warning: Missing parameter " << name()
         << "/INIT_PEI, using defaults" << endl;
  }
  SvxLink::splitStr(initcmds, initstr, ";");

  m_cmds = initcmds;

  pei = new Serial(port);
  pei->setParams(baudrate, Serial::PARITY_NONE, 8, 1, Serial::FLOW_NONE);
  pei->charactersReceived.connect(
      	  mem_fun(*this, &TetraLogic::onCharactersReceived));

  if (!pei->open(true))
  {
    cerr << "*** ERROR: Opening serial port " << name() << "/\""
         << port << "\"" << endl;
    isok = false;
  }

  peirequest = INIT;
  initPei();

  rxValveSetOpen(true);
  setTxCtrlMode(Tx::TX_AUTO);

  processEvent("startup");

  //handleLipSds("0A008CACAA480A120201D0");

  return isok;

} /* TetraLogic::initialize */



/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/

 void TetraLogic::audioStreamStateChange(bool is_active, bool is_idle)
{
  if (is_active)
  {
    tgUpTimer.reset();
    tgUpTimer.setEnable(false);
    initGroupCall(1);
  }

  Logic::audioStreamStateChange(is_active, is_idle);
  
} /* TetraLogic::audioStreamStateChange */

 
void TetraLogic::squelchOpen(bool is_open)
{
  //cout << name() << ": The squelch is " << (is_open ? "OPEN" : "CLOSED")
  //     << endl;

    // FIXME: A squelch open should not be possible to receive while
    // transmitting unless mute_rx_on_tx is false, in which case it
    // should be allowed. Commenting out the statements below.
#if 0
  if (tx().isTransmitting())
  {
    return;
  }
#endif

  if (!is_open)
  {
    if (rgr_sound_always || (activeModule() != 0))
    {
      enableRgrSoundTimer(true);
    }

    if (mute_tx_on_rx)
    {
      setTxCtrlMode(Tx::TX_AUTO);
    }
  }
  else
  {
    tgUpTimer.reset();
    enableRgrSoundTimer(false);
    if (mute_tx_on_rx)
    {
      setTxCtrlMode(Tx::TX_OFF);
    }
  }

  rx().setMuteState(is_open ? Rx::MUTE_NONE : Rx::MUTE_ALL);
  //rx().setSql(is_open);
  tgUpTimer.setEnable(!is_open);
  Logic::squelchOpen(is_open);

} /* TetraLogic::squelchOpen */


void TetraLogic::transmitterStateChange(bool is_transmitting)
{
  std::string cmd;
  if (is_transmitting) 
  {
    if (!talkgroup_up)
    {
      initGroupCall(1);
      talkgroup_up = true;
    }
    else 
    {
      cmd = "AT+TXI=1,1";
      sendPei(cmd);
    }
  }
  else
  {
    cmd = "AT+TXI=0,1";
    sendPei(cmd);
  }
  
  if (mute_rx_on_tx)
  {
   // rx().setMuteState(is_transmitting ? Rx::MUTE_ALL : Rx::MUTE_NONE);
  }
  tgUpTimer.setEnable(!is_transmitting);
  Logic::transmitterStateChange(is_transmitting);
} /* TetraLogic::transmitterStateChange */


void TetraLogic::allMsgsWritten(void)
{
  Logic::allMsgsWritten();
  if (!talkgroup_up)
  {
    setTxCtrlMode(Tx::TX_AUTO);
  }
} /* TetraLogic::allMsgsWritten */



/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/


 /*
  Initialize the Pei device, here some commends that being used
  to (re)direct the answers to the Pei port. See EN 300 392-5
  V2.2.0 manual, page 62 for further info

  TETRA Service Profile +CTSP:
  +CTSP=<service profile>, <service layer1>, [<service layer2>],
        [<AI mode>], [<link identifier>]

  AT+CTOM=1           set MRT into DMO-MS mode (0-TMO, 6-DMO-Repeater)
  AT+CTSP=1,3,131     Short Data Service type 4 with Transport Layer (SDS-TL)
                      service
                      131 - GPS
  AT+CTSP=1,3,130     130 - Text Messaging
  AT+CTSP=1,2,20      Short Data Service (SDS)
                      20 - Status
  AT+CTSP=2,0,0       0 - Voice
  AT+CTSP=1,3,24      24 - SDS type 4, PID values 0 to 127
  AT+CTSP=1,3,25      25 - SDS type 4, PID values 128 to 255
  AT+CTSP=1,3,3       3 - Simple GPS
  AT+CTSP=1,3,10      10 - Location information protocol
  AT+CTSP=1,1,11      11 - Group Management

  TETRA service definition for Circuit Mode services +CTSDC
  +CTSDC=<AI service>, <called party identity type>, [<area>], [<hook>],
         [<simplex>], [<end to end encryption>],[<comms type>],
         [<slots/codec>], [<RqTx>], [<priority>], [<CLIR control>]
  AT+CTSDC=0,0,0,1,1,0,1,1,0,0
 */
void TetraLogic::initPei(void)
{
  stringstream ss;
  if (!m_cmds.empty())
  {
    std::string cmd = *(m_cmds.begin());
    sendPei(cmd);
    m_cmds.erase(m_cmds.begin());
  }
  else
  {
    peirequest = INIT_COMPLETE;
    ss << "pei_init_finished";
    processEvent(ss.str());
  }
} /* TetraLogic::initPei */


void TetraLogic::onCharactersReceived(char *buf, int count)
{
  peiComTimer.setEnable(false);
  peiActivityTimer.reset();
  std::string m_message;
  size_t found, found2, found3, found_ds;

  peistream += buf;

  /*
  The asynchronous handling of incoming PEI commands is not easy due
  to the unpredictability of the reception of characters from the serial port.
  We have to analyze the incoming characters until we find the first
  \r\n-combination. Afterwards we are looking for a second occurrence,
  if one occurs, then we have an entire PEI command. The rest of the
  data is then left untouched.
  If we find a \r\n-combination after the second one, then it is most
  likely a SDS as an unsolicited answer just following the e.g.
  +CTSDSR:xxx message.
  */

  found = peistream.find("\r\n");
  found2 = peistream.find("\r\n", found + 1);
  found3 =  peistream.find("\r\n", found2 + 1);
  
  if (found != string::npos && found2 != string::npos)
  {
    // check for an incoming SDS
    if ((found_ds = peistream.find("+CTSDSR:") != string::npos))
    {
      if (found3 == string::npos)
      {
        return;  
      }
      else 
      {
        peistream.replace(found2, 2, ", ");
        m_message = peistream.substr(found + 2, found3 - found - 2);
        peistream.erase(found, found3 + 2);
      }
    }
    else
    {
      m_message = peistream.substr(found + 2, found2 - found - 2);
      peistream.erase(found, found2 + 2);
    }
  }
  else
  {
    return;
  }

   // debug output
  if (debug)
  {
    cout << "Pei message: >" << m_message << "<" << endl;
  }

  stringstream ss;
  int response = handleMessage(m_message);

  switch (response)
  {
    case OK:
      peistate = OK;
      break;

    case ERROR:
      peistate = ERROR;
      if (m_message.length()>11 && debug)
      {
        cout << getPeiError(atoi(m_message.erase(0,11).c_str())) << endl;
      }
      break;

    case GROUPCALL_BEGIN:
      handleGroupcallBegin(m_message);
      break;

    case GROUPCALL_END:
      handleGroupcallEnd(m_message);
      break;

    case CALL_END:
      handleCallEnd(m_message);
      break;

    case SDS:
      handleSds(m_message);
      break;

    case TX_DEMAND:
      break;

    case TX_GRANT:
      handleTxGrant(m_message);
      break;

    case CALL_CONNECT:
      break;

    case OP_MODE:
      getOpMode(m_message);
      break;

    case INVALID:
      cout << "+++ Pei answer not known, ignoring ;)"
           << endl;

    default:
      break;
  }

  if (peirequest == INIT)
  {
    initPei();
  }

} /* TetraLogic::onCharactersReceived*/


void TetraLogic::initGroupCall(int gc_gssi)
{
  std::string icmd = "AT+CTSDC=0,0,0,1,1,0,1,1,0,0";
  sendPei(icmd);
  
  std::string cmd = "ATD";
  cmd += to_string(gc_gssi);
  sendPei(cmd);
  
  stringstream ss;
  ss << "init_group_call " << to_string(gc_gssi);
  processEvent(ss.str());
} /* TetraLogic::initGroupCall */


void TetraLogic::endCall(void)
{
  std::string cmd = "ATH";
  sendPei(cmd);
  
  stringstream ss;
  ss << "end_call";
  processEvent(ss.str());
} /* TetraLogic::endCall */


/*
TETRA Incoming Call Notification +CTICN

+CTICN: <CC instance >, <call status>, <AI service>,
[<calling party identity type>], [<calling party identity>],
[<hook>], [<simplex>], [<end to end encryption>],
[<comms type>], [<slots/codec>], [<called party identity type>],
[<called party identity>], [<priority level>]

 Example:        MCC| MNC| ISSI  |             MCC| MNC|  GSSI |
 +CTICN: 1,0,0,5,09011638300023404,1,1,0,1,1,5,09011638300000001,0
*/
void TetraLogic::handleGroupcallBegin(std::string message)
{

  if (message.length() < 65)
  {
    cout << "*** No valid +CTICN response, message to short" << endl;
    return;
  }

  squelchOpen(true);  // open the Squelch

  Callinfo t_ci;
  stringstream ss;
  message.erase(0,8);
  std::string h = message;

  // split the message received from the Pei into single parmeters
  // for further use, not all of them are interesting
  t_ci.instance = getNextVal(h);
  t_ci.callstatus = getNextVal(h);
  t_ci.aistatus = getNextVal(h);
  t_ci.origin_cpit = getNextVal(h);

  std::string o_tei = getNextStr(h);
  t_ci.o_mcc = atoi(o_tei.substr(0,4).c_str());
  t_ci.o_mnc = atoi(o_tei.substr(4,5).c_str());
  t_ci.o_issi = atoi(o_tei.substr(9,8).c_str());

  t_ci.hook = getNextVal(h);
  t_ci.simplex = getNextVal(h);
  t_ci.e2eencryption = getNextVal(h);
  t_ci.commstype = getNextVal(h);
  t_ci.codec = getNextVal(h);
  t_ci.dest_cpit = getNextVal(h);

  std::string d_tei = getNextStr(h);
  t_ci.d_mcc = atoi(d_tei.substr(0,4).c_str());
  t_ci.d_mnc = atoi(d_tei.substr(4,5).c_str());
  t_ci.d_issi = atoi(d_tei.substr(9,8).c_str());
  t_ci.prio = atoi(h.c_str());

  // store call specific data into a Callinfo struct
  callinfo[t_ci.o_issi] = t_ci;

  // update last activity of a user
  struct tm *utc;
  time_t rawtime = time(NULL);
  utc = gmtime(&rawtime);
  userdata[o_tei].last_activity = utc;

  // store info in Qso struct
  Qso.tei = o_tei;
  Qso.start = utc;
  std::list<std::string>::iterator it;
  it = find(Qso.members.begin(), Qso.members.end(), userdata[o_tei].call);
  if (it == Qso.members.end())
  {
    Qso.members.push_back(userdata[o_tei].call);    
  }

  // callup tcl event
  ss << "groupcall_begin " << t_ci.o_issi << " " << t_ci.d_issi;
  processEvent(ss.str());

  // send group info to aprs network
  if (LocationInfo::has_instance())
  {
    stringstream m_aprsmesg;
    m_aprsmesg << aprspath << ">" << userdata[o_tei].call 
               << " initiated groupcall: " << t_ci.o_issi 
               << " -> " << t_ci.d_issi;
    cout << m_aprsmesg.str() << endl;
    LocationInfo::instance()->update3rdState(userdata[o_tei].call, m_aprsmesg.str());
  }

} /* TetraLogic::handleGroupcallBegin */


/*
 TETRA SDS Receive +CTSDSR

 CTSDSR unsolicited Result Codes
 +CTSDSR: <AI service>, [<calling party identity>],
 [<calling party identity type>], <called party identity>,
 <called party identity type>, <length>,
 [<end to end encryption>]<CR><LF>user data

 Example:
 +CTSDSR: 12,23404,0,23401,0,112, 82040801476A61746A616A676A61
*/
void TetraLogic::handleSds(std::string sds)
{
  stringstream ss;
  sds.erase(0,9);  // remove "+CTSDSR: "
  Sds m_sds;

  int m_sdsid = pending_sds.size() + 1;
  
  struct tm *utc;
  time_t rawtime = time(NULL);
  utc = gmtime(&rawtime);
  
  m_sds.tos = utc;       // last activity
  userdata[m_sds.tei].last_activity = utc;  // update last activity of sender
  m_sds.direction = 1;   // 1 = received
  
  m_sds.type = getNextVal(sds); // type of SDS (12)
  m_sds.tei = getTEI(getNextStr(sds)); // sender TEI (23404)
  getNextVal(sds); // (0)
  getNextVal(sds); // destination Issi
  getNextVal(sds); // (0)
  getNextVal(sds); // Sds length (112)

  int m_sdstype = handleMessage(sds.erase(0,1));

  float lat, lon;
  std::string sds_txt;
  stringstream m_aprsinfo;
    
  switch (m_sdstype)
  {
    case LIP_SDS:
      handle_LIP_short(sds, lat, lon);
      m_aprsinfo << "!" << dec2nmea_lat(lat) << "N" 
         << userdata[m_sds.tei].aprs_sym << dec2nmea_lon(lon) << "E"
         << userdata[m_sds.tei].aprs_tab << userdata[m_sds.tei].name 
         << ", " << userdata[m_sds.tei].comment;
      ss << "lip_sds_received " << m_sds.tei << " " << lat << " " << lon;
      userdata[m_sds.tei].lat = lat;
      userdata[m_sds.tei].lon = lon;
      break;
    
    case STATE_SDS:
      handleStateSds(sds);
      userdata[m_sds.tei].state = sds;
      m_aprsinfo << ">" << "State:" << state_sds.find(sds)->second << " (" 
                 << sds << ")";
      ss << "state_sds_received " << m_sds.tei << " " << sds;
      break;
      
    case TEXT_SDS:
      sds_txt = handleTextSds(sds);
      m_aprsinfo << ">" << sds_txt;
      ss << "text_sds_received " << m_sds.tei << " \"" << sds_txt
         << "\"";
      break;

    case REGISTER_TEI:
      ss << "register_tei " << m_sds.tei;
      break;
    
    case INVALID:
      ss << "unknown_sds_received";
      cout << "*** Unknown type of SDS" << endl;
      break;
    
    default:
      return;
  }
  
  pending_sds[m_sdsid] = m_sds;

  // send sds info of a user to aprs network
  if (LocationInfo::has_instance() && m_aprsinfo.str().length() > 0)
  {
    stringstream m_aprsmessage;
    m_aprsmessage << aprspath << m_aprsinfo.str();
    if (debug)
    {
      cout << m_aprsmessage.str() << endl;
    }
    LocationInfo::instance()->update3rdState(userdata[m_sds.tei].call, 
                                               m_aprsmessage.str());
  }
      
  if (ss.str().length() > 0)
  {
    processEvent(ss.str());
  }

} /* TetraLogic::getTypeOfService */


std::string TetraLogic::handleTextSds(std::string m_message)
{
  if (m_message.length() > 8) m_message.erase(0,8);  // delete 00A3xxxx
  return decodeSDS(m_message);
} /* TetraLogic::handleTextMessage */


/*
  Transmission Grant +CTXG
  +CTXG: <CC instance>, <TxGrant>, <TxRqPrmsn>, <end to end encryption>,
         [<TPI type>], [<TPI>]
  e.g.:
  +CTXG: 1,3,0,0,3,09011638300023404
*/
void TetraLogic::handleTxGrant(std::string txgrant)
{
  stringstream ss;
  squelchOpen(true);  // open Squelch
  ss << "tx_grant";
  processEvent(ss.str());
} /* TetraLogic::handleTxGrant */


std::string TetraLogic::getTEI(std::string issi)
{
  stringstream ss;
  char is[9];

  if (issi.length() < 17)
  {
    sprintf(is, "%08d", atoi(issi.c_str()));
    ss << mcc << mnc << is;
  }
  return ss.str();
} /* TetraLogic::toTEI */


void TetraLogic::handleStateSds(std::string m_message)
{
  stringstream ss;

  if (debug)
  {
     cout << "State Sds received: " << m_message << endl;        
  }
  
  if (state_sds[m_message].empty())
  {
    // process macro, if defined
    injectDtmf(m_message, 10);
  }
} /* TetraLogic::handleStateSds */


std::string TetraLogic::getNextStr(std::string& h)
{
  size_t f;
  std::string t = h.substr(0, f = h.find(','));
  if (f != string::npos)
  {
    h.erase(0, f + 1);    
  }
  return t;
} /* TetraLogic::getNextStr */


int TetraLogic::getNextVal(std::string& h)
{
  size_t f;
  int t = atoi(h.substr(0, f = h.find(',')).c_str());
  if (f != string::npos)
  {
    h.erase(0, f + 1);
  }
  return t;
} /* TetraLogic::getNextVal */


void TetraLogic::handleGroupcallEnd(std::string message)
{
  stringstream ss;
  ss << "groupcall_end";
  processEvent(ss.str());
  Qso.members.clear();
} /* TetraLogic::handleGroupcallEnd */


// Down Transmission Ceased +CDTXC
// +CDTXC: 1,0
void TetraLogic::handleCallEnd(std::string message)
{
  // update Qso information, set time of activity
  struct tm *utc;
  time_t rawtime = time(NULL);
  utc = gmtime(&rawtime);
  Qso.stop = utc;

  squelchOpen(false);  // close Squelch

  stringstream ss;
  ss << "call_end";
  processEvent(ss.str());

  // send call/qso end to aprs network
  if (LocationInfo::has_instance())
  {
    std::string m_aprsmesg = aprspath;    
    m_aprsmesg += "Qso ended (";
    for (const auto &it : Qso.members)
    {
      m_aprsmesg += it;
      m_aprsmesg += ",";
    }
    m_aprsmesg.pop_back();
    m_aprsmesg += ")";

    if (debug)
    {
      cout << m_aprsmesg << endl;
    }
    LocationInfo::instance()->update3rdState(userdata[Qso.tei].call, m_aprsmesg);
  }
} /* TetraLogic::handleCallEnd */


void TetraLogic::sendPei(std::string cmd)
{
  cmd += "\r";
  pei->write(cmd.c_str(), cmd.length());

  if (debug)
  {
    cout << "sending " << cmd << endl;
  }

  peiComTimer.reset();
  peiComTimer.setEnable(true);
} /* TetraLogic::sendPei */


void TetraLogic::onComTimeout(Async::Timer *timer)
{
  stringstream ss;
  ss << "peiCom_timeout";
  processEvent(ss.str());
  peistate = TIMEOUT;
} /* TetraLogic::onPeiTimeout */


void TetraLogic::onPeiActivityTimeout(Async::Timer *timer)
{
  sendPei("AT");
  peirequest = CHECK_AT;
  peiActivityTimer.reset();
} /* TetraLogic::onPeiTimeout */


void TetraLogic::tgUpTimeout(Async::Timer *tgUpTimer)
{
    
} /* TetraLogic::tgUpTimeout */

/*
  Create a confirmation sds and sends them to the Tetra radio
  that want to register
*/
void TetraLogic::cfmSdsReceived(std::string tei)
{
   std::string msg("821000FF");
   std::string sds;
   
   bool ret = createSDS(sds, tei, msg);
   if (ret)
   {
     sendPei(sds);
     stringstream ss;
     ss << "register_station " << tei;
     processEvent(ss.str());
   }
   else
   {
     cout << "*** ERROR: sending confirmation Sds" << endl;
   }
} /* TetraLogic::cfmSdsReceived */


// format of inject a Sds into SvxLink/TetraLogic
// TEI,Message
void TetraLogic::sdsPtyReceived(const void *buf, size_t count)
{
  const char *buffer = reinterpret_cast<const char*>(buf);
  std::string injmessage = buffer;
  std::string m_tei = getNextStr(injmessage);
  std::string sds;

  if(!createSDS(sds, m_tei, injmessage))
  {
    cout << "*** ERROR: creating Sds" << endl;
    return;
  }

  // out the new Sds int a queue...
  Sds m_Sds;
  m_Sds.tei = m_tei;
  m_Sds.content = injmessage;
  m_Sds.message = sds;
  m_Sds.type = 1;

  // update last activity of Sds
  struct tm *utc;
  time_t rawtime = time(NULL);
  utc = gmtime(&rawtime);
  m_Sds.tos = utc;

  int m_t = pending_sds.size()+1;
  pending_sds[m_t] = m_Sds;

} /* TetraLogic::sdsPtyReceived */


/*
+CTSDSR: 12,2629143,0,262905,0,84
0A008CACAA480A120201D0
DL1xxx-9>APRS,qAR,DB0xxx-10:!5119.89N/01221.83E>
*/
std::string TetraLogic::createAprsLip(std::string mesg)
{
  /*
    # Protocol identifier
    # 0x03 = simple GPS
    # 0x0a = LIP
    # 0x83 = Complex SDS-TL GPS message transfer
  */
   std::string t;
   return t;
} /* TetraLogic::handleLipSds */


int TetraLogic::handleMessage(std::string mesg)
{
  int retvalue = INVALID;
  typedef std::map<std::string, int> Mregex;
  Mregex mre;

  map<string, int>::iterator rt;

  mre["^OK"]                                      = OK;
  mre["^\\+CME ERROR"]                            = ERROR;
  mre["^\\+CTSDSR:"]                              = SDS;
  mre["^\\+CTICN:"]                               = GROUPCALL_BEGIN;
  mre["^\\+CTCR:"]                                = GROUPCALL_RELEASED;
  mre["^\\+CTCC:"]                                = CALL_CONNECT;
  mre["^\\+CDTXC:"]                               = CALL_END;
  mre["^\\+CTXG:"]                                = TX_GRANT;
  mre["^\\+CTXD:"]                                = TX_DEMAND;
  mre["^\\+CTXI:"]                                = TX_INTERRUPT;
  mre["^\\+CTXW:"]                                = TX_WAIT;
  mre["^0A00"]                                    = LIP_SDS;
  mre["^0300"]                                    = SIMPLE_LIP_SDS;
  mre["^8300"]                                    = COMPLEX_SDS;
  mre["^0A30000000000007FFE810"]                  = REGISTER_TEI;
  mre["^8[0-9A-F]{3}$"]                           = STATE_SDS;
  mre["^8204[0-9A-F]{2,}$"]                       = TEXT_SDS;
  mre["^\\+CTOM: [0-9]$"]                         = OP_MODE;

  for (rt = mre.begin(); rt != mre.end(); rt++)
  {
    if (rmatch(mesg, rt->first))
    {
      retvalue = rt->second;
      break;
    }
  }

  return retvalue;
} /* TetraLogic::handleMessage */


void TetraLogic::getOpMode(std::string opmode)
{
  int t = atoi(opmode.erase(0,6).c_str());
  cout << "+++ New Tetra mode: " << OpMode[t] << endl;
} /* TetraLogic::getOpMode */


bool TetraLogic::rmatch(std::string tok, std::string pattern)
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
} /* TetraLogic::rmatch */


/*
 * This file has not been truncated
 */

