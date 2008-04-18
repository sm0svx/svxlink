/**
@file	 TrxHandler.cpp
@brief   This file contains the receiver handler class
@author  Tobias Blomberg / SM0SVX
@date	 2006-04-14

\verbatim
RemoteRx - A remote receiver for the SvxLink server
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
    DummyRx(void) : Rx("DummyRx") {}
    virtual ~DummyRx(void) {}
    virtual bool initialize(void) { return true; }
    virtual void mute(bool do_mute) {}
    virtual void reset(void) {}
    virtual void resumeOutput(void) {}
    virtual void allSamplesFlushed(void) {}
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

TrxHandler::TrxHandler(Async::Config &cfg)
  : cfg(cfg), uplink(0), rx(0), tx(0)
{
  
} /* TrxHandler::TrxHandler */


TrxHandler::~TrxHandler(void)
{
  delete uplink;
  delete rx;
} /* TrxHandler::~TrxHandler */


bool TrxHandler::initialize(void)
{
  string rx_name;
  if (!cfg.getValue("GLOBAL", "RX", rx_name))
  {
    cerr << "*** ERROR: Configuration variable GLOBAL/RX not set.\n";
    return false;
  }

  string tx_name;
  if (!cfg.getValue("GLOBAL", "TX", tx_name))
  {
    cerr << "*** ERROR: Configuration variable GLOBAL/TX not set.\n";
    return false;
  }

  string uplink_name;
  if (!cfg.getValue("GLOBAL", "UPLINK", uplink_name))
  {
    cerr << "*** ERROR: Configuration variable GLOBAL/UPLINK not set.\n";
    return false;
  }
  
  if (rx_name != "NONE")
  {
    string rx_type;
    if (!cfg.getValue(rx_name, "TYPE", rx_type))
    {
      cerr << "*** ERROR: " << rx_name << "/TYPE not set\n";
      cleanup();
      return false;
    }
    if (rx_type == "NetTrxAdapter")
    {
      NetTrxAdapter *adapter = NetTrxAdapter::instance(cfg, rx_name);
      if (adapter != 0)
      {
      	rx = adapter->rx();
      }
    }
    else
    {
      rx = Rx::create(cfg, rx_name);
    }
    if ((rx == 0) || !rx->initialize())
    {
      cerr << "*** ERROR: Could not initialize Rx object \""
      	   << rx_name << "\".\n";
      cleanup();
      return false;
    }
  }
  else
  {
    rx = new DummyRx;
  }

  if (tx_name != "NONE")
  {
    string tx_type;
    if (!cfg.getValue(tx_name, "TYPE", tx_type))
    {
      cerr << "*** ERROR: " << tx_name << "/TYPE not set\n";
      cleanup();
      return false;
    }
    if (tx_type == "NetTrxAdapter")
    {
      NetTrxAdapter *adapter = NetTrxAdapter::instance(cfg, tx_name);
      if (adapter != 0)
      {
      	tx = adapter->tx();
      }
    }
    else
    {
      tx = Tx::create(cfg, tx_name);
    }
    if ((tx == 0) || !tx->initialize())
    {
      cerr << "*** ERROR: Could not initialize Tx object \""
      	   << tx_name << "\".\n";
      cleanup();
      return false;
    }
  }
  else
  {
    tx = new DummyTx;
  }
    
  uplink = Uplink::create(cfg, uplink_name, rx, tx);
  if ((uplink == 0) || !uplink->initialize())
  {
    cerr << "*** ERROR: Could not initialize Uplink object \""
      	 << uplink_name << "\".\n";
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
  delete rx;
  rx = 0;
  delete tx;
  tx = 0;
  delete uplink;
  uplink = 0;
} /* TrxHandler::cleanup */



/*
 * This file has not been truncated
 */

