/**
@file	 RepeaterLogic.cpp
@brief   A_brief_description_for_this_file
@author  Tobias Blomberg / SM0SVX
@date	 2004-03-

A_detailed_description_for_this_file

\verbatim
<A brief description of the program or library this file belongs to>
Copyright (C) 2003 Tobias Blomberg / SM0SVX

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


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncTimer.h>
#include <AsyncConfig.h>

#include <Rx.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "Tx.h"
#include "Module.h"
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
    idle_sound_timer(0), idle_sound_interval(0), repeating_enabled(false),
    preserve_idle_state(false), required_sql_open_duration(-1),
    open_on_dtmf('?'), activate_on_sql_close(false), no_repeat(false)
{
  timerclear(&sql_open_timestamp);
} /* RepeaterLogic::RepeaterLogic */


RepeaterLogic::~RepeaterLogic(void)
{
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
  
  if (cfg().getValue(name(), "IDLE_SOUND_INTERVAL", str))
  {
    idle_sound_interval = atoi(str.c_str());
  }
  
  if (cfg().getValue(name(), "NO_REPEAT", str))
  {
    no_repeat = atoi(str.c_str()) != 0;
  }
  
  //tx().txTimeout.connect(slot(this, &RepeaterLogic::txTimeout));
  
  //rx().mute(false);
  rx().audioReceived.connect(slot(*this, &RepeaterLogic::audioReceived));
  
  if (required_1750_duration > 0)
  {
    if (!rx().addToneDetector(1750, 25, -15, required_1750_duration))
    {
      cerr << "*** WARNING: Could not setup 1750 detection\n";
    }
    rx().toneDetected.connect(slot(*this, &RepeaterLogic::detectedTone));
  }
  
  if ((open_on_ctcss_fq > 0) && (open_on_ctcss_duration > 0))
  {
    if (!rx().addToneDetector(open_on_ctcss_fq, 8, -5, open_on_ctcss_duration))
    {
      cerr << "*** WARNING: Could not setup CTCSS tone detection\n";
    }
    rx().toneDetected.connect(slot(*this, &RepeaterLogic::detectedTone));
  }
  
  return true;
  
} /* RepeaterLogic::initialize */


void RepeaterLogic::processEvent(const string& event, const Module *module)
{
  if ((event == "repeater_idle") || (event == "send_rgr_sound"))
  {
    preserve_idle_state = true;
    Logic::processEvent(event, module);
    preserve_idle_state = false;
  }
  else
  {  
    Logic::processEvent(event, module);
  }  
} /* RepeaterLogic::processEvent */


void RepeaterLogic::playFile(const string& path)
{
  //printf("RepeaterLogic::playiFile: %s\n", path.c_str());
  
  if (!preserve_idle_state)
  {
    setIdle(false);
  }
  Logic::playFile(path);
} /* RepeaterLogic::playFile */


void RepeaterLogic::playSilence(int length)
{
  //printf("RepeaterLogic::playSilence: %d ms\n", length);
  
  if (!preserve_idle_state)
  {
    setIdle(false);
  }
  Logic::playSilence(length);
} /* RepeaterLogic::playSilence */


void RepeaterLogic::playTone(int fq, int amp, int len)
{
  //printf("RepeaterLogic::playTone: fq=%d amp=%d len=%d ms\n", fq, amp, len);
  
  if (!preserve_idle_state)
  {
    setIdle(false);
  }
  Logic::playTone(fq, amp, len);
} /* RepeaterLogic::playSilence */


void RepeaterLogic::moduleTransmitRequest(bool do_transmit)
{
  if (do_transmit)
  {
    setUp(true);
    setIdle(false);
  }
  Logic::moduleTransmitRequest(do_transmit);
} /* RepeaterLogic::moduleTransmitRequest */


bool RepeaterLogic::activateModule(Module *module)
{
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
      activate_on_sql_close = true;
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

void RepeaterLogic::allTxSamplesFlushed(void)
{
  //printf("RepeaterLogic::allTxSamplesFlushed\n");
  Module *module = activeModule();
  if (!rx().squelchIsOpen() && ((module == 0) || (!module->isTransmitting())))
  {
    setIdle(true);
  }
  Logic::allTxSamplesFlushed();
  repeating_enabled = true;
} /* RepeaterLogic::allTxSamplesFlushed */


void RepeaterLogic::remoteLogicTransmitRequest(bool do_tx)
{
  if (do_tx)
  {
    setUp(true);
    setIdle(false);
  }
  Logic::remoteLogicTransmitRequest(do_tx);
} /* RepeaterLogic::remoteLogicTransmitRequest */



/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/

int RepeaterLogic::audioReceived(float *samples, int count)
{
  if (repeater_is_up && repeating_enabled && !no_repeat)
  {
    return transmitAudio(samples, count);
  }
  
  return count;
  
} /* RepeaterLogic::audioReceived */


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

  enableRgrSoundTimer(idle);
  
} /* RepeaterLogic::setIdle */


void RepeaterLogic::setUp(bool up)
{
  //printf("RepeaterLogic::setUp: up=%s\n", up ? "true" : "false");
  if (up == repeater_is_up)
  {
    return;
  }
  
  if (up)
  {
    processEvent("repeater_up");
    repeater_is_up = true;
      // Enable repeating only if no statup message was played. If there is
      // a statuup message, repeating will be enabled when the message has
      // been played.
    repeating_enabled = !isWritingMessage();
    if (repeating_enabled)
    {
      setIdle(true);
    }
  }
  else
  {
    repeater_is_up = false;
    delete up_timer;
    up_timer = 0;
    delete idle_sound_timer;
    idle_sound_timer = 0;
    disconnectAllLogics();
    processEvent("repeater_down");
  }
  
  logicTransmitRequest(up);
  
} /* RepeaterLogic::setUp */


void RepeaterLogic::squelchOpen(bool is_open)
{
  //cout << name() << ": The squelch is " << (is_open ? "OPEN" : "CLOSED")
  //     << endl;
  
  if (repeater_is_up)
  {
    if (is_open)
    {
      setIdle(false);
    }
    else
    {
      tx().flushSamples();
    }
  }
  else
  {
    if (is_open)
    {
      gettimeofday(&sql_open_timestamp, NULL);
    }
    else
    {
      if (required_sql_open_duration >= 0)
      {
	assert(timerisset(&sql_open_timestamp));
	struct timeval tv, tv_diff;
	gettimeofday(&tv, NULL);
	timersub(&tv, &sql_open_timestamp, &tv_diff);
	long diff = tv_diff.tv_sec * 1000 + tv_diff.tv_usec / 1000;
	//printf("The squelch was open for %ld milliseconds\n", diff);
	if (diff >= required_sql_open_duration)
	{
	  setUp(true);
	}
	timerclear(&sql_open_timestamp);
      }
      
      if (activate_on_sql_close)
      {
      	activate_on_sql_close = false;
      	setUp(true);
      }
    }
  }
  
  Logic::squelchOpen(is_open);
  
} /* RepeaterLogic::squelchOpen */


#if 0
void RepeaterLogic::txTimeout(void)
{
  Logic::transmit(false);
  clearPendingSamples();
} /* RepeaterLogic::txTimeout */
#endif


void RepeaterLogic::detectedTone(float fq)
{
  if (!repeater_is_up)
  {
    cout << fq << " Hz tone call detected" << endl;
    setUp(true);
  }
} /* RepeaterLogic::detectedTone */


void RepeaterLogic::playIdleSound(Timer *t)
{
  processEvent("repeater_idle");
} /* RepeaterLogic::playIdleSound */



/*
 * This file has not been truncated
 */

