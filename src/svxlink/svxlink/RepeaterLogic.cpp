/**
@file	 RepeaterLogic.cpp
@brief   Contains a class that implements a repeater controller
@author  Tobias Blomberg / SM0SVX
@date	 2004-04-24

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

#include <sys/time.h>

#include <cstdio>
#include <string>
#include <iostream>
#include <cassert>
#include <cstdlib>
#include <sstream>
#include <algorithm>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncTimer.h>
#include <AsyncConfig.h>

#include <Rx.h>
#include <Tx.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "RepeaterLogic.h"



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



/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/


RepeaterLogic::RepeaterLogic(Async::Config& cfg, const std::string& name)
  : Logic(cfg, name), repeater_is_up(false), up_timer(0), idle_timeout(30000),
    idle_sound_timer(0), idle_sound_interval(0),
    required_sql_open_duration(-1),
    open_on_dtmf('?'), activate_on_sql_close(false), no_repeat(false),
    open_on_sql_timer(0), open_sql_flank(SQL_FLANK_CLOSE),
    short_sql_open_cnt(0), sql_flap_sup_min_time(1000),
    sql_flap_sup_max_cnt(0), rgr_enable(true), open_reason("?")
{
} /* RepeaterLogic::RepeaterLogic */


RepeaterLogic::~RepeaterLogic(void)
{
  delete open_on_sql_timer;
  delete idle_sound_timer;
  delete up_timer;
} /* RepeaterLogic::~RepeaterLogic */


bool RepeaterLogic::initialize(void)
{
  if (!Logic::initialize())
  {
    return false;
  }
  
  float open_on_ctcss_fq = 0;
  int open_on_ctcss_duration = 0;
  int required_1750_duration = 0;
  
  string str;
  if (cfg().getValue(name(), "IDLE_TIMEOUT", str))
  {
    idle_timeout = atoi(str.c_str()) * 1000;
  }
  
  if (cfg().getValue(name(), "OPEN_ON_1750", str))
  {
    required_1750_duration = atoi(str.c_str());
  }
  
  if (cfg().getValue(name(), "OPEN_ON_CTCSS", str))
  {
    string::iterator it;
    it = find(str.begin(), str.end(), ':');
    if (it == str.end())
    {
      cerr << "*** ERROR: Illegal format for config variable " << name()
      	   << "/OPEN_ON_CTCSS. Should be <fq>:<required duration>.\n";
    }
    string fq_str(str.begin(), it);
    string dur_str(it+1, str.end());
    open_on_ctcss_fq = atof(fq_str.c_str());
    open_on_ctcss_duration = atoi(dur_str.c_str());
  }

  if (cfg().getValue(name(), "OPEN_ON_SQL", str))
  {
    required_sql_open_duration = atoi(str.c_str());
  }
  
  if (cfg().getValue(name(), "OPEN_ON_DTMF", str))
  {
    open_on_dtmf = str.c_str()[0];
  }
  
  if (cfg().getValue(name(), "OPEN_SQL_FLANK", str))
  {
    if (str == "OPEN")
    {
      open_sql_flank = SQL_FLANK_OPEN;
    }
    else if (str == "CLOSE")
    {
      open_sql_flank = SQL_FLANK_CLOSE;
    }
    else
    {
      cerr << "*** ERROR: Valid values for configuration variable "
      	   << "OPEN_SQL_FLANK are OPEN and CLOSE.\n";
    }
  }
  
  if (cfg().getValue(name(), "IDLE_SOUND_INTERVAL", str))
  {
    idle_sound_interval = atoi(str.c_str());
  }
  
  if (cfg().getValue(name(), "NO_REPEAT", str))
  {
    no_repeat = atoi(str.c_str()) != 0;
  }
  
  if (cfg().getValue(name(), "SQL_FLAP_SUP_MIN_TIME", str))
  {
    sql_flap_sup_min_time = atoi(str.c_str());
  }
  
  if (cfg().getValue(name(), "SQL_FLAP_SUP_MAX_COUNT", str))
  {
    sql_flap_sup_max_cnt = atoi(str.c_str());
  }
  
  rx().toneDetected.connect(slot(*this, &RepeaterLogic::detectedTone));
  
  if (required_1750_duration > 0)
  {
    if (!rx().addToneDetector(1750, 50, 10, required_1750_duration))
    {
      cerr << "*** WARNING: Could not setup 1750 detection\n";
    }
  }
  
  if ((open_on_ctcss_fq > 0) && (open_on_ctcss_duration > 0))
  {
    if (!rx().addToneDetector(open_on_ctcss_fq, 4, 10, open_on_ctcss_duration))
    {
      cerr << "*** WARNING: Could not setup CTCSS tone detection\n";
    }
  }
  
  rptValveSetOpen(!no_repeat);
  
  tx().setTxCtrlMode(Tx::TX_AUTO);
  
  idleStateChanged.connect(slot(*this, &RepeaterLogic::setIdle));
  
  return true;
  
} /* RepeaterLogic::initialize */


void RepeaterLogic::processEvent(const string& event, const Module *module)
{
  rgr_enable = true;
  
  if ((event == "every_minute") && isIdle())
  {
    rgr_enable = false;
  }
  
  if ((event == "repeater_idle") || (event == "send_rgr_sound"))
  {
    setReportEventsAsIdle(true);
    Logic::processEvent(event, module);
    setReportEventsAsIdle(false);
  }
  else
  {  
    Logic::processEvent(event, module);
  }  
} /* RepeaterLogic::processEvent */


bool RepeaterLogic::activateModule(Module *module)
{
  open_reason = "MODULE";
  setUp(true);
  return Logic::activateModule(module);
} /* RepeaterLogic::activateModule */


void RepeaterLogic::dtmfDigitDetected(char digit, int duration)
{
  if (repeater_is_up)
  {
    Logic::dtmfDigitDetected(digit, duration);
  }
  else
  {
    if (digit == open_on_dtmf)
    {
      cout << "DTMF digit \"" << digit << "\" detected. "
      	      "Activating repeater...\n";
      open_reason = "DTMF";
      activateOnOpenOrClose(SQL_FLANK_CLOSE);
    }
    else
    {
      cout << "Ignoring DTMF digit \"" << digit << "\"\n";
    }
  }
} /* RepeaterLogic::dtmfDigitDetected */



/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/

void RepeaterLogic::allMsgsWritten(void)
{
  Logic::allMsgsWritten();
  if (!repeater_is_up)
  {
    tx().setTxCtrlMode(Tx::TX_AUTO);
  }
} /* RepeaterLogic::allMsgsWritten */


void RepeaterLogic::audioStreamStateChange(bool is_active, bool is_idle)
{
  rgr_enable = true;

  if (!repeater_is_up && !is_idle)
  {
    open_reason = "AUDIO";
    setUp(true);
  }

  Logic::audioStreamStateChange(is_active, is_idle);
  
} /* Logic::audioStreamStateChange */


#if 0
bool RepeaterLogic::getIdleState(void) const
{
  /*
  if (preserve_idle_state)
  {
    return isIdle();
  }
  */

  return Logic::getIdleState();

} /* RepeaterLogic::isIdle */
#endif



/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/

void RepeaterLogic::idleTimeout(Timer *t)
{
  //printf("RepeaterLogic::idleTimeout\n");
  setUp(false);
} /* RepeaterLogic::idleTimeout */


void RepeaterLogic::setIdle(bool idle)
{
  //printf("RepeaterLogic::setIdle: idle=%s\n", idle ? "true" : "false");

  if (!repeater_is_up)
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
    up_timer->expired.connect(slot(*this, &RepeaterLogic::idleTimeout));
    
    if (idle_sound_interval > 0)
    {
      idle_sound_timer = new Timer(idle_sound_interval, Timer::TYPE_PERIODIC);
      idle_sound_timer->expired.connect(
      	  slot(*this, &RepeaterLogic::playIdleSound));
    }
  }

  enableRgrSoundTimer(idle && rgr_enable);
  
} /* RepeaterLogic::setIdle */


void RepeaterLogic::setUp(bool up)
{
  //printf("RepeaterLogic::setUp: up=%s  reason=%s\n",
  //    	 up ? "true" : "false", open_reason.c_str());
  if (up == repeater_is_up)
  {
    return;
  }
  
  if (up)
  {
    short_sql_open_cnt = 0;
    repeater_is_up = true;

    stringstream ss;
    //ss << "repeater_up " << (ident ? "1" : "0");
    ss << "repeater_up " << open_reason;
    processEvent(ss.str());
    
    rxValveSetOpen(true);
    tx().setTxCtrlMode(Tx::TX_ON);
    
    setIdle(false);
    checkIdle();
    setIdle(isIdle());
  }
  else
  {
    open_reason = "?";
    rxValveSetOpen(false);
    repeater_is_up = false;
    delete up_timer;
    up_timer = 0;
    delete idle_sound_timer;
    idle_sound_timer = 0;
    disconnectAllLogics();
    processEvent("repeater_down");
    if (!isWritingMessage())
    {
      tx().setTxCtrlMode(Tx::TX_AUTO);
    }
  }
  
} /* RepeaterLogic::setUp */


void RepeaterLogic::squelchOpen(bool is_open)
{
  //cout << name() << ": The squelch is " << (is_open ? "OPEN" : "CLOSED")
  //     << endl;
  
  rgr_enable = true;
  
  if (repeater_is_up)
  {
    if (is_open)
    {
      gettimeofday(&sql_up_timestamp, NULL);
      setIdle(false);
    }
    else
    {
      //tx().flushSamples();
      
      if (sql_flap_sup_max_cnt > 0)
      {
	struct timeval now, diff_tv;
	gettimeofday(&now, NULL);
	timersub(&now, &sql_up_timestamp, &diff_tv);
	int diff_ms = diff_tv.tv_sec * 1000 + diff_tv.tv_usec / 1000;
	if (diff_ms < sql_flap_sup_min_time)
	{
      	  if (++short_sql_open_cnt >= sql_flap_sup_max_cnt)
	  {
	    short_sql_open_cnt = 0;
	    setUp(false);
	  }
	}
	else
	{
      	  short_sql_open_cnt = 0;
	}
      }
    }
  
    Logic::squelchOpen(is_open);
  }
  else
  {
    if (is_open)
    {
      if (required_sql_open_duration >= 0)
      {
      	open_on_sql_timer = new Timer(required_sql_open_duration);
	open_on_sql_timer->expired.connect(
	    slot(*this, &RepeaterLogic::openOnSqlTimerExpired));
      }
    }
    else
    {
      if (open_on_sql_timer != 0)
      {
      	delete open_on_sql_timer;
      	open_on_sql_timer = 0;
      }
      
      if (activate_on_sql_close)
      {
      	activate_on_sql_close = false;
      	setUp(true);
      }
    }
  }
} /* RepeaterLogic::squelchOpen */


void RepeaterLogic::detectedTone(float fq)
{
  if (!repeater_is_up && !activate_on_sql_close)
  {
    cout << fq << " Hz tone call detected" << endl;
    
    if (fq < 300.0)
    {
      open_reason = "CTCSS";
      activateOnOpenOrClose(open_sql_flank);
    }
    else
    {
      open_reason = "TONE";
      activateOnOpenOrClose(SQL_FLANK_CLOSE);
    }
  }
} /* RepeaterLogic::detectedTone */


void RepeaterLogic::playIdleSound(Timer *t)
{
  processEvent("repeater_idle");
} /* RepeaterLogic::playIdleSound */


void RepeaterLogic::openOnSqlTimerExpired(Timer *t)
{
  delete open_on_sql_timer;
  open_on_sql_timer = 0;
  open_reason = "SQL";
  activateOnOpenOrClose(open_sql_flank);
} /* RepeaterLogic::openOnSqlTimerExpired */


void RepeaterLogic::activateOnOpenOrClose(SqlFlank flank)
{
  if (flank == SQL_FLANK_OPEN)
  {
    if ((open_reason == "SQL") || (open_reason == "CTCSS"))
    {
      open_reason += "_OPEN";
    }
    setUp(true);
    if (rx().squelchIsOpen())
    {
      RepeaterLogic::squelchOpen(true);
    }
  }
  else
  {
    if ((open_reason == "SQL") || (open_reason == "CTCSS"))
    {
      open_reason += "_CLOSE";
    }
    if (!rx().squelchIsOpen())
    {
      setUp(true);
    }
    else
    {
      activate_on_sql_close = true;
    }
  }
}



/*
 * This file has not been truncated
 */

