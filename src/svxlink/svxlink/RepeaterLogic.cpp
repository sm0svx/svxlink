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

#include "Tx.h"
#include "Rx.h"
#include "RepeaterLogic.h"



/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/

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
  : Logic(cfg, name), repeater_is_up(false), up_timer(0), idle_timeout(10000)
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
  
  tx().txTimeout.connect(slot(this, &RepeaterLogic::txTimeout));
  
  rx().mute(false);
  rx().audioReceived.connect(slot(this, &RepeaterLogic::audioReceived));
  
  playMsg("online");
  identify();
    
  return true;
  
} /* RepeaterLogic::initialize */





/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/


/*
 *------------------------------------------------------------------------
 * Method:    
 * Purpose:   
 * Input:     
 * Output:    
 * Author:    
 * Created:   
 * Remarks:   
 * Bugs:      
 *------------------------------------------------------------------------
 */






/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/

void RepeaterLogic::identify(void)
{
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
  setUp(false);
  identify();
} /* RepeaterLogic::idleTimeout */


void RepeaterLogic::setIdle(bool idle)
{
  if (!repeater_is_up)
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
  if (up == repeater_is_up)
  {
    return;
  }
  
  transmit(up);
  repeater_is_up = up;
} /* RepeaterLogic::setUp */


void RepeaterLogic::squelchOpen(bool is_open)
{
  printf("The squelch is %s\n", is_open ? "OPEN" : "CLOSED");
  
  //squelch_is_open = is_open;
  if (is_open)
  {
    setUp(true);
  }
  setIdle(!is_open);
  
} /* RepeaterLogic::squelchOpen */


void RepeaterLogic::txTimeout(void)
{
  Logic::transmit(false);
  clearPendingSamples();
} /* RepeaterLogic::txTimeout */









/*
 * This file has not been truncated
 */

