/**
@file	 Tx.cpp
@brief   The base class for a transmitter
@author  Tobias Blomberg / SM0SVX
@date	 2006-08-01

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
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

#include <iostream>



/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

//#include <AsyncAudioPassthrough.h>



/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "Tx.h"
#include "LocalTx.h"



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

#if 0
class Tx::InputHandler : public AudioPassthrough
{
  public:
    explicit InputHandler(Tx *tx)
      : tx(tx), is_flushing(false), samples_to_flush(false),
      	auto_tx(false), is_transmitting(false)
    {
    }
    
    bool isFlushing(void) const { return is_flushing; }
    //bool samplesToFlush(void) const { return samples_to_flush; }
    void enableAutoTx(bool enable)
    {
      auto_tx = enable;
      if (auto_tx)
      {
      	transmit(samples_to_flush);
      }
      else
      {
      	is_transmitting = false;
      }
    }
    
    int writeSamples(const float *samples, int count)
    {
      is_flushing = false;
      samples_to_flush = true;
      if (auto_tx && !is_transmitting)
      {
      	transmit(true);
      }
      return sinkWriteSamples(samples, count);
    }
    
    void flushSamples(void)
    {
      is_flushing = true;
      sinkFlushSamples();
    }
    
    void allSamplesFlushed(void)
    {
      is_flushing = false;
      samples_to_flush = false;
      sourceAllSamplesFlushed();
      if (auto_tx && is_transmitting && !samples_to_flush)
      {
      	transmit(false);
      }
    }
  
  private:
    Tx	  *tx;
    bool  is_flushing;
    bool  samples_to_flush;
    bool  auto_tx;
    bool  is_transmitting;
    
    void transmit(bool do_transmit)
    {
      is_transmitting = do_transmit;
      tx->transmit(do_transmit);
    }
    
}; /* class Tx::InputHandler */
#endif


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

Tx *Tx::create(Config& cfg, const string& name)
{
  Tx *tx = 0;
  string tx_type;
  if (!cfg.getValue(name, "TYPE", tx_type))
  {
    cerr << "*** ERROR: Config variable " << name << "/TYPE not set\n";
    return 0;
  }
  
  if (tx_type == "Local")
  {
    tx = new LocalTx(cfg, name);
  }
  else
  {
    cerr << "*** ERROR: Unknown TX type \"" << tx_type << "\". Legal values "
      	 << "are: \"Local\"\n";
    return 0;
  }
  
  return tx;
  
} /* Tx::create */


#if 0
Tx::Tx(const std::string name)
  : input_handler(0)
{
  
} /* Tx::Tx */


Tx::~Tx(void)
{
  clearHandler();
  delete input_handler;
} /* Tx::~Tx */


bool Tx::initialize(void)
{
  input_handler = new InputHandler(this);
  if (input_handler == 0)
  {
    return false;
  }
  setHandler(input_handler);
  
  return true;
  
} /* Tx::initialize */


void Tx::setTxCtrlMode(Tx::TxCtrlMode mode)
{
  switch (mode)
  {
    case TX_OFF:
      input_handler->enableAutoTx(false);
      transmit(false);
      break;
      
    case TX_ON:
      input_handler->enableAutoTx(false);
      transmit(true);
      break;
    
    case TX_AUTO:
      input_handler->enableAutoTx(true);
      break;
  }
  
} /* Tx::setTxCtrlMode */


bool Tx::isFlushing(void) const
{
  return input_handler->isFlushing();
}
#endif


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


/*
 *----------------------------------------------------------------------------
 * Method:    
 * Purpose:   
 * Input:     
 * Output:    
 * Author:    
 * Created:   
 * Remarks:   
 * Bugs:      
 *----------------------------------------------------------------------------
 */







/*
 * This file has not been truncated
 */

