/**
@file	 Rx.cpp
@brief   The base class for a receiver
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

#include <iostream>
#include <cstdlib>
#include <string.h>



/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncTimer.h>
#include <AsyncConfig.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "Rx.h"
#include "LocalRx.h"
#include "Voter.h"
#include "NetRx.h"
#include "DummyRxTx.h"
#include "Ddr.h"
#include "LocalRxSim.h"



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

class LocalRxFactory : public RxFactory
{
  public:
    LocalRxFactory(void) : RxFactory("Local") {}
    
  protected:
    Rx *createRx(Config &cfg, const string& name)
    {
      return new LocalRx(cfg, name);
    }
}; /* class LocalRxFactory */


class VoterFactory : public RxFactory
{
  public:
    VoterFactory(void) : RxFactory("Voter") {}
    
  protected:
    Rx *createRx(Config &cfg, const string& name)
    {
      return new Voter(cfg, name);
    }
}; /* class VoterFactory */


class NetRxFactory : public RxFactory
{
  public:
    NetRxFactory(void) : RxFactory("Net") {}
    
  protected:
    Rx *createRx(Config &cfg, const string& name)
    {
      return new NetRx(cfg, name);
    }
}; /* class NetRxFactory */


class DummyRxFactory : public RxFactory
{
  public:
    DummyRxFactory(void) : RxFactory("Dummy") {}
    
  protected:
    Rx *createRx(Config &cfg, const string& name)
    {
      return new DummyRx(cfg, name);
    }
}; /* class DummyRxFactory */


class DdrFactory : public RxFactory
{
  public:
    DdrFactory(void) : RxFactory("Ddr") {}
    
  protected:
    Rx *createRx(Config &cfg, const string& name)
    {
      return new Ddr(cfg, name);
    }
}; /* class DdrFactory */


class LocalRxSimFactory : public RxFactory
{
  public:
    LocalRxSimFactory(void) : RxFactory("LocalSim") {}
    
  protected:
    Rx *createRx(Config &cfg, const string& name)
    {
      return new LocalRxSim(cfg, name);
    }
}; /* class LocalRxSimFactory */


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

map<string, RxFactory*> RxFactory::rx_factories;



/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/
void Rx::setEnabled(bool status)
{
  cout << "RX: " << m_name << ": setEnabled: " << status << endl;
  m_is_enabled = status;
  if (! status) {
    // close squelch, but set m_sql_open_hidden to what it WAS
    int was_open = m_sql_open;
    setSquelchState(false);
    m_sql_open_hidden = was_open;

  } else {
    cout << "Rx:setEnabled: re-enable, set SQ to " << m_sql_open_hidden << endl;
    // Set squelch to the correct, current status
    setSquelchState(m_sql_open_hidden);
  }
}

bool Rx::isEnabled(void)
{
  return m_is_enabled;
}



std::string Rx::muteStateToString(MuteState mute_state)
{
  switch (mute_state)
  {
    case MUTE_NONE:
      return "NONE";
    case MUTE_CONTENT:
      return "CONTENT";
    case MUTE_ALL:
      return "ALL";
  }
  return "?";
} /* Rx::muteStateToString */


Rx::Rx(Config &cfg, const string& name)
  : m_name(name), m_verbose(true), m_sql_open(false), m_sql_open_hidden(false), m_cfg(cfg),
    m_sql_tmo_timer(0), m_is_enabled(true)
{
} /* Rx::Rx */


Rx::~Rx(void)
{
  delete m_sql_tmo_timer;
} /* Rx::~Rx */


bool Rx::initialize(void)
{
  /*
  string value;
  if (m_cfg.getValue(name(), "SQL_TIMEOUT", value))
  {
    int tmo_val = atoi(value.c_str()) * 1000;
    if (tmo_val > 0)
    {
      m_sql_tmo_timer = new Timer(tmo_val);
      m_sql_tmo_timer->expired.connect(slot(*this, &Rx::sqlTimeout));
      m_sql_tmo_timer->setEnable(false);
    }
  }
  */
  
  return true;
  
} /* Rx::initialize */


RxFactory::RxFactory(const string &name)
  : m_name(name)
{
  rx_factories[name] = this;
} /* RxFactory::RxFactory */


RxFactory::~RxFactory(void)
{
  std::map<std::string, RxFactory*>::iterator it;
  it = rx_factories.find(m_name);
  assert(it != rx_factories.end());
  rx_factories.erase(it);
} /* RxFactory::~RxFactory */


Rx *RxFactory::createNamedRx(Config& cfg, const string& name)
{
  LocalRxFactory local_rx_factory;
  VoterFactory voter_factory;
  NetRxFactory net_rx_factory;
  DummyRxFactory dummy_rx_factory;
  DdrFactory ddr_rx_factory;
  LocalRxSimFactory local_rx_sim_factory;
  
  string rx_type;
  if (name != "NONE")
  {
    if (!cfg.getValue(name, "TYPE", rx_type))
    {
      cerr << "*** ERROR: Config variable " << name << "/TYPE not set\n";
      return 0;
    }
  }
  else
  {
    rx_type = "Dummy";
  }
  
  map<string, RxFactory*>::iterator it;
  it = rx_factories.find(rx_type);
  if (it == rx_factories.end())
  {
    cerr << "*** ERROR: Unknown RX type \"" << rx_type << "\" specified for "
         << "receiver " << name << ". Legal values are: ";
    for (it=rx_factories.begin(); it!=rx_factories.end(); ++it)
    {
      cerr << "\"" << (*it).first << "\" ";
    }
    cerr << endl;
    return 0;
  }
  
  return (*it).second->createRx(cfg, name);

} /* RxFactory::createNamedRx */



/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/

void Rx::setSquelchState(bool is_open)
{
  m_sql_open_hidden = is_open;

  if (is_open && not m_is_enabled)
  {
    // cout << m_name << ": Refusing to open squelch, we're disabled" << endl;
    return;
  }

  if (is_open == m_sql_open)
  {
    return;
  }
  
  if (m_verbose)
  {
    cout << m_name << ": The squelch is " << (is_open ? "OPEN" : "CLOSED")
         << " (" << signalStrength() << ")" << endl;
  }
  m_sql_open = is_open;
  squelchOpen(is_open);
  
  if (m_sql_tmo_timer != 0)
  {
    m_sql_tmo_timer->setEnable(is_open);
  }
} /* Rx::setSquelchState */



/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/

void Rx::sqlTimeout(Timer *t)
{
  cerr << "*** WARNING: The squelch was open for too long for receiver "
       << name() << ". Forcing it closed.\n";
  setSquelchState(false);
} /* Rx::sqlTimeout */



/*
 * This file has not been truncated
 */

