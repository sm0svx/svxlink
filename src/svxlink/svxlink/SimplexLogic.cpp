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
#include "Module.h"
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
  : Logic(cfg, name), pending_transmit(false),
    tx_timeout_occured(false), ident_timer(0), ident_interval(0),
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

    // This is off by default and isn't required to exist
  ident_only_after_tx = 0;
  if (cfg().getValue(name(), "IDENT_ONLY_AFTER_TX", str))
  {
    ident_only_after_tx = atoi(str.c_str());
  }
  
  //tx().txTimeout.connect(slot(this, &SimplexLogic::txTimeout));
  
  if (ident_interval > 0)
  {
    ident_timer = new Timer(ident_interval, Timer::TYPE_PERIODIC);
    ident_timer->expired.connect(slot(this, &SimplexLogic::identify));
  }
      
  need_ident = false;
  ident_last_sent = time(NULL);

  rx().mute(false);
  
  return true;
  
} /* SimplexLogic::initialize */


void SimplexLogic::transmit(bool do_transmit)
{
  //printf("SimplexLogic::transmit: do_transmit=%s\n",
  //  	  do_transmit ? "true" : "false");
  if (do_transmit)
  {
    if (!tx_timeout_occured)
    {
      if (!rx().squelchIsOpen())
      {
	//printf("Squelch is NOT open. Transmitting...\n");

	  // If ident_only_after_tx set, flag that we transmitted and will need
	  // to ID ourselves -- but only if we didn't just finish IDing already.
	if (ident_only_after_tx && !need_ident)
	{
	  if (time(NULL) - ident_last_sent > ident_only_after_tx) 
	  {
	    need_ident = true;
	  }
	}
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
  //printf("SimplexLogic::identify\n");
  
  if (rx().squelchIsOpen() ||
      ((activeModule() != 0) && (activeModule()->isTransmitting())))
  {
    pending_ident = true;
    return;
  }
  
  if (!callsign().empty())
  {
    if (!ident_only_after_tx || need_ident) 
    {
      playMsg("online");
      spellWord(callsign());
      need_ident = false;
      ident_last_sent = time(NULL);
    }
    
    if (ident_timer != 0)
    {
      ident_timer->reset();
    }
  }
} /* SimplexLogic::identify */


void SimplexLogic::squelchOpen(bool is_open)
{
  printf("The squelch is %s\n", is_open ? "OPEN" : "CLOSED");
  
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


#if 0
void SimplexLogic::txTimeout(void)
{
  tx_timeout_occured = true;
  Logic::transmit(false);
  clearPendingSamples();
  rx().mute(false);
} /* SimplexLogic::txTimeout */
#endif






/*
 * This file has not been truncated
 */

