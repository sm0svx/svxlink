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

#include <cstdio>
#include <string>
#include <iostream>


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

#include "Tx.h"
#include "Rx.h"
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
    blip_timer(0), blip_delay(0), required_1750_duration(0)
{

} /* RepeaterLogic::RepeaterLogic */


RepeaterLogic::~RepeaterLogic(void)
{

} /* RepeaterLogic::~RepeaterLogic */


bool RepeaterLogic::initialize(void)
{
  if (!Logic::initialize())
  {
    return false;
  }
  
  string blip_delay_str;
  if (cfg().getValue(name(), "BLIP_DELAY", blip_delay_str))
  {
    blip_delay = atoi(blip_delay_str.c_str());
  }
  
  string idle_timeout_str;
  if (cfg().getValue(name(), "IDLE_TIMEOUT", idle_timeout_str))
  {
    idle_timeout = atoi(idle_timeout_str.c_str());
  }
  
  string req_1750_duration_str;
  if (cfg().getValue(name(), "REQUIRED_1750_DURATION", req_1750_duration_str))
  {
    required_1750_duration = atoi(req_1750_duration_str.c_str());
  }
  
  tx().txTimeout.connect(slot(this, &RepeaterLogic::txTimeout));
  
  rx().mute(false);
  rx().audioReceived.connect(slot(this, &RepeaterLogic::audioReceived));
  if (required_1750_duration > 0)
  {
    if (!rx().detect1750(required_1750_duration))
    {
      cerr << "*** WARNING: Could not setup 1750 detection\n";
    }
    rx().detected1750.connect(slot(this, &RepeaterLogic::detected1750));
  }
    
  playMsg("online");
  identify();
    
  return true;
  
} /* RepeaterLogic::initialize */


void RepeaterLogic::moduleTransmitRequest(bool do_transmit)
{
  if (do_transmit)
  {
    setUp(true);
  }
  Logic::moduleTransmitRequest(do_transmit);
} /* RepeaterLogic::moduleTransmitRequest */



/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/

void RepeaterLogic::transmit(bool do_transmit)
{
  printf("RepeaterLogic::transmit: do_transmit=%s\n",
      do_transmit ? "true" : "false");
  if (repeater_is_up)
  {
    if (do_transmit)
    {
      Logic::transmit(true);
    }
    setIdle(!do_transmit);
  }
  else
  {
    Logic::transmit(do_transmit);
  }
} /* RepeaterLogic::transmit */





/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/

void RepeaterLogic::identify(void)
{
  printf("RepeaterLogic::identify\n");
  if (!callsign().empty())
  {
    spellWord(callsign());
    playMsg("repeater");
  }
} /* RepeaterLogic::identify */


int RepeaterLogic::audioReceived(short *samples, int count)
{
  if (repeater_is_up)
  {
    return transmitAudio(samples, count);
  }
  
  return count;
  
} /* RepeaterLogic::audioReceived */


void RepeaterLogic::idleTimeout(Timer *t)
{
  printf("RepeaterLogic::idleTimeout\n");
  setUp(false);
  identify();
} /* RepeaterLogic::idleTimeout */


void RepeaterLogic::setIdle(bool idle)
{
  printf("RepeaterLogic::setIdle: idle=%s\n", idle ? "true" : "false");
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
  if (idle)
  {
    up_timer = new Timer(idle_timeout);
    up_timer->expired.connect(slot(this, &RepeaterLogic::idleTimeout));
  }
  
} /* RepeaterLogic::setIdle */


void RepeaterLogic::setUp(bool up)
{
  printf("RepeaterLogic::setUp\n");
  if (up == repeater_is_up)
  {
    return;
  }
  repeater_is_up = up;
  
  Logic::transmit(up);
  if (up)
  {
    identify();
  }
  
  //logicTransmitRequest(up);
} /* RepeaterLogic::setUp */


void RepeaterLogic::sendBlip(Timer *t)
{
  printf("RepeaterLogic::sendBlip\n");
  playMsg("blip");
  delete blip_timer;
  blip_timer = 0;
} /* RepeaterLogic::sendBlip */


void RepeaterLogic::squelchOpen(bool is_open)
{
  printf("The squelch is %s\n", is_open ? "OPEN" : "CLOSED");
  
  //squelch_is_open = is_open;
  
  if (repeater_is_up)
  {
    delete blip_timer;
    if (is_open)
    {
      blip_timer = 0;
    }
    else
    {
      if (blip_delay > 0)
      {
	blip_timer = new Timer(blip_delay);
	blip_timer->expired.connect(slot(this, &RepeaterLogic::sendBlip));
      }
      else
      {
	sendBlip();
      }
    }
    //setIdle(!is_open);
    logicTransmitRequest(is_open);
  }
  else
  {
    if (is_open && (required_1750_duration == 0))
    {
      setUp(true);
    }
  }
  
} /* RepeaterLogic::squelchOpen */


void RepeaterLogic::txTimeout(void)
{
  Logic::transmit(false);
  clearPendingSamples();
} /* RepeaterLogic::txTimeout */


void RepeaterLogic::detected1750(void)
{
  printf("1750 tone call detected\n");
  setUp(true);
} /* RepeaterLogic::detected1750 */




/*
 * This file has not been truncated
 */

