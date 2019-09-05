/**
@file	 PttCtrl.cpp
@brief   Implements the PTT controller
@author  Tobias Blomberg / SM0SVX
@date	 2004-03-21

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

#include <sigc++/sigc++.h>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "PttCtrl.h"


/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/

using namespace Async;
using namespace sigc;



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


PttCtrl::PttCtrl(int tx_delay)
  : tx_ctrl_mode(Tx::TX_OFF), is_transmitting(false), tx_delay_timer(0),
    tx_delay(tx_delay), fifo(0)
{
  valve.setBlockWhenClosed(false);
  valve.setOpen(false);
      
  if (tx_delay > 0)
  {
    fifo = new AudioFifo((tx_delay + 500) * INTERNAL_SAMPLE_RATE / 1000);
    fifo->registerSink(&valve);
    AudioSink::setHandler(fifo);
  }
  else
  {
    AudioSink::setHandler(&valve);
  }
      
  AudioSource::setHandler(&valve);
}


PttCtrl::~PttCtrl(void)
{
  AudioSink::clearHandler();
  AudioSource::clearHandler();
  delete fifo;
  delete tx_delay_timer;
}


void PttCtrl::setTxCtrlMode(Tx::TxCtrlMode mode)
{
  if (mode == tx_ctrl_mode)
  {
    return;
  }
  tx_ctrl_mode = mode;
      
  switch (mode)
  {
    case Tx::TX_OFF:
      transmit(false);
      break;

    case Tx::TX_ON:
      transmit(true);
      break;

    case Tx::TX_AUTO:
      transmit(!valve.isIdle());
      break;
  }
}


int PttCtrl::writeSamples(const float *samples, int count)
{
  if ((tx_ctrl_mode == Tx::TX_AUTO) && !is_transmitting)
  {
    transmit(true);
  }
      
  if (fifo != 0)
  {
    return fifo->writeSamples(samples, count);
  }
      
  return valve.writeSamples(samples, count);
}


void PttCtrl::allSamplesFlushed(void)
{
  valve.allSamplesFlushed();
  if ((tx_ctrl_mode == Tx::TX_AUTO) && is_transmitting && valve.isIdle())
  {
    if (!preTransmitterStateChange(false))
    {
      transmit(false);
    }
  }
}



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

void PttCtrl::transmit(bool do_transmit)
{
  if (do_transmit == is_transmitting)
  {
    return;
  }
      
  is_transmitting = do_transmit;
  transmitterStateChange(do_transmit);
  if (do_transmit)
  {
    if (tx_delay > 0)
    {
      fifo->enableBuffering(true);
      valve.setBlockWhenClosed(true);
      tx_delay_timer = new Timer(tx_delay);
      tx_delay_timer->expired.connect(
          mem_fun(*this, &PttCtrl::txDelayExpired));
    }
    else
    {
      valve.setOpen(true);
    }
  }
  else
  {
    if (tx_delay_timer != 0)
    {
      delete tx_delay_timer;
      tx_delay_timer = 0;
    }
    valve.setBlockWhenClosed(false);
    valve.setOpen(false);
  }
}


void PttCtrl::txDelayExpired(Timer *t)
{
  delete tx_delay_timer;
  tx_delay_timer = 0;
  fifo->enableBuffering(false);
  valve.setOpen(true);
  valve.setBlockWhenClosed(false);
}


/*
 * This file has not been truncated
 */

