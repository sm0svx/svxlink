/**
@file	 TrxUplink.cpp
@brief   Uplink type that communicates to the SvxLink core through a transceiver
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
#include <algorithm>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <Rx.h>
#include <Tx.h>
#include <AsyncAudioDebugger.h>
#include <AsyncAudioFifo.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "TrxUplink.h"



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

TrxUplink::TrxUplink(Config &cfg, const string &name, Rx *rx, Tx *tx)
  : cfg(cfg), name(name), rx(rx), tx(tx), uplink_tx(0), uplink_rx(0)
{
  
} /* TrxUplink::TrxUplink */


TrxUplink::~TrxUplink(void)
{
  delete uplink_tx;
} /* TrxUplink::~TrxUplink */


bool TrxUplink::initialize(void)
{
  string uplink_tx_name;
  if (!cfg.getValue(name, "TX", uplink_tx_name))
  {
    cerr << "*** ERROR: Config variable " << name << "/TX not set.\n";
    return false;
  }

  string uplink_rx_name;
  if (!cfg.getValue(name, "RX", uplink_rx_name))
  {
    cerr << "*** ERROR: Config variable " << name << "/RX not set.\n";
    return false;
  }

  rx->squelchOpen.connect(slot(*this, &TrxUplink::rxSquelchOpen));
  rx->dtmfDigitDetected.connect(slot(*this, &TrxUplink::rxDtmfDigitDetected));
  rx->reset();
  rx->mute(false);
  AudioSource *prev_src = rx;
  
  AudioFifo *fifo = new AudioFifo(8000);
  fifo->setPrebufSamples(512);
  prev_src->registerSink(fifo);
  prev_src = fifo;
  
  uplink_tx = Tx::create(cfg, uplink_tx_name);
  if ((uplink_tx == 0) || !uplink_tx->initialize())
  {
    cerr << "*** ERROR: Could not initialize uplink transmitter\n";
    delete uplink_tx;
    uplink_tx = 0;
    return false;
  }
  uplink_tx->setTxCtrlMode(Tx::TX_AUTO);
  uplink_tx->enableCtcss(true);
  prev_src->registerSink(uplink_tx);
  prev_src = 0;
  
  
  uplink_rx = RxFactory::createNamedRx(cfg, uplink_rx_name);
  if ((uplink_rx == 0) || !uplink_rx->initialize())
  {
    delete uplink_tx;
    uplink_tx = 0;
    delete uplink_rx;
    uplink_rx = 0;
    return false;
  }
  uplink_rx->dtmfDigitDetected.connect(
      slot(*this, &TrxUplink::uplinkRxDtmfRcvd));
  uplink_rx->reset();
  uplink_rx->mute(false);
  prev_src = uplink_rx;
  
  tx->setTxCtrlMode(Tx::TX_AUTO);
  prev_src->registerSink(tx);
  
  return true;
  
} /* TrxUplink::initialize */



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

void TrxUplink::uplinkRxDtmfRcvd(char digit, int duration)
{
  char digit_str[2] = {digit, 0};
  tx->sendDtmf(digit_str);
} /* TrxUplink::uplinkRxDtmfRcvd */


void TrxUplink::rxSquelchOpen(bool is_open)
{
  /*
  if (is_open)
  {
    int ss = min(99, max(0, static_cast<int>(rx->signalStrength())));
    char dtmf_str[] = {'0' + ss / 10, 0};
    uplink_tx->sendDtmf(dtmf_str);
  }
  */
} /* TrxUplink::rxSquelchOpen  */


void TrxUplink::rxDtmfDigitDetected(char digit, int duration)
{
    // FIXME: DTMF digits should be retransmitted with the correct duration.
  const char dtmf_str[] = {digit, 0};
  uplink_tx->sendDtmf(dtmf_str);
} /* TrxUplink::rxDtmfDigitDetected */



/*
 * This file has not been truncated
 */

