/**
@file	 SimplexLogic.cpp
@brief   Contains a simplex logic SvxLink core implementation
@author  Tobias Blomberg / SM0SVX
@date	 2004-03-23

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

#include <cstdio>
#include <cstdlib>
#include <iostream>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <Rx.h>
#include <Tx.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

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
  : Logic(cfg, name), mute_rx_on_tx(true)
{
} /* SimplexLogic::SimplexLogic */


SimplexLogic::~SimplexLogic(void)
{
} /* SimplexLogic::~SimplexLogic */


bool SimplexLogic::initialize(void)
{
  if (!Logic::initialize())
  {
    return false;
  }
  
  string value;
  if (cfg().getValue(name(), "MUTE_RX_ON_TX", value))
  {
    mute_rx_on_tx = (atoi(value.c_str()) > 0);
  }
  
  rxValveSetOpen(true);
  tx().setTxCtrlMode(Tx::TX_AUTO);
  
  processEvent("startup");
  
  return true;
  
} /* SimplexLogic::initialize */



/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/

void SimplexLogic::squelchOpen(bool is_open)
{
  //cout << name() << ": The squelch is " << (is_open ? "OPEN" : "CLOSED")
  //     << endl;
  
  if (!is_open)
  {
    if (activeModule() != 0)
    {
      enableRgrSoundTimer(true);
    }
    
    tx().setTxCtrlMode(Tx::TX_AUTO);
  }
  else
  {
    enableRgrSoundTimer(false);
    tx().setTxCtrlMode(Tx::TX_OFF);
  }
    
  Logic::squelchOpen(is_open);
  
} /* SimplexLogic::squelchOpen */


void SimplexLogic::transmitterStateChange(bool is_transmitting)
{
  if (mute_rx_on_tx)
  {
    rx().mute(is_transmitting);
  }
  Logic::transmitterStateChange(is_transmitting);
} /* SimplexLogic::transmitterStateChange */




/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/



/*
 * This file has not been truncated
 */

