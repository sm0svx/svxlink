/**
@file	 TxUplink.cpp
@brief   A simple uplink that just retransmits what comes into the receiver
@author  Tobias Blomberg / SM0SVX
@date	 2008-03-20

\verbatim
RemoteTrx - A remote receiver for the SvxLink server
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

#include "TxUplink.h"



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

TxUplink::TxUplink(Config &cfg, const string &name, Rx *rx, Tx *tx)
  : cfg(cfg), name(name), rx(rx), uplink_tx(0)
{
  
} /* TxUplink::TxUplink */


TxUplink::~TxUplink(void)
{
  delete uplink_tx;
} /* TxUplink::~TxUplink */


bool TxUplink::initialize(void)
{
  string uplink_tx_name;
  if (!cfg.getValue(name, "TX", uplink_tx_name))
  {
    cerr << "*** ERROR: Config variable " << name << "/TX not spoecified.\n";
    return false;
  }
  
  uplink_tx = Tx::create(cfg, uplink_tx_name);
  if ((uplink_tx == 0) || !uplink_tx->initialize())
  {
    cerr << "*** ERROR: Could not initialize uplink transmitter\n";
    delete uplink_tx;
    return false;
  }
  
  uplink_tx->setTxCtrlMode(Tx::TX_AUTO);
  rx->registerSink(uplink_tx);
  rx->reset();
  rx->mute(false);
  
  return true;
  
} /* TxUplink::initialize */



/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/




/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/





/*
 * This file has not been truncated
 */

