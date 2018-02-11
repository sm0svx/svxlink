/**
@file	 Tx.cpp
@brief   The base class for a transmitter
@author  Tobias Blomberg / SM0SVX
@date	 2006-08-01

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2018 Tobias Blomberg / SM0SVX

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



/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "Tx.h"
#include "LocalTx.h"
#include "NetTx.h"
#include "MultiTx.h"
#include "DummyRxTx.h"



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

class LocalTxFactory : public TxFactory
{
  public:
    LocalTxFactory(void) : TxFactory("Local") {}
    
  protected:
    Tx *createTx(Config &cfg, const string& name)
    {
      return new LocalTx(cfg, name);
    }
}; /* class LocalTxFactory */


class NetTxFactory : public TxFactory
{
  public:
    NetTxFactory(void) : TxFactory("Net") {}
    
  protected:
    Tx *createTx(Config &cfg, const string& name)
    {
      return new NetTx(cfg, name);
    }
}; /* class NetTxFactory */


class MultiTxFactory : public TxFactory
{
  public:
    MultiTxFactory(void) : TxFactory("Multi") {}
    
  protected:
    Tx *createTx(Config &cfg, const string& name)
    {
      return new MultiTx(cfg, name);
    }
}; /* class MultiTxFactory */


class DummyTxFactory : public TxFactory
{
  public:
    DummyTxFactory(void) : TxFactory("Dummy") {}
    
  protected:
    Tx *createTx(Config &cfg, const string& name)
    {
      return new DummyTx(name);
    }
}; /* class MultiTxFactory */



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

map<string, TxFactory*> TxFactory::tx_factories;



/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/

TxFactory::TxFactory(const string &name)
  : m_name(name)
{
  tx_factories[name] = this;
} /* TxFactory::TxFactory */


TxFactory::~TxFactory(void)
{
  std::map<std::string, TxFactory*>::iterator it;
  it = tx_factories.find(m_name);
  assert(it != tx_factories.end());
  tx_factories.erase(it);
} /* TxFactory::~TxFactory */


Tx *TxFactory::createNamedTx(Config& cfg, const string& name)
{
  LocalTxFactory local_tx_factory;
  NetTxFactory net_tx_factory;
  MultiTxFactory multi_tx_factory;
  DummyTxFactory dummy_tx_factory;
  
  string tx_type;
  if (name != "NONE")
  {
    if (!cfg.getValue(name, "TYPE", tx_type))
    {
      cerr << "*** ERROR: Config variable " << name << "/TYPE not set\n";
      return 0;
    }
  }
  else
  {
    tx_type = "Dummy";
  }
  
  map<string, TxFactory*>::iterator it;
  it = tx_factories.find(tx_type);
  if (it == tx_factories.end())
  {
    cerr << "*** ERROR: Unknown TX type \"" << tx_type << "\" for transmitter "
         << name << ". Legal values " << "are: ";
    for (it=tx_factories.begin(); it!=tx_factories.end(); ++it)
    {
      cerr << "\"" << (*it).first << "\" ";
    }
    cerr << endl;
    return 0;
  }
  
  return (*it).second->createTx(cfg, name);

} /* TxFactory::createNamedTx */



/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/

void Tx::setIsTransmitting(bool is_transmitting)
{
  if (is_transmitting != m_is_transmitting)
  {
    if (isVerbose())
    {
      cout << m_name << ": Turning the transmitter "
           << (is_transmitting ? "ON" : "OFF") << endl;
    }
    m_is_transmitting = is_transmitting;
    transmitterStateChange(is_transmitting);
  }
} /* Tx::setIsTransmitting */




/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/



/*
 * This file has not been truncated
 */
