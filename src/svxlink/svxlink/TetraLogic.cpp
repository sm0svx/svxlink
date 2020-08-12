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
#define CALL_BEGIN 3
#define GROUPCALL_END 4

#define SDS 6
#define TEXT_SDS 7
#define CNUMF 8
#define CALL_CONNECT 9
#define TRANSMISSION_END 10
#define CALL_RELEASED 11
#define LIP_SDS 12
#define REGISTER_TSI 13
#define STATE_SDS 14
#define OP_MODE 15
#define TRANSMISSION_GRANT 16
#define TX_DEMAND 17
#define TX_WAIT 18
#define TX_INTERRUPT 19
#define SIMPLE_LIP_SDS 20
#define COMPLEX_SDS 21
#define MS_CNUM 22
#define WAP_PROTOCOL 23
#define SIMPLE_TEXT_SDS 24
#define ACK_SDS 25
#define SDS_ID 26
#define CONCAT_SDS 27

#define DMO_OFF 7
#define DMO_ON 8

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
  debug(false), talkgroup_up(false), sds_when_dmo_on(false),
  sds_when_dmo_off(false), sds_when_proximity(false),
  peiComTimer(1000, Timer::TYPE_ONESHOT, false),
  peiActivityTimer(10000, Timer::TYPE_ONESHOT, true),
  proximity_warning(3.1), time_between_sds(3600)
{
  peiComTimer.expired.connect(mem_fun(*this, &TetraLogic::onComTimeout));
  peiActivityTimer.expired.connect(mem_fun(*this, &TetraLogic::onPeiActivityTimeout));
} /* TetraLogic::TetraLogic */


TetraLogic::~TetraLogic(void)
{
  delete pei;
  pei = 0;
  peiComTimer = 0;
  peiActivityTimer = 0;
  delete call;
  delete tetra_modem_sql;
  tetra_modem_sql = 0;
} /* TetraLogic::~TetraLogic */


bool TetraLogic::initialize(void)
{
  static SquelchSpecificFactory<SquelchTetra> tetra_modem_factory;
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
  // Welcome message to new users
  if (!cfg().getValue(name(), "INFO_SDS", infosds))
  {
    infosds = "Welcome TETRA-User@";
    infosds += callsign(); 
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

  if (cfg().getValue(name(), "DEFAULT_APRS_ICON", value))
  {
    if (value.length() != 2)
    {
      isok = false;
      cout << "*** ERROR: " << name() << "/DEFAULT_APRS_ICON "
           << "must have 2 characters, e.g. '/e' or if the backslash or "
           << "a comma is used it has to be encoded with an additional " 
           << "'\', e.g. " << "DEFAULT_APRS_ICON=\\r" << endl;
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

  list<string>::iterator slit;

  // read infos of tetra users configured in svxlink.conf
  string user_section;
  if (cfg().getValue(name(), "TETRA_USERS", user_section))
  {
    list<string> user_list = cfg().listSection(user_section);
    User m_user;

    for (slit=user_list.begin(); slit!=user_list.end(); slit++)
    {
      cfg().getValue(user_section, *slit, value);
      if ((*slit).length() != 17)
      {
        cout << "*** ERROR: Wrong length of TSI in TETRA_USERS definition, "
             << "should have 17 digits (MCC[4] MNC[5] ISSI[8]), e.g. "
             << "09011638312345678" << endl;
        isok = false;
      }
      else
      {
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
        m_user.comment = getNextStr(value); // comment for each user
        struct tm mtime = {0}; // set default date/time 31.12.1899
        m_user.last_activity = mktime(&mtime);
        m_user.sent_last_sds = mktime(&mtime);
        userdata[*slit] = m_user;
      }
    }
  }

  // define sds messages send to user when received Sds's from him due to
  // state changes
  std::string sds_useractivity;
  if (cfg().getValue(name(), "SDS_ON_USERACTIVITY", sds_useractivity))
  {
    list<string> activity_list = cfg().listSection(sds_useractivity); 
    for (slit=activity_list.begin(); slit!=activity_list.end(); slit++)
    {
      cfg().getValue(sds_useractivity, *slit, value);
      if (value.length() > 100)
      {
        cout << "+++ WARNING: Message to long (>100 digits) at " << name() 
             << "/" << sds_useractivity << ": " << (*slit) 
             << ". Cutting message.";
        sds_on_activity[atoi((*slit).c_str())] = value.substr(0,100);
      }
      else 
      {
        sds_on_activity[atoi((*slit).c_str())] = value;
      }
    }
  }

  // a section that combine SDS and a command:
  // 8055=1234
  std::string sds_to_cmd;
  if (cfg().getValue(name(), "SDS_TO_COMMAND", sds_to_cmd))
  {
    list<string> sds2cmd_list = cfg().listSection(sds_to_cmd);
    for (slit=sds2cmd_list.begin(); slit!=sds2cmd_list.end(); slit++)
    {
      cfg().getValue(sds_to_cmd, *slit, value);
      if ( (*slit).length() != 4)
      {
        cout << "***ERROR: Sds length in section " << name() 
             << "/SDS_TO_COMMAND to long (" << (*slit) 
             << "), must be 4, ignoring" << endl;
      } 
      else
      {
        sds_to_command[*slit] = value;
        cout << *slit << "=" << value << endl;
      }
    }
  }
  
  // define if Sds's send to all other users if the state of one user is 
  // changed at the moment only: DMO_ON, DMO_OFF, PROXIMITY
  std::string sds_othersactivity;
  if (cfg().getValue(name(), "SDS_TO_OTHERS_ON_ACTIVITY", sds_othersactivity))
  {
    string::iterator comma;
    string::iterator begin = sds_othersactivity.begin();
    do
    {
      comma = find(begin, sds_othersactivity.end(), ',');
      string item;
      if (comma == sds_othersactivity.end())
      {
        item = string(begin, sds_othersactivity.end());
      }
      else
      {
        item = string(begin, comma);
        begin = comma + 1;
      }
      if (item == "DMO_ON")
      {
        sds_when_dmo_on = true;
      }
      else if (item == "DMO_OFF")
      {
        sds_when_dmo_off = true;
      }
      else if (item == "PROXIMITY")
      {
        sds_when_proximity = true;
      }
    } while (comma != sds_othersactivity.end());
  }

  // read info of tetra state to receive SDS's
  std::string status_section;
  if (cfg().getValue(name(), "TETRA_STATUS", status_section))
  {
    list<string> state_list = cfg().listSection(status_section);
    for (slit=state_list.begin(); slit!=state_list.end(); slit++)
    {
      cfg().getValue(status_section, *slit, value);
      if( (*slit).length() != 4)
      {
        cout << "***ERROR: Sds length in section "<< name() 
             << "/TETRA_STATUS to long (" << (*slit)
             << "), must be 4, ignoring" << endl;
      }
      else
      {
        state_sds[*slit] = value;
      }
    }
  }
    
  if (cfg().getValue(name(), "PROXIMITY_WARNING", value))
  {
    proximity_warning = atof(value.c_str());  
  }

  if (cfg().getValue(name(), "TIME_BETWEEN_SDS", value))
  {
    time_between_sds = atoi(value.c_str());  
  }

  // create the special Tetra-squelch
  Squelch *squelch_det = createSquelch("TETRA_SQL");
  tetra_modem_sql = dynamic_cast<SquelchTetra*>(squelch_det);
  if (tetra_modem_sql != nullptr)
  {
    cout << "Creating tetra specific Sql ok" << endl;
  }
  else
  {
    cout << "*** ERROR creating Tetra specific squelch" << endl;
    isok = false;
  }

  // init the Pei device
  if (!cfg().getValue(name(), "INIT_PEI", initstr))
  {
    cout << "Warning: Missing parameter " << name()
         << "/INIT_PEI, using defaults" << endl;
  }
  SvxLink::splitStr(initcmds, initstr, ";");

  m_cmds = initcmds;

  pei = new Serial(port);
  pei->setParams(baudrate, Serial::PARITY_NONE, 8, 1, Serial::FLOW_HW);
  pei->charactersReceived.connect(
      	  mem_fun(*this, &TetraLogic::onCharactersReceived));

  if (!pei->open(true))
  {
    cerr << "*** ERROR: Opening serial port " << name() << "/\""
         << port << "\"" << endl;
    return false;
  }
  sendPei("\r\n");

  peirequest = INIT;
  initPei();
  
  rxValveSetOpen(true);
  setTxCtrlMode(Tx::TX_AUTO);
  
  processEvent("startup");
  
  return isok;

} /* TetraLogic::initialize */


void TetraLogic::remoteCmdReceived(LogicBase* src_logic, const std::string& cmd)
{
  cout << "command received:" << cmd << endl;
} /* TetraLogic::remoteCmdReceived */


/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/

 void TetraLogic::allMsgsWritten(void)
{
  Logic::allMsgsWritten();
  if (!talkgroup_up)
  {
    setTxCtrlMode(Tx::TX_AUTO);
  }
} /* TetraLogic::allMsgsWritten */

 
 void TetraLogic::audioStreamStateChange(bool is_active, bool is_idle)
{
  Logic::audioStreamStateChange(is_active, is_idle);  
} /* TetraLogic::audioStreamStateChange */

 
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
      cmd = "AT+CTXD=1,1";
      sendPei(cmd);
    }
  }
  else
  {
    cmd = "AT+CUTXC=1";
    sendPei(cmd);
  }
  
  if (mute_rx_on_tx)
  {
   // rx().setMuteState(is_transmitting ? Rx::MUTE_ALL : Rx::MUTE_NONE);
  }

  Logic::transmitterStateChange(is_transmitting);
} /* TetraLogic::transmitterStateChange */

 
void TetraLogic::squelchOpen(bool is_open)
{
  cout << name() << ": The squelch is " << (is_open ? "OPEN" : "CLOSED")
       << endl;

    // FIXME: A squelch open should not be possible to receive while
    // transmitting unless mute_rx_on_tx is false, in which case it
    // should be allowed. Commenting out the statements below.

  if (tx().isTransmitting())
  {
    return;
  }

  tetra_modem_sql->setSql(is_open);
  Logic::squelchOpen(is_open);

} /* TetraLogic::squelchOpen */


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
  std::string cmd;
  if (!m_cmds.empty())
  {
    cmd = *(m_cmds.begin());
    sendPei(cmd);
    m_cmds.erase(m_cmds.begin());
  }
  else if (peirequest == INIT)
  {
    cmd = "AT+CNUMF?"; // get the MCC,MNC,ISSI from MS
    sendPei(cmd);
  }
  else 
  {
    ss << "pei_init_finished";
    processEvent(ss.str());
  }
} /* TetraLogic::initPei */


void TetraLogic::onCharactersReceived(char *buf, int count)
{
  peiComTimer.setEnable(false);
  peiActivityTimer.reset();
  size_t found;

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

  while ((found = peistream.find("\r\n")) != string::npos)
  {
    if (found != 0)
    {
      handlePeiAnswer(peistream.substr(0, found));
    }
    peistream.erase(0, found+2);
  }

} /* TetraLogic::onCharactersReceived */


void TetraLogic::handlePeiAnswer(std::string m_message)
{

  if (debug)
  {
    cout << "From PEI:" << m_message << endl;
  }

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

    case CNUMF:
      handleCnumf(m_message);  
      break;

    case CALL_BEGIN:
      handleCallBegin(m_message);
      break;

    case TRANSMISSION_END:
      handleTransmissionEnd(m_message);
      break;

    case CALL_RELEASED:
      handleCallReleased(m_message);
      break;

    case SDS:
      handleSds(m_message);
      break;
      
    case TEXT_SDS:
    case SIMPLE_TEXT_SDS:
    case STATE_SDS:
    case ACK_SDS:
    case COMPLEX_SDS:
    case CONCAT_SDS:
    case LIP_SDS:
      handleSdsMsg(m_message);
      break;

    case SDS_ID:
      // +CMGS: <SDS Instance>[, <SDS status> [, <message reference>]]
      // sds state send be MS
      break;
      
    case TX_DEMAND:
      break;

    case TRANSMISSION_GRANT:
      handleTxGrant(m_message);
      break;

    case CALL_CONNECT:
      break;

    case OP_MODE:
      getOpMode(m_message);
      break;

    case INVALID:
      cout << "+++ Pei answer not known, ignoring ;)" << endl;

    default:
      break;
  }

  if (peirequest == INIT && (response == OK || response == ERROR))
  {
    initPei();
  }

} /* TetraLogic::handlePeiAnswer */


void TetraLogic::initGroupCall(int gc_gssi)
{
  std::string cmd = "AT+CTSDC=0,0,0,1,1,0,1,1,0,0,0";
  sendPei(cmd);

  cmd = "ATD";
  cmd += to_string(gc_gssi);
  sendPei(cmd);
  
  stringstream ss;
  ss << "init_group_call " << to_string(gc_gssi);
  processEvent(ss.str());
} /* TetraLogic::initGroupCall */


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
void TetraLogic::handleCallBegin(std::string message)
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

  // split the message received from the Pei into single parameters
  // for further use, not all of them are interesting
  t_ci.instance = getNextVal(h);
  t_ci.callstatus = getNextVal(h);
  t_ci.aistatus = getNextVal(h);
  t_ci.origin_cpit = getNextVal(h);

  std::string o_tsi = getNextStr(h);
  t_ci.o_mcc = atoi(o_tsi.substr(0,4).c_str());
  t_ci.o_mnc = atoi(o_tsi.substr(4,5).c_str());
  t_ci.o_issi = atoi(o_tsi.substr(9,8).c_str());

  t_ci.hook = getNextVal(h);
  t_ci.simplex = getNextVal(h);
  t_ci.e2eencryption = getNextVal(h);
  t_ci.commstype = getNextVal(h);
  t_ci.codec = getNextVal(h);
  t_ci.dest_cpit = getNextVal(h);

  std::string d_tsi = getNextStr(h);
  t_ci.d_mcc = atoi(d_tsi.substr(0,4).c_str());
  t_ci.d_mnc = atoi(d_tsi.substr(4,5).c_str());
  t_ci.d_issi = atoi(d_tsi.substr(9,8).c_str());
  t_ci.prio = atoi(h.c_str());

  // store call specific data into a Callinfo struct
  callinfo[t_ci.o_issi] = t_ci;
  
  // check if the user is stored? no -> default
  std::map<std::string, User>::iterator iu = userdata.find(o_tsi);
  if (iu == userdata.end())
  {
    userdata[o_tsi].call = "NoCall";
    userdata[o_tsi].name = "NoName";
    userdata[o_tsi].comment = "NN";
    userdata[o_tsi].aprs_sym = t_aprs_sym;
    userdata[o_tsi].aprs_tab = t_aprs_tab;
    return;
  }
  
  userdata[o_tsi].last_activity = time(NULL);

  // store info in Qso struct
  Qso.tsi = o_tsi;
  Qso.start = time(NULL);
  
  std::list<std::string>::iterator it;
  it = find(Qso.members.begin(), Qso.members.end(), iu->second.call);
  if (it == Qso.members.end())
  {
    Qso.members.push_back(iu->second.call);
  }

  // callup tcl event
  ss << "groupcall_begin " << t_ci.o_issi << " " << t_ci.d_issi;
  processEvent(ss.str());

  // send group info to aprs network
  if (LocationInfo::has_instance())
  {
    stringstream m_aprsmesg;
    m_aprsmesg << aprspath << ">" << iu->second.call
               << " initiated groupcall: " << t_ci.o_issi 
               << " -> " << t_ci.d_issi;
    cout << " To APRS:" << m_aprsmesg.str() << endl;
    LocationInfo::instance()->update3rdState(iu->second.call, m_aprsmesg.str());
  }
} /* TetraLogic::handleCallBegin */


/*
 TETRA SDS Receive +CTSDSR

 CTSDSR unsolicited Result Codes
 +CTSDSR: <AI service>, [<calling party identity>],
 [<calling party identity type>], <called party identity>,
 <called party identity type>, <length>,
 [<end to end encryption>]<CR><LF>user data

 Example:
 +CTSDSR: 12,23404,0,23401,0,112
 (82040801476A61746A616A676A61)
*/
void TetraLogic::handleSds(std::string sds)
{
  sds.erase(0,9);  // remove "+CTSDSR: "
  
  // store header of sds for further handling
  pSDS.sdstype = getNextVal(sds);        // type of SDS (12)
  pSDS.fromtsi = getTSI(getNextStr(sds)); // sender Tsi (23404)
  getNextVal(sds);                     // (0)
  pSDS.totsi = getNextVal(sds);        // destination Issi
  getNextVal(sds);                     // (0)
  getNextVal(sds);                     // Sds length (112)
  pSDS.last_activity = time(NULL);
} /* TetraLogic::handleSds */


/*
 Handle the sds message
 Example:
 (+CTSDSR: 12,23404,0,23401,0,112)
 82040801476A61746A616A676A61
*/
void TetraLogic::handleSdsMsg(std::string sds)
{

  Sds m_sds;
  stringstream ss;
  std::string sds_txt;
  std::string t_sds;
  stringstream m_aprsinfo;
  std::map<string, string>::iterator it;
  std::map<int, string>::iterator oa;
  LipInfo lipinfo;

  int m_sdsid = pending_sds.size() + 1;

  m_sds.tos = pSDS.last_activity;      // last activity
  m_sds.direction = INCOMING;          // 1 = received
  m_sds.tsi = pSDS.fromtsi;
  
  // check if the user is stored? no -> default
  std::map<std::string, User>::iterator iu = userdata.find(m_sds.tsi);
  if (iu == userdata.end())
  {
    userdata[m_sds.tsi].call = "NoCall";
    userdata[m_sds.tsi].name = "NoName";
    userdata[m_sds.tsi].aprs_sym = t_aprs_sym;
    userdata[m_sds.tsi].aprs_tab = t_aprs_tab;
    createSDS(t_sds, getISSI(m_sds.tsi), infosds);
    sendPei(t_sds);
    if (debug) cout << "+++ sending welcome sds to " << t_sds << endl;
    return;
  }

   // update last activity of sender
  userdata[m_sds.tsi].last_activity = time(NULL);

  int m_sdstype = handleMessage(sds);
  switch (m_sdstype)
  {
    case LIP_SDS:
      handleLipSds(sds, lipinfo);
      m_aprsinfo << "!" << dec2nmea_lat(lipinfo.latitude)  
         << iu->second.aprs_sym << dec2nmea_lon(lipinfo.longitude)
         << iu->second.aprs_tab << iu->second.name << ", "
         << iu->second.comment;
      ss << "lip_sds_received " << m_sds.tsi << " " 
         << lipinfo.latitude << " " << lipinfo.longitude;
      userdata[m_sds.tsi].lat = lipinfo.latitude;
      userdata[m_sds.tsi].lon = lipinfo.longitude;
      userdata[m_sds.tsi].reasonforsending = lipinfo.reasonforsending;
      if (debug)
      { 
        cout << " To APRS:" << m_aprsinfo.str() << endl;
        cout << "         " << m_sds.tsi << ":" 
             << ReasonForSending[lipinfo.reasonforsending] << endl;
      }
      // Power-On -> send sds
      oa = sds_on_activity.find(lipinfo.reasonforsending);
      if (oa != sds_on_activity.end())
      {
        if (debug)
        {
          cout << "Send SDS:" << getISSI(m_sds.tsi) << ", " << 
                 oa->second << endl;
        } 
        createSDS(t_sds, getISSI(m_sds.tsi), oa->second);
        sendPei(t_sds);
      }
      // proximity, dmo on, dmo off?
      sendInfoSds(m_sds.tsi, lipinfo.reasonforsending);
      break;

    case STATE_SDS:
      handleStateSds(sds);
      userdata[m_sds.tsi].state = sds;
      //cfmSdsReceived(m_sds.tsi);
      m_aprsinfo << ">" << "State:";
      if ((it = state_sds.find(sds)) != state_sds.end())
      {
        m_aprsinfo << it->second;
      }        
      m_aprsinfo << " (" << sds << ")";

      ss << "state_sds_received " << m_sds.tsi << " " << sds;
      break;
      
    case TEXT_SDS:
      sds_txt = handleTextSds(sds);
      m_aprsinfo << ">" << sds_txt;
      cfmTxtSdsReceived(sds, m_sds.tsi);
      ss << "text_sds_received " << m_sds.tsi << " \"" << sds_txt << "\"";
      break;

    case SIMPLE_TEXT_SDS:
      sds_txt = handleSimpleTextSds(sds);
      m_aprsinfo << ">" << sds_txt;
      cfmSdsReceived(m_sds.tsi);
      ss << "text_sds_received " << m_sds.tsi << " \"" << sds_txt << "\"";
      break;
      
    case ACK_SDS:
      // +CTSDSR: 12,23404,0,23401,0,32
      // 82100002
      // sds msg received by MS from remote
      ss << "sds_receiced_ack " << sds;
      break;
      
    case REGISTER_TSI:
      ss << "register_tsi " << m_sds.tsi;
      cfmSdsReceived(m_sds.tsi);
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
    LocationInfo::instance()->update3rdState(userdata.find(m_sds.tsi)->second.call, 
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
} /* TetraLogic::handleTextSds */


std::string TetraLogic::handleSimpleTextSds(std::string m_message)
{
  if (m_message.length() > 4) m_message.erase(0,4);  // delete 0201
  return decodeSDS(m_message);
} /* TetraLogic::handleSimpleTextSds */


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


std::string TetraLogic::getTSI(std::string issi)
{
  stringstream ss;
  char is[18];
  int len = issi.length(); 
  
  if (len < 9)
  {
    sprintf(is, "%08d", atoi(issi.c_str()));
    ss << mcc << mnc << is;  
  }
  else if (issi.substr(0,1) != "0")
  {
    sprintf(is, "%04d%05d%s", atoi(issi.substr(0,3).c_str()),
                              atoi(issi.substr(3,len-11).c_str()),
                              issi.substr(-8,8).c_str());
    ss << is;
  } 
  else 
  {
    sprintf(is, "%04d%05d%s", atoi(issi.substr(0,4).c_str()),
                              atoi(issi.substr(4,len-12).c_str()),
                              issi.substr(-8,8).c_str());
    ss << is;
  }

  return ss.str();
} /* TetraLogic::getTSI */


std::string TetraLogic::getISSI(std::string tsi)
{
  stringstream t_issi;
  if (tsi.length() == 17)
  {
    t_issi << atoi(tsi.substr(9,8).c_str());  
  }
  else
  {
    t_issi << tsi;  
  }
  return t_issi.str();
} /* TetraLogic::getISSI */


void TetraLogic::handleStateSds(std::string m_message)
{
  stringstream ss;

  if (debug)
  {
    cout << "State Sds received: " << m_message << endl;        
  }
  
  std::map<string, string>::iterator it = sds_to_command.find(m_message);
  
  if (it != sds_to_command.end())
  {
    // to connect/disconnect Links
    ss << it->second << "#";
    injectDtmf(ss.str(), 10);
  }
  else if (state_sds.find(m_message) != state_sds.end())
  {
    // process macro, if defined
    ss << "D" << m_message << "#";  
    injectDtmf(ss.str(), 10);
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


// Down Transmission Ceased +CDTXC
// +CDTXC: 1,0
void TetraLogic::handleTransmissionEnd(std::string message)
{
  squelchOpen(false);  // close Squelch
  stringstream ss;
  ss << "groupcall_end";
  processEvent(ss.str());
} /* TetraLogic::handleTransmissionEnd */


// TETRA Call Release +CTCR
// +CTCR: 1,13
void TetraLogic::handleCallReleased(std::string message)
{
  // update Qso information, set time of activity
  Qso.stop = time(NULL);

  stringstream ss;
  getNextStr(message);

  ss << "call_end \"" << DisconnectCause[getNextVal(message)] << "\"";
  processEvent(ss.str());
  
  // send call/qso end to aprs network
  if (LocationInfo::has_instance())
  {
    std::string m_aprsmesg = aprspath;    
    
    if (!Qso.members.empty())
    {
      m_aprsmesg += "Qso ended (";
      for (const auto &it : Qso.members)
      {
        m_aprsmesg += it;
        m_aprsmesg += ",";
      }
      m_aprsmesg.pop_back();
      m_aprsmesg += ")";
    }
    else
    {
      m_aprsmesg += "Transmission ended";
    }

    if (debug)
    {
      cout << m_aprsmesg << endl;
    }
    LocationInfo::instance()->update3rdState(userdata[Qso.tsi].call, m_aprsmesg);
  }

  talkgroup_up = false;
  Qso.members.clear();
  
} /* TetraLogic::handleCallReleased */


void TetraLogic::sendPei(std::string cmd)
{

  // a sdsmsg must end with 0x1a
  if (cmd.at(cmd.length()-1) != 0x1a)
  {
    cmd += "\r";
  }

  pei->write(cmd.c_str(), cmd.length());

  if (debug)
  {
    cout << "  To PEI:" << cmd << endl;
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
} /* TetraLogic::onComTimeout */


void TetraLogic::onPeiActivityTimeout(Async::Timer *timer)
{
  sendPei("AT");
  peirequest = CHECK_AT;
  peiActivityTimer.reset();
} /* TetraLogic::onPeiActivityTimeout */


/*
  Create a confirmation sds and sends them to the Tetra radio
*/
void TetraLogic::cfmSdsReceived(std::string tsi)
{
   std::string msg("OK");  // confirm a sds received
   std::string sds;

   if (createSDS(sds, getISSI(tsi), msg))
   {
     sendPei(sds);
   }
   else
   {
     cout << "*** ERROR: sending confirmation Sds" << endl;
   }
} /* TetraLogic::cfmSdsReceived */


/* +CTSDSR: 12,23404,0,23401,0,96, 82041D014164676A6D707477 */
void TetraLogic::cfmTxtSdsReceived(std::string message, std::string tsi)
{
   if (message.length() < 8) return;
   
   std::string msg("821000");  // confirm a sds received
   msg += message.substr(4,2);
   
   std::string sds;
   if (debug)
   {
     cout << "sending confirmation Sds to " << tsi << endl;
   }

   if (createCfmSDS(sds, getISSI(tsi), msg))
   {
     sendPei(sds);
   }
   else
   {
     cout << "*** ERROR: sending confirmation Sds" << endl;
   }
} /* TetraLogic::cfmSdsReceived */


void TetraLogic::handleCnumf(std::string m_message)
{

  size_t f = m_message.find("+CNUMF: ");
  if (f != string::npos)
  {
    m_message.erase(0,8);
  }
 // e.g. +CNUMF: 6,09011638300023401
  
  short m_numtype = getNextVal(m_message);
  if (debug) cout << "<num type> is " << m_numtype << " (" 
               << TetraNumType[m_numtype] << ")" << endl; 
  if (m_numtype == 6)
  {
    if (mcc != m_message.substr(0,4)) 
    {
      cout << "*** ERROR: wrong MCC in MS, will not work! " << mcc << "!=" 
           << m_message.substr(0,4) << endl;
    }
    if (mnc != m_message.substr(4,5)) {
      cout << "*** ERROR: wrong MNC in MS, will not work! " << mnc << "!=" 
           << m_message.substr(4,5) << endl;
    }
    if (atoi(issi.c_str()) != atoi(m_message.substr(9,8).c_str())) {
      cout << "*** ERROR: wrong ISSI in MS, will not work! " << issi <<"!=" 
           << m_message.substr(9,8) << endl;
    }
  }
  
  peirequest = INIT_COMPLETE;
} /* TetraLogic::handleCnumf */


// format of inject a Sds into SvxLink/TetraLogic
// tsi,Message
void TetraLogic::sdsPtyReceived(const void *buf, size_t count)
{
  const char *buffer = reinterpret_cast<const char*>(buf);
  std::string injmessage = "";
  
  for (size_t i=0; i<count-1; i++)
  {
    injmessage += *buffer++;
  }
  std::string m_tsi = getNextStr(injmessage);
  std::string sds;

  if(!createSDS(sds, getISSI(m_tsi), injmessage))
  {
    std::string s = "*** ERROR: creating Sds to ";
    s += m_tsi;
    if (debug) cout << s << endl;
    sds_pty->write(s.c_str(), s.size());
    return;
  }

  // sending message to PEI
  sendPei(sds);
  
  // put the new Sds int a queue...
  Sds m_Sds;
  m_Sds.tsi = m_tsi;
  m_Sds.content = injmessage;
  m_Sds.message = sds;
  m_Sds.direction = OUTGOING;
  m_Sds.type = TEXT;

  // update last activity of Sds
  m_Sds.tos = time(NULL);

  int m_t = pending_sds.size()+1;
  pending_sds[m_t] = m_Sds;
  
} /* TetraLogic::sdsPtyReceived */


void TetraLogic::sendInfoSds(std::string tsi, short reason)
{
  double timediff;
  float distancediff;
  stringstream ss;
  std::string t_sds;
  std::map<std::string, User>::iterator iu = userdata.find(tsi);
  if (iu == userdata.end()) return;
  
  for (std::map<std::string, User>::iterator t_iu = userdata.begin();
        t_iu != userdata.end(); t_iu++)
  {
    if (t_iu->first != tsi)
    {
      timediff = difftime(time(NULL), t_iu->second.last_activity);

      if (timediff >= time_between_sds)
      {
        distancediff = calcDistance(iu->second.lat, iu->second.lon,
                              t_iu->second.lat, t_iu->second.lon);

        ss.str("");
        ss << iu->second.call << " state change, ";
        if (sds_when_dmo_on && reason == DMO_ON)
        {
          ss << "DMO=on";
        } 
        else if (sds_when_dmo_off && reason == DMO_OFF)
        {
          ss << "DMO=off";  
        } 
        else if (sds_when_proximity && distancediff <= proximity_warning)
        {
          ss << "new distance" << distancediff << " km";
        }

        if (debug)
        {
          cout << ss.str() << endl;
        }

        createSDS(t_sds, getISSI(t_iu->first), ss.str());
        
         // put the new Sds int a queue...
        Sds m_Sds;
        m_Sds.tsi = t_iu->first;
        m_Sds.content = ss.str();
        m_Sds.message = t_sds;
        m_Sds.direction = OUTGOING;
        m_Sds.type = TEXT;
        m_Sds.tos = time(NULL);
        int m_t = pending_sds.size()+1;
        pending_sds[m_t] = m_Sds;

        // send SDS
        sendPei(t_sds);
        t_iu->second.sent_last_sds = time(NULL);
      }
    }
  }
} /* TetraLogic::sendInfoSds */


int TetraLogic::handleMessage(std::string mesg)
{
  int retvalue = INVALID;
  typedef std::map<std::string, int> Mregex;
  Mregex mre;

  map<string, int>::iterator rt;

  mre["^OK"]                                      = OK;
  mre["^\\+CME ERROR"]                            = ERROR;
  mre["^\\+CTSDSR:"]                              = SDS;
  mre["^\\+CTICN:"]                               = CALL_BEGIN;
  mre["^\\+CTCR:"]                                = CALL_RELEASED;
  mre["^\\+CTCC:"]                                = CALL_CONNECT;
  mre["^\\+CDTXC:"]                               = TRANSMISSION_END;
  mre["^\\+CTXG:"]                                = TRANSMISSION_GRANT;
  mre["^\\+CTXD:"]                                = TX_DEMAND;
  mre["^\\+CTXI:"]                                = TX_INTERRUPT;
  mre["^\\+CTXW:"]                                = TX_WAIT;
  mre["^\\+CNUM:"]                                = MS_CNUM;
  mre["^\\+CTOM: [0-9]$"]                         = OP_MODE;
  mre["^\\+CMGS:"]                                = SDS_ID;
  mre["^\\+CNUMF:"]                               = CNUMF;
  mre["^02"]                                      = SIMPLE_TEXT_SDS; 
  mre["^03"]                                      = SIMPLE_LIP_SDS;
  mre["^04"]                                      = WAP_PROTOCOL;
  mre["^0A"]                                      = LIP_SDS;
  mre["^8204"]                                    = TEXT_SDS;
  mre["^821000"]                                  = ACK_SDS;
  mre["^0C"]                                      = CONCAT_SDS;
  mre["^83"]                                      = COMPLEX_SDS;
  mre["^8[0-9A-F]{3}$"]                           = STATE_SDS;

  for (rt = mre.begin(); rt != mre.end(); rt++)
  {
    if (rmatch(mesg, rt->first))
    {
      retvalue = rt->second;
      return retvalue;
    }
  }

  return peistate;
} /* TetraLogic::handleMessage */


void TetraLogic::getOpMode(std::string opmode)
{
  if (opmode.length() > 6)
  {
    int t = atoi(opmode.erase(0,6).c_str());
    cout << "+++ New Tetra mode: " << OpMode[t] << endl;      
  }
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

