/**
@file	 SimplexLogic.cpp
@brief   A_brief_description_for_this_file
@author  Tobias Blomberg / SM0SVX
@date	 2004-03-23

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

#include <stdio.h>

#include <iostream>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncConfig.h>
#include <AsyncTimer.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "Rx.h"
#include "Tx.h"
#include "SimplexLogic.h"



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


SimplexLogic::SimplexLogic(Async::Config& cfg, const string& name)
  : Logic(cfg, name), pending_transmit(false), squelch_is_open(false),
    tx_timeout_occured(false), ident_timer(0), ident_interval(1800),
    pending_ident(false)
{

} /* SimplexLogic::SimplexLogic */


SimplexLogic::~SimplexLogic(void)
{
  delete ident_timer;
} /* SimplexLogic::~SimplexLogic */


bool SimplexLogic::initialize(void)
{
  if (!Logic::initialize())
  {
    return false;
  }
  
  string str;
  if (cfg().getValue(name(), "IDENT_INTERVAL", str))
  {
    ident_interval = atoi(str.c_str()) * 1000;
  }
  
  tx().txTimeout.connect(slot(this, &SimplexLogic::txTimeout));
  
  if (ident_interval > 0)
  {
    ident_timer = new Timer(ident_interval, Timer::TYPE_PERIODIC);
    ident_timer->expired.connect(slot(this, &SimplexLogic::identify));
  }
      
  identify();
  
  return true;
  
} /* SimplexLogic::initialize */


void SimplexLogic::transmit(bool do_transmit)
{
  //printf("transmit=%s\n", do_transmit ? "true" : "false");
  if (do_transmit)
  {
    if (!tx_timeout_occured)
    {
      if (!squelch_is_open)
      {
	//printf("Squelch is NOT open. Transmitting...\n");
	rx().mute(true);
	Logic::transmit(true);
      }
      else
      {
	//printf("Pending transmit request...\n");
	pending_transmit = true;
      }
    }
  }
  else
  {
    pending_transmit = false;
    //printf("Resetting tx timeout timer\n");
    tx_timeout_occured = false;
    Logic::transmit(false);
    rx().mute(false);
  }
} /* SimplexLogic::transmit */


int SimplexLogic::transmitAudio(short *samples, int count)
{
  if (!tx_timeout_occured)
  {
    return Logic::transmitAudio(samples, count);
  }
  else
  {
    return count;
  }
} /* SimplexLogic::transmitAudio */



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


void SimplexLogic::identify(Timer *t)
{
  printf("SimplexLogic::identify\n");
  
  if (squelch_is_open)
  {
    pending_ident = true;
    return;
  }
  
  if (!callsign().empty())
  {
    playMsg("online");
    spellWord(callsign());
    ident_timer->reset();
  }
} /* SimplexLogic::identify */


void SimplexLogic::squelchOpen(bool is_open)
{
  printf("The squelch is %s\n", is_open ? "OPEN" : "CLOSED");
  
  squelch_is_open = is_open;
  
  if (!is_open)
  {
    if (pending_ident)
    {
      pending_ident = false;
      identify();
    }

    if (pending_transmit)
    {
      //printf("Executing pending transmit\n");
      pending_transmit = false;
      transmit(true);
    }
  }
    
  Logic::squelchOpen(is_open);
  
} /* SimplexLogic::squelchOpen */


void SimplexLogic::txTimeout(void)
{
  tx_timeout_occured = true;
  Logic::transmit(false);
  clearPendingSamples();
  rx().mute(false);
} /* SimplexLogic::txTimeout */







/*
 * This file has not been truncated
 */

