/**
@file	 AnalogPhoneLogic.cpp
@brief   Contains a class that implements a repeater controller
@author  Tobias Blomberg / SM0SVX
@date	 2004-04-24

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2009 Tobias Blomberg / SM0SVX

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

#include <sys/time.h>

#include <cstdio>
#include <stdio.h>
#include <string>
#include <iostream>
#include <cassert>
#include <cstdlib>
#include <sstream>
#include <algorithm>
#include <sigc++/sigc++.h>

/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncTimer.h>
#include <AsyncConfig.h>
#include <AsyncSerial.h>

#include <Rx.h>
#include <Tx.h>
#include <regex.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "AnalogPhoneLogic.h"


/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/

using namespace std;
using namespace Async;
using namespace SigC;



/****************************************************************************
 *
 * Defines & typedefs
 *
 ****************************************************************************/

#define OK 1
#define NO_DIALTONE 2
#define BUSY 3
#define RINGING 4
#define NO_CARRIER 5
#define CONNECT 6
#define ERROR 7
#define PHONELINE_DOWN 8
#define VCON 9

#define MODEM_INIT 1
#define MODEM_RESET 2
#define MODEM_HANGUP 3
#define MODEM_PICKUP 4
#define MODEM_PICKUP2 5
#define MODEM_VOICE 6
#define MODEM_DIAL 7

#define INIT 1


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


AnalogPhoneLogic::AnalogPhoneLogic(Async::Config& cfg, const std::string& name)
  : Logic(cfg, name), auth(false), auth_request(false), auth_timeout(10000),  cnt(0),
    hangup_counter(0), ident_nag_timeout(0), ident_nag_min_time(2000), ident_nag_timer(0),
    idle_sound_interval(0), idle_sound_timer(0), idle_timeout(60000), line_state(0),
    modem_init_ok(false), norepeat(0), open_on_dtmf('?'),
    phoneline_is_up(false), pickup_delay(0), rgr_enable(true),rings(0), up_timer(0),
    debug(false)
{
  timerclear(&rpt_close_timestamp);
  timerclear(&sql_up_timestamp);


  m_response[OK] = "OK";
  m_response[BUSY] = "BUSY";
  m_response[NO_DIALTONE] = "NO DIALTONE";
  m_response[RINGING] = "RINGING";
  m_response[VCON] = "VCON";
  m_response[ERROR] = "ERROR";
  m_response[CONNECT] = "CONNECT";
} /* AnalogPhoneLogic::AnalogPhoneLogic */


AnalogPhoneLogic::~AnalogPhoneLogic(void)
{
  modem->close();
  delete modem;
  delete idle_sound_timer;
  delete up_timer;
  delete auth_timer;
  delete ident_nag_timer;
} /* AnalogPhoneLogic::~AnalogPhoneLogic */


bool AnalogPhoneLogic::initialize(void)
{
 // regex_t re;
 // int stat;
  string str;
  std::string pin_pattern = "[ABCD0123456789]";

  if (!Logic::initialize())
  {
    return false;
  }

  if (cfg().getValue(name(), "IDLE_TIMEOUT", str))
  {
    idle_timeout = atoi(str.c_str()) * 1000;
  }

  if (cfg().getValue(name(), "DEBUG", str))
  {
      debug = true;
  }

  // dials this number automatically, if the autopatch is enabled from
  // the RF site
  if (cfg().getValue(name(), "AUTODIAL_PHONE_NR", str))
  {
     autodial_nr = str;
  }

  // maximum time in seconds to get a VCON from the modem to indicate
  // that the voice-connection has been established
  if (!cfg().getValue(name(), "VCON_TIMEOUT", str))
  {
     vcon_timeout = 50000;
  }
  else
  {
     vcon_timeout = atoi(str.c_str()) * 1000;
     if (vcon_timeout > 60000 || vcon_timeout < 8000) vcon_timeout = 50000;
  }

  // begin modem config
  if (!cfg().getValue(name(), "MODEM_PORT", str))
  {
     modem_port = str;
  }
  else
  {
      modem_port = "/dev/ttyS0";
  }

  if (!cfg().getValue(name(), "MODEM_SPEED", str))
  {
     modem_speed = 38400;
  }
  else
  {
      modem_speed = atoi(str.c_str());
  }

  if (!cfg().getValue(name(), "MODEM_RESPONSE_TIMEOUT", str))
  {
     modem_response_timeout = 1500;
  }
  else
  {
      modem_response_timeout = atoi(str.c_str()) * 100;
      if (modem_response_timeout < 200 || modem_response_timeout > 1000)
        modem_response_timeout = 1500;
  }

  if (!cfg().getValue(name(), "MODEM_INITSTRING", str))
  {
     modem_cmds.insert(pair<int, string>(MODEM_INIT, "AT&F&C1&D0V1#CLS=8#VLS=6"));
  }
  else
  {
     modem_cmds.insert(pair<int, string>(MODEM_INIT, str));
  }

  if (!cfg().getValue(name(), "MODEM_VOICESTRING", str))
  {
     modem_cmds.insert(pair<int, string>(MODEM_VOICE, "AT#VTD=3f,3f,3f"));
  }
  else
  {
     modem_cmds.insert(pair<int, string>(MODEM_VOICE, str));
  }

  if (!cfg().getValue(name(), "MODEM_DIALSTRING", str))
  {
     modem_cmds.insert(pair<int, string>(MODEM_DIAL, "ATX3DT"));
  }
  else
  {
     modem_cmds.insert(pair<int, string>(MODEM_DIAL, str));
  }

  if (!cfg().getValue(name(), "MODEM_PICKUP_CMD", str))
  {
     modem_cmds.insert(pair<int, string>(MODEM_PICKUP, "ATA"));
  }
  else
  {
     modem_cmds.insert(pair<int, string>(MODEM_PICKUP, str));
  }

  if (!cfg().getValue(name(), "MODEM_HANGUP_CMD", str))
  {
     modem_cmds.insert(pair<int, string>(MODEM_HANGUP, "ATH"));
  }
  else
  {
     modem_cmds.insert(pair<int, string>(MODEM_HANGUP, str));
  }

  if (!cfg().getValue(name(), "MODEM_RESET_CMD", str))
  {
     modem_cmds.insert(pair<int, string>(MODEM_RESET, "ATZ"));
  }
  else
  {
     modem_cmds.insert(pair<int, string>(MODEM_RESET, str));
  }

  if (!cfg().getValue(name(), "MODEM_MAX_RINGNUMBER", str))
  {
     max_rings = 3;
  }
  else
  {
     max_rings = atoi(str.c_str());
     if (max_rings > 10 || max_rings < 1) max_rings = 1;
  }
  // end modem config

  if (cfg().getValue(name(), "IDLE_SOUND_INTERVAL", str))
  {
    idle_sound_interval = atoi(str.c_str());
  }

  if (cfg().getValue(name(), "PICKUP_DELAY", str))
  {
    pickup_delay = atoi(str.c_str());
  }

  if (!cfg().getValue(name(), "AUTH_DELAY", str))
  {
     // you have 15 seconds for authentification
     auth_timeout = 15000;
  }
  else
  {
     auth_timeout = atoi(str.c_str()) * 1000;
  }

  if (cfg().getValue(name(), "IDENT_NAG_TIMEOUT", str))
  {
    ident_nag_timeout = 1000 * atoi(str.c_str());
  }

  if (cfg().getValue(name(), "IDENT_NAG_MIN_TIME", str))
  {
    ident_nag_min_time = atoi(str.c_str());
  }

  // sets the tone frequency for a busy tone, at the moment we can set only one
  // tone
  if (!cfg().getValue(name(), "BUSY_TONE_FREQUENCY", str))
  {
      busy_tone_freq = 425;
  }
  else
  {
      busy_tone_freq = atoi(str.c_str());
  }

  if (!cfg().getValue(name(), "BUSY_TONE_LENGTH", str))
  {
      busy_tone_len.insert(pair<int, int>(720, 880));
  }
  else
  {
      std::string ttone;
      int btone_max, btone_min;
      StrList tone_list;

      splitStr(tone_list, str, ",");

      for( StrList::const_iterator busy_it = tone_list.begin();
           busy_it != tone_list.end(); busy_it++ )
      {
           ttone = *busy_it;
           btone_max = atoi(ttone.c_str()) + int(atoi(ttone.c_str()) / 10);
           btone_min = atoi(ttone.c_str()) - int(atoi(ttone.c_str()) / 10);
           busy_tone_len.insert(pair<int, int>(btone_min, btone_max));
      }
  }

  // check users for phoneline access
  if (cfg().getValue(name(), "USERS", str))
  {
     StrList tstr;
     string tuser;

     splitStr(user_list, str, ",");   // DL1ABC:445343      DL2ABC:343434

     for( StrList::const_iterator user_it = user_list.begin();
           user_it != user_list.end(); user_it++ )
        {
            tuser = *user_it;
            splitStr(tstr, tuser, ":");

            if (tstr[1].length() < 4)
            {
              cout << "***bad PIN (" << tstr[1] << ") number for " << tstr[0]
                   << endl;
            }

            users_map.insert(pair<std::string, std::string>(tstr[1], tstr[0]));
        }
  }

  rptValveSetOpen(false);
  tx().setTxCtrlMode(Tx::TX_AUTO);

  idleStateChanged.connect(slot(*this, &AnalogPhoneLogic::setIdle));

  rx().toneLenDetected.connect(slot(*this, &AnalogPhoneLogic::detectedTone));
  if (!rx().addToneTimeDetector(busy_tone_freq, 70, 9))
  {
     cerr << "*** WARNING: Could not setup tone detector" << endl;
     return false;
  }

  // init modem
  modem = new Serial(modem_port);
  modem->charactersReceived.connect(
      	  slot(*this, &AnalogPhoneLogic::onModemDataReceived));

  if (!modem->open())
  {
     cerr << "Could not open modem port " << endl;
     return false;
  }

  modem->setParams(modem_speed, Serial::PARITY_NONE, 8, 1, Serial::FLOW_HW);
  modem->setPin(Serial::PIN_RTS,1);
  modem->setPin(Serial::PIN_DTR,1);

  resetModem();

  return true;
} /* AnalogPhoneLogic::initialize */


void AnalogPhoneLogic::processEvent(const string& event, const Module *module)
{
  rgr_enable = true;

  if ((event == "every_minute") && isIdle())
  {
    rgr_enable = false;
  }

  if ((event == "phoneline_idle") || (event == "send_rgr_sound"))
  {
    setReportEventsAsIdle(true);
    Logic::processEvent(event, module);
    setReportEventsAsIdle(false);
  }
  else
  {
    Logic::processEvent(event, module);
  }
} /* AnalogPhoneLogic::processEvent */

/**
  will be called up by the phoneline by the
  PHONELINK-command, e.g. "959#"
**/
void AnalogPhoneLogic::requestAuth(std::string reason)
{
   stringstream ss;
   auth_request = true;    // authentification request
   ss << "request_authentification " << reason;
   processEvent(ss.str());

   auth_timer = new Timer(auth_timeout);
   auth_timer->expired.connect(slot(*this, &AnalogPhoneLogic::authTimeout));
} /* requAuth */


void AnalogPhoneLogic::discPhone(std::string reason)
{
   stringstream ss;
   processEvent("phone_disconnecting");
   Logic::connectPhoneline(false);
   ss << "phone_disconnected " << reason;
   processEvent(ss.str());
   auth = false;
   auth_request = false;
}


bool AnalogPhoneLogic::activateModule(Module *module)
{
  setUp(true, "MODULE");
  return Logic::activateModule(module);
} /* AnalogPhoneLogic::activateModule */


void AnalogPhoneLogic::dtmfDigitDetected(char digit, int duration)
{
  stringstream ss;

  cout << "digit '" << digit << "' received" << endl;

  if (!phoneline_is_up) return;

  if (!auth_request)
  {
     if (debug) cout << "DTMF digit \"" << digit << "\" detected.";
     Logic::dtmfDigitDetected(digit, duration);
  }
  else   // Authentification pending
  {
     if (digit == '#')
     {
        if (debug) cout << "access_pin:" << access_pin << " | in_pin:"
                        << in_pin << endl;

        map<string,string>::iterator iter = users_map.find(in_pin);

        // check entered Pin
        if (iter != users_map.end())
        {
            phoneuser = iter->second;
            ss << "access_granted " << phoneuser;
            processEvent(ss.str());
            delete auth_timer;
            auth = true;  // authentificated

            wait_phone_connection_timer = new Timer(1500);
            wait_phone_connection_timer->expired.connect(slot(*this,
                         &AnalogPhoneLogic::waitPhoneConnection));
        }
        else   // Pin is wrong
        {
            processEvent("wrong_pin");
        }
        in_pin = "";             // reset the entered vals
        auth_timer = 0;          // reset the timer
        auth_request = false;   // stop authentification
     }
     else
     {
        in_pin += digit;         // add the entered digit to get the
     }                           // Pin number
  }
} /* AnalogPhoneLogic::dtmfDigitDetected */


/* will called up by an external request (via rf)
*  still work to do !!
*/
void AnalogPhoneLogic::pickupRemote(const std::string reason)
{

   if (autodial_nr.length() > 1) phonenumber = autodial_nr;

   // pickup phoneline
   (phoneuser.length() > 0) ? setUp(true, phoneuser) : setUp(true, reason);

} /* pickupRemote */


// by external request
void AnalogPhoneLogic::hangupRemote(const std::string reason)
{

   setUp(false, reason);  // hangup phoneline

} /* hangupRemote */

/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/

void AnalogPhoneLogic::allMsgsWritten(void)
{
   Logic::allMsgsWritten();
   tx().setTxCtrlMode(Tx::TX_AUTO);

} /* AnalogPhoneLogic::allMsgsWritten */


void AnalogPhoneLogic::audioStreamStateChange(bool is_active, bool is_idle)
{
  rgr_enable = true;

/* if (!phoneline_is_up && !is_idle)
  {
    setUp(true, "AUDIO");
  }

  Logic::audioStreamStateChange(is_active, is_idle);
*/
} /* Logic::audioStreamStateChange */


#if 0
bool AnalogPhoneLogic::getIdleState(void) const
{
  /*
  if (preserve_idle_state)
  {
    return isIdle();
  }
  */

  return Logic::getIdleState();

} /* AnalogPhoneLogic::isIdle */
#endif



/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/


void AnalogPhoneLogic::idleTimeout(Timer *t)
{
  printf("AnalogPhoneLogic::idleTimeout\n");
  setUp(false, "IDLE_timeout");
} /* idleTimeout */


void AnalogPhoneLogic::setIdle(bool idle)
{
  return;   // Don't need it at the moment

  if (debug) printf("AnalogPhoneLogic::setIdle: idle=%s\n", idle ? "true" : "false");

  if (!phoneline_is_up)
  {
    return;
  }

  if ((idle && (up_timer != 0)) || (!idle && (up_timer == 0)))
  {
    return;
  }

  delete up_timer;
  up_timer = 0;
  delete idle_sound_timer;
  idle_sound_timer = 0;
  if (idle)
  {
    up_timer = new Timer(idle_timeout);
    up_timer->expired.connect(slot(*this, &AnalogPhoneLogic::idleTimeout));

    if (idle_sound_interval > 0)
    {
   //  idle_sound_timer = new Timer(idle_sound_interval, Timer::TYPE_PERIODIC);
   //  idle_sound_timer->expired.connect(
    //     slot(*this, &AnalogPhoneLogic::playIdleSound));
    }
  }

  enableRgrSoundTimer(idle && rgr_enable);

} /* AnalogPhoneLogic::setIdle */


void AnalogPhoneLogic::setUp(bool up, string reason)
{
  stringstream ss;

  //cerr << "Phonline: " << up << "|||" << phoneline_is_up << "\n";

  if (up == phoneline_is_up)
  {
    return;
  }

  if (up)     // setting "up"
  {
    tx().setTxCtrlMode(Tx::TX_ON);
    rxValveSetOpen(true);
    phoneline_is_up = true;
    ss << "phoneline_up " << reason;
    processEvent(ss.str());
    //Logic::connectPhoneline(true);
  }
  else
  {
    // user interface
    phoneline_is_up = false;
    voice_ok = false;
    auth = false;
    phoneuser = "";
    in_pin = "";
    auth_request = false;
    ss << "phoneline_down " << reason;
    processEvent(ss.str());             // repeater logic
    AnalogPhoneLogic::processEvent(ss.str()); // phone logic

    Logic::connectPhoneline(false);

    tx().setTxCtrlMode(Tx::TX_OFF);  // switch off AF
    rxValveSetOpen(false);
    idle_sound_timer = 0;
    ident_nag_timer = 0;
    disconnectAllLogics();
    modem_dial_timer = 0;
  }
} /* AnalogPhoneLogic::setUp */


void AnalogPhoneLogic::waitPhoneConnection(Timer *t)
{
    stringstream ss;
    auth = true;  // authentificated
    Logic::connectPhoneline(true);
    ss << "incoming_phonecall " << phoneuser;
    processEvent(ss.str());
} /* AnalogPhoneLogic::waitPhoneConnection */


void AnalogPhoneLogic::detectedTone(long len)
{
    stringstream ss;

    if (debug) cout << "tone detected, length: " << len << endl;

    for( map<int,int>::iterator busy_it = busy_tone_len.begin();
           busy_it != busy_tone_len.end(); busy_it++ )
    {
       if ( len > busy_it->first && len < busy_it->second )
       {
	  if (debug) cout << "length between the limits -> hangup follows" << endl;
          cnt++;
       }
    }

    if (cnt >= 3)
    {
       cnt = 0;
       if (debug) cout << "***hangup phoneline" << endl;
       ss << "busy";
       hangupLocal(ss.str());
    }
} /* AnalogPhoneLogic::detectedTone */


void AnalogPhoneLogic::initModem(int extime, int ser)
{
    modemCmd::iterator it = modem_cmds.find(ser);
    std::string mstr = it->second;

    writeModem(mstr);

    modem_to_timer = new Timer(extime);
    modem_to_timer->expired.connect(slot(*this,
                      &AnalogPhoneLogic::modemInitTimeout));

} /* AnalaogPhoneLogic::checkModem */


void AnalogPhoneLogic::modemInitTimeout(Timer *t)
{

    if (line_state == OK)
    {
       modem_init_ok = true;
    }
    else
    {
       cout << "*** modem could not beeing initialized\n";
       modem_init_ok = false;
       initModem(modem_response_timeout * 2, INIT);
    }
} /* AnalogPhoneLigic::modemInitTimeout */


void AnalogPhoneLogic::dialPhoneNumber(std::string dialnr)
{
    modemCmd::iterator it = modem_cmds.find(MODEM_DIAL);
    std::string mstr = it->second;

    mstr += dialnr;

    writeModem(mstr);

    modem_dial_timer = new Timer(vcon_timeout);
    modem_dial_timer->expired.connect(slot(*this,
                         &AnalogPhoneLogic::modemDialTimeout));

} /* AnalogPhoneLogic::dialPhoneNumber */


void AnalogPhoneLogic::modemDialTimeout(Timer *t)
{
    if (line_state == VCON)
    {
       line_state = 0;
       cout << "party connected" << endl;
    }
    else
    {
       setUp(false, "modem_dial_timeout" );
       resetModem();
       if (debug) cout << "*** party not responding\n";
    }
} /* AnalogPhoneLigic::modemDialTimeout */


void AnalogPhoneLogic::resetModem(void)
{
    modemCmd::iterator it = modem_cmds.find(MODEM_RESET);
    std::string mstr = it->second;

    writeModem(mstr);

    modem_reset_timer = new Timer(modem_response_timeout);
    modem_reset_timer->expired.connect(slot(*this,
                         &AnalogPhoneLogic::modemResetTimeout));
} /* AnalogPhoneLogic::resetModem */


void AnalogPhoneLogic::modemResetTimeout(Timer *t)
{

   if (line_state == OK)
   {
       if (debug) cout << "***Modem reset OK, reinit modem" << endl;
       initModem(modem_response_timeout, INIT);
   }
   else
   {
      if (debug) cout << "***ERROR: Modem could not reset, trying again"
                      << endl;
      resetModem();
   }
} /* AnalogPhoneLogic::modemResetTimeout */


void AnalogPhoneLogic::pickupLocal(std::string reason)
{
    modemCmd::iterator it = modem_cmds.find(MODEM_PICKUP);
    std::string mstr = it->second;

    writeModem(mstr);

    modem_voicecmd_timer = new Timer(modem_response_timeout);
    modem_voicecmd_timer->expired.connect(slot(*this,
                         &AnalogPhoneLogic::pickupTimeout));

} /* AnalogPhoneLogic::pickupLocal */


void AnalogPhoneLogic::pickupTimeout(Timer *t)
{

    if (voice_ok) return;
    if (line_state == OK || line_state == VCON)
    {
        voice_ok = true;
        setUp(true, "local_pickup_ring");
    }
    else
    {
        if (debug) cout << "****modem error" << endl;
        voice_ok = false;
        hangupLocal("modem error");
    }
} /* AnalogPhoneLogic::pickupTimeout */


void AnalogPhoneLogic::hangupLocal(std::string reason)
{
    modemCmd::iterator it = modem_cmds.find(MODEM_HANGUP);
    std::string mstr = it->second;

    if (!isWritingMessage() || ++hangup_counter > 4)
    {
       if (debug) cout << "***hanging up now" << endl;
       setUp(false, reason);
       writeModem(mstr);
    }
    else
    {
       line_state = 0;
       voice_ok = false;
    }

    modem_hangup_timer = new Timer(modem_response_timeout);
    modem_hangup_timer->expired.connect(slot(*this,
                         &AnalogPhoneLogic::modemHangupTimeout));

} /* AnalogPhoneLogic::hangupLocal */


void AnalogPhoneLogic::modemHangupTimeout(Timer *t)
{

   if (line_state == OK)
   {
       if (debug) cout << "***Modem hangup OK, reinit all" << endl;
       hangup_counter = 0;
       resetModem();
   }
   else
   {
      hangupLocal("Modem_hangup_error_try_again");
   }
} /* AnalogPhoneLogic::modemHangupTimeout */


void AnalogPhoneLogic::writeModem(std::string modemstr)
{
    modemstr += "\r\n";
    if (debug) cout << "***write to modem: " << modemstr;
    line_state = 0;
    modem->write(modemstr.c_str(), modemstr.length());
} /* AnalogPhoneLogic::writeModem */


void AnalogPhoneLogic::authTimeout(Timer *t)
{
    if (debug) cout << "***AUTH TIMEOUT***" << endl;
    auth_request = false;
    in_pin = "";
    processEvent("auth_timeout");
} /* AnalogPhoneLogic::authTimeout */


void AnalogPhoneLogic::squelchOpen(bool is_open)
{
   Logic::squelchOpen(is_open);
} /* AnalogPhoneLogic::squelchOpen */


void AnalogPhoneLogic::onModemDataReceived(char *buf, int count)
{
   string tstr = "";
   tstr += buf;

   if (tstr.find("OK") != string::npos )
   {
       line_state = OK;
   }
   else if (tstr.find("BUSY") != string::npos)
   {
       line_state = BUSY;
       setUp(false, "party busy");
   }
   else if ( tstr.find("NO CARRIER") != string::npos)
   {
      line_state = NO_CARRIER;
   }
   else if ( tstr.find("ERROR") != string::npos)
   {
      line_state = ERROR;
   }
   else if ( tstr.find("R") != string::npos)
   {
      line_state = RINGING;
      if (++rings >= max_rings)
      {
         rings = 0;
         voice_ok = false;
         pickupLocal("incoming party");
      }
   }
   else if ( tstr.find("NO DIALTONE") != string::npos)
   {
      line_state = NO_DIALTONE;
   }
   else if ( tstr.find("CONNECT") != string::npos)
   {
      line_state = CONNECT;
   }
   else if ( tstr.find("VCON") != string::npos)
   {
      line_state = VCON;
   }

   if (debug && line_state > 0)
   {
      cout << "***Modem response: " << m_response[line_state] << endl;
   }

} /* onModemDataReceived */


int AnalogPhoneLogic::splitStr(StrList& L, const string& seq, const string& delims)
{
  L.clear();

  string str;
  string::size_type pos = 0;
  string::size_type len = seq.size();
  while (pos < len)
  {
      // Init/clear the STR token buffer
    str = "";

      // remove any delimiters including optional (white)spaces
    while ((delims.find(seq[pos]) != string::npos) && (pos < len))
    {
      pos++;
    }

      // leave if @eos
    if (pos == len)
    {
      return L.size();
    }

      // Save token data
    while ((delims.find(seq[pos]) == string::npos) && (pos < len))
    {
      str += seq[pos++];
    }

      // put valid STR buffer into the supplied list
    if (!str.empty())
    {
      L.push_back(str);
    }
  }

  return L.size();

} /* AnalogPhoneLogic::splitStr */


/*
 * This file has not been truncated
 */

