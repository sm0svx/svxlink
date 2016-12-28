/**
@file	 MultiTx.cpp
@brief   Make it possible to use multiple transmitters for one logic
@author  Tobias Blomberg / SM0SVX
@date	 2008-07-08

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

#include <iostream>
#include <algorithm>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncAudioSplitter.h>



/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "MultiTx.h"



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

MultiTx::MultiTx(Config& cfg, const string& name)
  : cfg(cfg), m_name(name), splitter(0)
{
  
} /* MultiTx::MultiTx */


MultiTx::~MultiTx(void)
{
  clearHandler();

  list<Tx *>::iterator it;
  for (it=txs.begin(); it!=txs.end(); ++it)
  {
    delete *it;
  }
  txs.clear();
  
  delete splitter;
  
} /* MultiTx::~MultiTx */


bool MultiTx::initialize(void)
{
  string transmitters;
  if (!cfg.getValue(m_name, "TRANSMITTERS", transmitters))
  {
    cerr << "*** ERROR: Config variable " << m_name
      	 << "/TRANSMITTERS not set\n";
    return false;
  }
  
  splitter = new AudioSplitter;
  
  string::iterator start(transmitters.begin());
  for (;;)
  {
    string::iterator comma = find(start, transmitters.end(), ',');
    string tx_name(start, comma);
    if (!tx_name.empty())
    {
      cout << "\tAdding transmitter: " << tx_name << endl;
      Tx *tx = TxFactory::createNamedTx(cfg, tx_name);
      if ((tx == 0) || !tx->initialize())
      {
      	// FIXME: Cleanup
      	return false;
      }
      tx->txTimeout.connect(txTimeout.make_slot());
      tx->transmitterStateChange.connect(
      	      mem_fun(*this, &MultiTx::onTransmitterStateChange));
      
      splitter->addSink(tx);
      
      txs.push_back(tx);
    }
    if (comma == transmitters.end())
    {
      break;
    }
    start = comma;
    ++start;
  }
  
  setHandler(splitter);
  
  return true;
  
} /* MultiTx::initialize */


void MultiTx::setTxCtrlMode(TxCtrlMode mode)
{
  list<Tx *>::iterator it;
  for (it=txs.begin(); it!=txs.end(); ++it)
  {
    (*it)->setTxCtrlMode(mode);
  }  
} /* MultiTx::setTxCtrlMode */


bool MultiTx::isTransmitting(void) const
{
  bool is_transmitting = false;
  list<Tx *>::const_iterator it;
  for (it=txs.begin(); it!=txs.end(); ++it)
  {
    is_transmitting |= (*it)->isTransmitting();
  }
  
  return is_transmitting;
  
} /* MultiTx::isTransmitting */


void MultiTx::enableCtcss(bool enable)
{
  list<Tx *>::iterator it;
  for (it=txs.begin(); it!=txs.end(); ++it)
  {
    (*it)->enableCtcss(enable);
  }
} /* MultiTx::enableCtcss */


void MultiTx::sendDtmf(const std::string& digits)
{
  list<Tx *>::iterator it;
  for (it=txs.begin(); it!=txs.end(); ++it)
  {
    (*it)->sendDtmf(digits);
  }
} /* MultiTx::sendDtmf */


void MultiTx::setTransmittedSignalStrength(float siglev)
{
  list<Tx *>::iterator it;
  for (it=txs.begin(); it!=txs.end(); ++it)
  {
    (*it)->setTransmittedSignalStrength(siglev);
  }
} /* MultiTx::setTransmittedSignalStrength */



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

void MultiTx::onTransmitterStateChange(bool is_transmitting)
{
  if (is_transmitting == isTransmitting())
  {
    transmitterStateChange(is_transmitting);
  }
} /* MultiTx::onTransmitterStateChange */



/*
 * This file has not been truncated
 */

