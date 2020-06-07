/**
@file	 TetraLogic.cpp
@brief   Contains a Tetra logic SvxLink core implementation
@author  Tobias Blomberg / SM0SVX & Adi Bier / DL1HRC
@date	 2020-05-27

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2018 Tobias Blomberg / SM0SVX

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
#define TXGRANT 5
#define SDS 6
#define SDS_MESSAGE 7
#define ERROR35 8
#define CALL_PERMIT 9
#define CALL_END 10
#define GROUPCALL_RELEASED 11
#define LIP_SDS 12
#define REGISTER_TEI 13
#define STATE_SDS 14


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
    debug(false),
    peiComTimer(1000, Timer::TYPE_ONESHOT, false),
    peiActivityTimer(10000, Timer::TYPE_ONESHOT, true)

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
} /* TetraLogic::~TetraLogic */


bool TetraLogic::initialize(void)
{
  bool isok = true;
  if (!Logic::initialize())
  {
    isok = false;;
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
    aprspath = ">APRS,qAR";
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
     cout << "Warning: Missing parameter " << name() << "/PORT, "
          << port << endl;
  }

  if (!cfg().getValue(name(), "BAUD", baudrate))
  {
     cout << "Warning: Missing parameter " << name() << "/BAUD, guess "
          << baudrate << endl;
  }

  // read infos of tetra users configured in svxlink.conf
  string user_section;
  if (cfg().getValue(name(), "TETRA_USERS", user_section))
  {
    list<string> user_list = cfg().listSection(user_section);
    list<string>::iterator ulit;
    User m_user;

    for (ulit=user_list.begin(); ulit!=user_list.end(); ++ulit)
    {
      cfg().getValue(user_section, *ulit, value);
      if ((*ulit).length() != 17)
      {
        cout << "*** ERROR: Wrong length of TEI in TETRA_USERS definition,"
             << " should have 17 digits (MCC[4] MNC[5] ISSI[8]), e.g. "
             << "09011638312345678" << endl;
        isok = false;
      }
      else
      {
        m_user.call = getNextStr(value);
        m_user.name = getNextStr(value);
        getNextStr(value).copy(m_user.aprs_icon, 2);
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
      cout << *slit << "=" << value << endl;
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
    cerr << "*** ERROR: Opening serial port " << name() << "/" << port << endl;
    isok = false;
  }

  peirequest = INIT;
  initPei();

  rxValveSetOpen(true);
  setTxCtrlMode(Tx::TX_AUTO);

  processEvent("startup");

  return isok;

} /* TetraLogic::initialize */



/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/

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
    enableRgrSoundTimer(false);
    if (mute_tx_on_rx)
    {
      setTxCtrlMode(Tx::TX_OFF);
    }
  }

  Logic::squelchOpen(is_open);

} /* TetraLogic::squelchOpen */


void TetraLogic::transmitterStateChange(bool is_transmitting)
{
  if (mute_rx_on_tx)
  {
    rx().setMuteState(is_transmitting ? Rx::MUTE_ALL : Rx::MUTE_NONE);
  }
  Logic::transmitterStateChange(is_transmitting);
} /* TetraLogic::transmitterStateChange */




/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/

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
  size_t found, found2;

  peistream += buf;

  if ((found = peistream.find("\r\n")) == string::npos)
  {
    return;
  }

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

  found2 = peistream.find("\r\n", found + 1);
  if (found != string::npos && found2 != string::npos)
  {
    m_message = peistream.substr(found + 2, found2 - found - 2);
    peistream.erase(found, found2 + 2);
  }
  else if (found != string::npos && found > 2)
  {
    m_message = peistream.substr(0, found);
    peistream.erase(0, found + 2);
  }
  else
  {
    return;
  }

  cout << "message >" << m_message << "<" << endl;

  stringstream ss;
  int response = handleMessage(m_message);

  switch (response)
  {
    case OK:
      peistate = OK;
      break;

    case ERROR:
      peistate = ERROR;
      break;

    case GROUPCALL_BEGIN:
      Logic::squelchOpen(true);
      handleGroupcallBegin(m_message);
      break;

    case GROUPCALL_END:
      handleGroupcallEnd(m_message);
      break;

    case CALL_END:
      Logic::squelchOpen(false);
      handleCallEnd(m_message);
      break;

    case SDS:
      wait4sds = true;
      handleSdsHeader(m_message);
      break;

    case STATE_SDS:
      handleStateSds(m_message);
      wait4sds = false;
      break;

    case SDS_MESSAGE:
      if (!wait4sds) break;
      handleSdsMessage(m_message);
      break;

    default:
      break;
  }

  if (peirequest == INIT)
  {
    initPei();
  }

} /* TetraLogic::onCharactersReceived*/


/*
TETRA Incoming Call Notification +CTICN

+CTICN: <CC instance >, <call status>, <AI service>,
[<calling party identity type>], [<calling party identity>],
[<hook>], [<simplex>], [<end to end encryption>],
[<comms type>], [<slots/codec>], [<called party identity type>],
[<called party identity>], [<priority level>]

 Example:
 +CTICN: 1,0,0,5,09011638300023404,1,1,0,1,1,5,09011638300000001,0
*/
void TetraLogic::handleGroupcallBegin(std::string message)
{

  if (message.length() < 65)
  {
    cout << "*** No valid +CTICN response, message to short" << endl;
    return;
  }

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
  Qso.mebmers.push_back(userdata[o_tei].call);
  
  // callup tcl event
  ss << "groupcall_begin " << t_ci.o_issi << " " << t_ci.d_issi;
  processEvent(ss.str());

  // send group info to aprs network 
  if (LocationInfo::has_instance())
  {
    ss.clear();
    ss << userdata[o_tei].call << aprspath << "Groupcall initiated: " 
       << t_ci.d_issi << " -> " << t_ci.d_issi;
    std::string  m_aprsmesg = ss.str();
    LocationInfo::instance()->update3rdState(userdata[o_tei].call, m_aprsmesg);
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
 +CTSDSR: 12,23404,0,23401,0,112
 82040801476A61746A616A676A61
*/
void TetraLogic::handleSdsHeader(std::string sds_head)
{
  stringstream ss;
  sds_head.erase(0,9);
  Sds m_sds;

  struct tm *utc;
  time_t rawtime = time(NULL);
  utc = gmtime(&rawtime);
  m_sds.tos = utc;     // last activity

  m_sds.type = getNextVal(sds_head); // type of sds
  m_sds.o_issi = getTEI(getNextStr(sds_head)); // sender ISSI
  getNextVal(sds_head);

  // destination ISSI
  pending_sds[getNextStr(sds_head)] = m_sds;

  // update last activity of sender
  userdata[m_sds.o_issi].last_activity = utc;
  
  ss << "sds_header_received " << m_sds.o_issi << " " << m_sds.type;
  processEvent(ss.str());

} /* TetraLogic::getTypeOfService */


void TetraLogic::handleSdsMessage(std::string sds_message)
{
  stringstream ss;  
  ss << "sds_message " << decodeSDS(sds_message);
  processEvent(ss.str());
} /* TetraLogic::handleSdsMessage */


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

  if (state_sds[m_message].empty())
  {
    // process macro, if defined
    injectDtmf(m_message, 10);
  }
  else
  {
    cout << "sending APRS-Info via LocatioInfo "
    << m_message << endl;
  }
  ss << "state_sds_received";
  processEvent(ss.str());
} /* TetraLogic::handleStateSds */


std::string TetraLogic::getNextStr(std::string& h)
{
  size_t f;
  std::string t = h.substr(0, f = h.find(','));
  h.erase(0, f + 1);
  return t;
} /* TetraLogic::getNextStr */


int TetraLogic::getNextVal(std::string& h)
{
  size_t f;
  int t = atoi(h.substr(0, f = h.find(',')).c_str());
  h.erase(0, f + 1);
  return t;
} /* TetraLogic::getNextVal */


void TetraLogic::handleGroupcallEnd(std::string message)
{
  stringstream ss;
  ss << "groupcall_end";
  processEvent(ss.str());
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

  stringstream ss;
  ss << "call_end";
  processEvent(ss.str());
  
  // send call/qso end to aprs network 
  if (LocationInfo::has_instance())
  {
    std::string  m_aprsmesg = "Qso ended (";
    for (const auto &it : Qso.mebmers) m_aprsmesg += it;
    m_aprsmesg += ")";
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
  mre["^\\+CTCC:"]                                = CALL_PERMIT;
  mre["^\\+CDTXC:"]                               = CALL_END;
  mre["^\\+CTCR:"]                                = GROUPCALL_RELEASED;
  mre["^0A00"]                                    = LIP_SDS;
  mre["^0A30000000000007FFE810"]                  = REGISTER_TEI;
  mre["^8[0-9A-F]{3}$"]                           = STATE_SDS;
  mre["^[0-9A-F]{2,}$"]                           = SDS_MESSAGE;

  for (rt = mre.begin(); rt != mre.end(); rt++)
  {
    if (rmatch(mesg, rt->first))
    {
      retvalue = rt->second;
      break;
    }
  }

  return retvalue;
}


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

