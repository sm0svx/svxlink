/**
@file	 TrxHandler.cpp
@brief   This file contains the receiver handler class
@author  Tobias Blomberg / SM0SVX
@date	 2006-04-14

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
#include <string>


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

#include "TrxHandler.h"
#include "Rx.h"
#include "Tx.h"
#include "Uplink.h"
#include "NetTrxAdapter.h"


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

class DummyTx : public Tx
{
  public:
    DummyTx(void) {}
    virtual ~DummyTx(void) {}
    virtual bool initialize(void) { return true; }
    virtual void setTxCtrlMode(TxCtrlMode mode) {}
    virtual bool isTransmitting(void) const { return false; }
    virtual int writeSamples(const float *samples, int count) { return count; }
    virtual void flushSamples(void) { sourceAllSamplesFlushed(); }
};


class DummyRx : public Rx
{
  public:
    DummyRx(void) : Rx(cfg, "DummyRx") {}
    virtual ~DummyRx(void) {}
    virtual bool initialize(void) { return Rx::initialize(); }
    virtual void mute(bool do_mute) {}
    virtual void reset(void) {}
    virtual void resumeOutput(void) {}
    virtual void allSamplesFlushed(void) {}
  
  private:
    Config  cfg;
};


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

TrxHandler::TrxHandler(Async::Config &cfg, const std::string &name)
  : m_cfg(cfg), m_name(name), m_uplink(0), m_rx(0), m_tx(0)
{
  
} /* TrxHandler::TrxHandler */


TrxHandler::~TrxHandler(void)
{
  delete m_uplink;
  delete m_rx;
  delete m_tx;
} /* TrxHandler::~TrxHandler */


bool TrxHandler::initialize(void)
{
  string rx_name;
  if (!m_cfg.getValue(m_name, "RX", rx_name))
  {
    cerr << "*** ERROR: Configuration variable " << m_name << "/RX not set.\n";
    return false;
  }

  string tx_name;
  if (!m_cfg.getValue(m_name, "TX", tx_name))
  {
    cerr << "*** ERROR: Configuration variable " << m_name << "/TX not set.\n";
    return false;
  }

  if (rx_name != "NONE")
  {
    string rx_type;
    if (!m_cfg.getValue(rx_name, "TYPE", rx_type))
    {
      cerr << "*** ERROR: " << rx_name << "/TYPE not set\n";
      cleanup();
      return false;
    }
    m_rx = RxFactory::createNamedRx(m_cfg, rx_name);
    if ((m_rx == 0) || !m_rx->initialize())
    {
      cerr << "*** ERROR: Could not initialize rx object \""
      	   << rx_name << "\" for trx \"" << m_name << "\".\n";
      cleanup();
      return false;
    }
  }
  else
  {
    m_rx = new DummyRx;
  }

  if (tx_name != "NONE")
  {
    string tx_type;
    if (!m_cfg.getValue(tx_name, "TYPE", tx_type))
    {
      cerr << "*** ERROR: " << tx_name << "/TYPE not set\n";
      cleanup();
      return false;
    }
    m_tx = TxFactory::createNamedTx(m_cfg, tx_name);
    if ((m_tx == 0) || !m_tx->initialize())
    {
      cerr << "*** ERROR: Could not initialize tx object \""
      	   << tx_name << "\" for trx \"" << m_name << "\".\n";
      cleanup();
      return false;
    }
  }
  else
  {
    m_tx = new DummyTx;
  }
    
  m_uplink = Uplink::create(m_cfg, m_name, m_rx, m_tx);
  if ((m_uplink == 0) || !m_uplink->initialize())
  {
    cerr << "*** ERROR: Could not initialize uplink object for \""
      	 << m_name << "\".\n";
    cleanup();
    return false;
  }
  
  return true;
  
} /* TrxHandler::initialize */



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


void TrxHandler::cleanup(void)
{
  delete m_rx;
  m_rx = 0;
  delete m_tx;
  m_tx = 0;
  delete m_uplink;
  m_uplink = 0;
} /* TrxHandler::cleanup */



/*
 * This file has not been truncated
 */

