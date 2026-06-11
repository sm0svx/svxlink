/**
@file	 SimplexLogic.cpp
@brief   Contains a simplex logic SvxLink core implementation
@author  Tobias Blomberg / SM0SVX
@date	 2004-03-23

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2022 Tobias Blomberg / SM0SVX

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
 * Exported Global functions
 *
 ****************************************************************************/

extern "C" {
  LogicBase* construct(void) { return new SimplexLogic; }
}


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


SimplexLogic::SimplexLogic(void)
  : mute_rx_on_tx(true), mute_tx_on_rx(true),
    rgr_sound_always(false)
{
} /* SimplexLogic::SimplexLogic */


bool SimplexLogic::initialize(Async::Config& cfgobj, const string& logic_name)
{
  if (!Logic::initialize(cfgobj, logic_name))
  {
    return false;
  }
  
  cfg().getValue(name(), "MUTE_RX_ON_TX", mute_rx_on_tx);
  cfg().getValue(name(), "MUTE_TX_ON_RX", mute_tx_on_rx);
  cfg().getValue(name(), "RGR_SOUND_ALWAYS", rgr_sound_always);
  
  m_sub_mute_rx_on_tx = cfg().subscribeValue(name(), "MUTE_RX_ON_TX", mute_rx_on_tx,
      [&](bool value) { mute_rx_on_tx = value; });
  m_sub_mute_tx_on_rx = cfg().subscribeValue(name(), "MUTE_TX_ON_RX", mute_tx_on_rx,
      [&](bool value) { mute_tx_on_rx = value; });
  m_sub_rgr_sound_always = cfg().subscribeValue(name(), "RGR_SOUND_ALWAYS", rgr_sound_always,
      [&](bool value) { rgr_sound_always = value; });

  rxValveSetOpen(true);
  setTxCtrlMode(Tx::TX_AUTO);
  
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
  
} /* SimplexLogic::squelchOpen */


void SimplexLogic::transmitterStateChange(bool is_transmitting)
{
  if (mute_rx_on_tx)
  {
    rx().setMuteState(is_transmitting ? Rx::MUTE_ALL : Rx::MUTE_NONE);
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

