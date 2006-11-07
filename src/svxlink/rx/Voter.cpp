/**
@file	 Voter.cpp
@brief   This file contains a class that implement a receiver voter
@author  Tobias Blomberg / SM0SVX
@date	 2005-04-18

\verbatim
<A brief description of the program or library this file belongs to>
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
#include <cassert>
#include <cmath>
#include <pair.h>
#include <list>
#include <sigc++/bind.h>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncTimer.h>
#include <AsyncSampleFifo.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "Voter.h"
//#include "LocalRx.h"



/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/

using namespace std;
using namespace SigC;
using namespace Async;



/****************************************************************************
 *
 * Defines & typedefs
 *
 ****************************************************************************/

#define BEST_RX_SIGLEV_RESET  -100
#define MAX_VOTING_DELAY      5000


/****************************************************************************
 *
 * Local class definitions
 *
 ****************************************************************************/

class SatRx : public SigC::Object
{
  public:
    int id;
    Rx *rx;
    SampleFifo fifo;
    
    SatRx(int id, Rx *rx)
      : id(id), rx(rx), fifo(MAX_VOTING_DELAY), stop_output(true)
    {
      fifo.stopOutput(true);
      fifo.setOverwrite(true);
      fifo.setDebugName("SatRx");
      rx->audioReceived.connect(slot(fifo, &SampleFifo::addSamples));
      rx->dtmfDigitDetected.connect(slot(*this, &SatRx::onDtmfDigitDetected));
    }
    
    ~SatRx(void) {}
    
    void stopOutput(bool do_stop)
    {
      fifo.stopOutput(do_stop);
      stop_output = do_stop;
      if (!do_stop)
      {
      	DtmfBuf::iterator it;
      	for (it=dtmf_buf.begin(); it!=dtmf_buf.end(); ++it)
	{
	  dtmfDigitDetected((*it).first, (*it).second);
	}
      }
    }
    
    void clear(void)
    {
      fifo.clear();
      dtmf_buf.clear();
    }
  
    Signal2<void, char, int> dtmfDigitDetected;
    
  private:
    typedef list<pair<char, int> >  DtmfBuf;
    
    bool    stop_output;
    DtmfBuf dtmf_buf;
    
    void onDtmfDigitDetected(char digit, int duration)
    {
      if (stop_output)
      {
	dtmf_buf.push_back(pair<char, int>(digit, duration));
      }
      else
      {
      	dtmfDigitDetected(digit, duration);
      }
    }
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

Voter::Voter(Config &cfg, const std::string& name)
  : Rx(cfg, name), active_rx(0), is_muted(true), m_verbose(true),
    best_rx(0), best_rx_siglev(BEST_RX_SIGLEV_RESET), best_rx_timer(0),
    voting_delay(0), sql_rx_id(0)
{
  Rx::setVerbose(false);
} /* Voter::Voter */


Voter::~Voter(void)
{
  delete best_rx_timer;
  
  list<SatRx *>::iterator it;
  for (it=rxs.begin(); it!=rxs.end(); ++it)
  {
    delete (*it)->rx;
    delete *it;
  }
  rxs.clear();
} /* Voter::~Voter */


bool Voter::initialize(void)
{
  string receivers;
  if (!cfg().getValue(name(), "RECEIVERS", receivers))
  {
    cerr << "*** ERROR: Config variable " << name() << "/RECEIVERS not set\n";
    return false;
  }

  string value;
  if (cfg().getValue(name(), "VOTING_DELAY", value))
  {
    voting_delay = atoi(value.c_str());
  }

  string::iterator start(receivers.begin());
  for (;;)
  {
    string::iterator comma = find(start, receivers.end(), ',');
    string rx_name(start, comma);
    if (!rx_name.empty())
    {
      cout << "Adding receiver to Voter: " << rx_name << endl;
      SatRx *srx = new SatRx(rxs.size() + 1, Rx::create(cfg(), rx_name));
      Rx *rx = srx->rx;
      if ((srx == 0) || !rx->initialize())
      {
      	return false;
      }
      rx->mute(true);
      rx->setVerbose(false);
      rx->squelchOpen.connect(bind(slot(*this, &Voter::satSquelchOpen), srx));
      srx->fifo.writeSamples.connect(audioReceived.slot());
      srx->dtmfDigitDetected.connect(dtmfDigitDetected.slot());
      rx->toneDetected.connect(toneDetected.slot());
      
      rxs.push_back(srx);
    }
    if (comma == receivers.end())
    {
      break;
    }
    start = comma;
    ++start;
  }
  
  return true;
  
} /* Voter::initialize */


void Voter::mute(bool do_mute)
{
  //cout << "Voter::mute: do_mute=" << (do_mute ? "TRUE" : "FALSE") << endl;
  
  if (do_mute == is_muted)
  {
    return;
  }
  
  if (active_rx != 0)
  {
    assert(!is_muted);
    active_rx = 0;
  }
  else if (best_rx_timer != 0)
  {
    assert(!is_muted);
    delete best_rx_timer;
    best_rx_timer = 0;
    best_rx = 0;
    best_rx_siglev = BEST_RX_SIGLEV_RESET;
  }
  
  list<SatRx *>::iterator it;
  for (it=rxs.begin(); it!=rxs.end(); ++it)
  {
    Rx *rx = (*it)->rx;
    rx->mute(do_mute);
  }
    
  is_muted = do_mute;
  
} /* Voter::mute */


bool Voter::squelchIsOpen(void) const
{
  return active_rx != 0;
} /* Voter::squelchIsOpen */


bool Voter::addToneDetector(float fq, int bw, float thresh,
      	      	      	    int required_duration)
{
  bool success = true;
  list<SatRx *>::iterator it;
  for (it=rxs.begin(); it!=rxs.end(); ++it)
  {
    Rx *rx = (*it)->rx;
    success &= rx->addToneDetector(fq, bw, thresh, required_duration);
  }
  
  return success;
  
} /* Voter::addToneDetector */


int Voter::sqlRxId(void) const
{
  return sql_rx_id;
} /* Voter::sqlRxId */


void Voter::reset(void)
{
  list<SatRx *>::iterator it;
  for (it=rxs.begin(); it!=rxs.end(); ++it)
  {
    Rx *rx = (*it)->rx;
    rx->reset();
  }
  
  is_muted = true;
  active_rx = 0;
  best_rx = 0;
  best_rx_siglev = BEST_RX_SIGLEV_RESET;
  delete best_rx_timer;
  best_rx_timer = 0;
  sql_rx_id = 0;
  
} /* Voter::reset */




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
void Voter::satSquelchOpen(bool is_open, SatRx *srx)
{
  Rx *rx = srx->rx;
  
  //cout << "Voter::satSquelchOpen(" << (is_open ? "TRUE" : "FALSE")
  //     << ", " << rx->name() << "): Signal Strength = "
  //     << rx->signalStrength() << "\n";

  if (is_open)
  {
    assert(active_rx == 0);
    
    if (best_rx == 0)
    {
      delete best_rx_timer;
      best_rx_timer = new Timer(voting_delay);
      best_rx_timer->expired.connect(slot(*this, &Voter::chooseBestRx));
    }
    
    if (rx->signalStrength() > best_rx_siglev)
    {
      best_rx_siglev = rx->signalStrength();
      best_rx = srx;
    }
    
    srx->clear();
  }
  else
  {
    if (active_rx != 0)
    {
      assert(best_rx == 0);
      assert(srx == active_rx);
      
      list<SatRx *>::iterator it;
      for (it=rxs.begin(); it!=rxs.end(); ++it)
      {
	if (*it != best_rx)
	{
	  (*it)->rx->mute(false);
	}
      }
      
      if (m_verbose)
      {
      	cout << name() << ": The squelch is CLOSED"
             << " (" << active_rx->rx->name() << "="
	     << active_rx->rx->signalStrength() << ")" << endl;
      }
      
      active_rx = 0;
      
      srx->stopOutput(true);
      sql_rx_id = srx->id;
      setSquelchState(false);
    }
    else if (srx == best_rx)
    {
      assert(active_rx == 0);
      
      best_rx = 0;
      best_rx_siglev = BEST_RX_SIGLEV_RESET;
      list<SatRx *>::iterator it;
      for (it=rxs.begin(); it!=rxs.end(); ++it)
      {
	if ((*it)->rx->squelchIsOpen() &&
	    ((*it)->rx->signalStrength() > best_rx_siglev))
	{
	  best_rx = *it;
	  best_rx_siglev = (*it)->rx->signalStrength();
	}
      }
    }
  }

} /* Voter::satSquelchOpen */


void Voter::chooseBestRx(Timer *t)
{
  //cout << "Voter::chooseBestRx\n";
  
  delete best_rx_timer;
  best_rx_timer = 0;
  
  if (best_rx != 0)
  {
    list<SatRx *>::iterator it;
    for (it=rxs.begin(); it!=rxs.end(); ++it)
    {
      if (*it != best_rx)
      {
	(*it)->rx->mute(true);
      }
    }
    
    if (m_verbose)
    {
      cout << name() << ": The squelch is OPEN"
           << " (" << best_rx->rx->name() << "=" << best_rx_siglev << ")"
	   << endl;
    }
    
    active_rx = best_rx;
    best_rx = 0;
    best_rx_siglev = BEST_RX_SIGLEV_RESET;

    sql_rx_id = active_rx->id;
    setSquelchState(true);
    active_rx->stopOutput(false);
  }
  
} /* Voter::chooseBestRx */



/*
 * This file has not been truncated
 */

