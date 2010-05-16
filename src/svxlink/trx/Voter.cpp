/**
@file	 Voter.cpp
@brief  This file contains a class that implement a receiver voter
@author Tobias Blomberg / SM0SVX
@date	 2005-04-18

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2010 Tobias Blomberg / SM0SVX

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
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <utility>
#include <list>
#include <sigc++/bind.h>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncTimer.h>
#include <AsyncAudioFifo.h>
#include <AsyncAudioSelector.h>
#include <AsyncAudioValve.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "Voter.h"



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

/**
 * @brief A class that represents a satellite receiver
 */
class SatRx : public AudioSource, public SigC::Object
{
  public:
    int id;
    Rx *rx;
    
    SatRx(int id, Rx *rx, int fifo_length_ms)
      : id(id), rx(rx), fifo(0), sql_open(false)
    {
      rx->dtmfDigitDetected.connect(slot(*this, &SatRx::onDtmfDigitDetected));
      rx->selcallSequenceDetected.connect(
	      slot(*this, &SatRx::onSelcallSequenceDetected));
      rx->squelchOpen.connect(slot(*this, &SatRx::rxSquelchOpen));
      AudioSource *prev_src = rx;

      if (fifo_length_ms > 0)
      {
        fifo = new AudioFifo(fifo_length_ms * INTERNAL_SAMPLE_RATE / 1000);
        fifo->setOverwrite(true);
        prev_src->registerSink(fifo);
        prev_src = fifo;
        valve.setBlockWhenClosed(true);
      }
      else
      {
        valve.setBlockWhenClosed(false);
      }
      
      valve.setOpen(false);
      prev_src->registerSink(&valve);
      
      AudioSource::setHandler(&valve);
    }
    
    ~SatRx(void)
    {
      delete fifo;
    }
    
    void stopOutput(bool do_stop)
    {
      valve.setOpen(!do_stop);
      if (!do_stop)
      {
      	DtmfBuf::iterator dit;
      	for (dit=dtmf_buf.begin(); dit!=dtmf_buf.end(); ++dit)
	{
	  dtmfDigitDetected((*dit).first, (*dit).second);
	}
	dtmf_buf.clear();
	
      	SelcallBuf::iterator sit;
      	for (sit=selcall_buf.begin(); sit!=selcall_buf.end(); ++sit)
	{
	  selcallSequenceDetected(*sit);
	}
	selcall_buf.clear();
      }
    }
    
    void mute(bool do_mute)
    {
      rx->mute(do_mute);
      if (do_mute)
      {
      	if (fifo != 0)
        {
          fifo->clear();
        }
	setSquelchOpen(false);
	dtmf_buf.clear();
	selcall_buf.clear();
      }
    }
    
    /*
    void clear(void)
    {
      fifo.clear();
      dtmf_buf.clear();
      selcall_buf.clear();
    }
    */
  
    Signal2<void, char, int>  	dtmfDigitDetected;
    Signal1<void, string>  	selcallSequenceDetected;
    Signal2<void, bool, SatRx*> squelchOpen;
  
  protected:
    virtual void allSamplesFlushed(void)
    {
      AudioSource::allSamplesFlushed();
      setSquelchOpen(rx->squelchIsOpen());
    }
  
  private:
    typedef list<pair<char, int> >  DtmfBuf;
    typedef list<string>  	     SelcallBuf;
    
    AudioFifo 	*fifo;
    AudioValve	valve;
    DtmfBuf   	dtmf_buf;
    SelcallBuf	selcall_buf;
    bool      	sql_open;
    
    void onDtmfDigitDetected(char digit, int duration)
    {
      if (!valve.isOpen())
      {
	dtmf_buf.push_back(pair<char, int>(digit, duration));
      }
      else
      {
      	dtmfDigitDetected(digit, duration);
      }
    }
    
    void onSelcallSequenceDetected(string sequence)
    {
      if (!valve.isOpen())
      {
	selcall_buf.push_back(sequence);
      }
      else
      {
      	selcallSequenceDetected(sequence);
      }
    }
    
    void rxSquelchOpen(bool is_open)
    {
      if (is_open)
      {
      	setSquelchOpen(true);
      }
      else
      {
      	if ((fifo == 0) || fifo->empty())
	{
	  setSquelchOpen(false);
	}
      }
    }
    
    void setSquelchOpen(bool is_open)
    {
      if (is_open != sql_open)
      {
      	sql_open = is_open;
	squelchOpen(is_open, this);
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
  : Rx(cfg, name), cfg(cfg), active_rx(0), is_muted(true), m_verbose(true),
    best_rx(0), best_rx_siglev(BEST_RX_SIGLEV_RESET), best_rx_timer(0),
    voting_delay(0), sql_rx_id(0), selector(0), buffer_length(0)
{
  Rx::setVerbose(false);
} /* Voter::Voter */


Voter::~Voter(void)
{
  delete selector;
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
  if (!Rx::initialize())
  {
    return false;
  }
  
  string receivers;
  if (!cfg.getValue(name(), "RECEIVERS", receivers))
  {
    cerr << "*** ERROR: Config variable " << name() << "/RECEIVERS not set\n";
    return false;
  }

  string value;
  if (cfg.getValue(name(), "VOTING_DELAY", value))
  {
    voting_delay = atoi(value.c_str());
    buffer_length = voting_delay;
  }
  
  if (cfg.getValue(name(), "BUFFER_LENGTH", value))
  {
    buffer_length = atoi(value.c_str());
  }

  selector = new AudioSelector;
  setHandler(selector);
  
  string::iterator start(receivers.begin());
  for (;;)
  {
    string::iterator comma = find(start, receivers.end(), ',');
    string rx_name(start, comma);
    if (!rx_name.empty())
    {
      cout << "Adding receiver to Voter: " << rx_name << endl;
      Rx *rx = RxFactory::createNamedRx(cfg, rx_name);
      if (rx == 0)
      {
      	// FIXME: Cleanup
      	return false;
      }
      SatRx *srx = new SatRx(rxs.size() + 1, rx, buffer_length);
      srx->squelchOpen.connect(slot(*this, &Voter::satSquelchOpen));
      srx->dtmfDigitDetected.connect(dtmfDigitDetected.slot());
      srx->selcallSequenceDetected.connect(selcallSequenceDetected.slot());
      if ((srx == 0) || !rx->initialize())
      {
      	// FIXME: Cleanup
      	return false;
      }
      srx->mute(true);
      rx->setVerbose(false);
      rx->toneDetected.connect(toneDetected.slot());
      selector->addSource(srx);
      selector->enableAutoSelect(srx, 0);
      
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
    (*it)->mute(do_mute);
  }
    
  is_muted = do_mute;
  
} /* Voter::mute */


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


float Voter::signalStrength(void) const
{
  if (active_rx != 0)
  {
    return active_rx->rx->signalStrength();
  }
  
  return BEST_RX_SIGLEV_RESET;
  
} /* Voter::signalStrength */


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



/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/

void Voter::satSquelchOpen(bool is_open, SatRx *srx)
{
  Rx *rx = srx->rx;
  
  //cout << name() << "::satSquelchOpen(" << (is_open ? "TRUE" : "FALSE")
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
    
    //srx->clear();
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
	if (*it != active_rx)
	{
	  (*it)->mute(false);
	}
      }
      
      if (m_verbose)
      {
      	cout << name() << ": The squelch is CLOSED"
             << " (" << active_rx->rx->name() << "="
	     << active_rx->rx->signalStrength() << ")" << endl;
      }
      
      srx->stopOutput(true);
      sql_rx_id = srx->id;
      setSquelchState(false);
      
      active_rx = 0;      
    }
    else if (srx == best_rx)
    {
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
      if (best_rx == 0)
      {
      	delete best_rx_timer;
	best_rx_timer = 0;
      }
    }
  }

} /* Voter::satSquelchOpen */


void Voter::chooseBestRx(Timer *t)
{
  //cout << name() << "::chooseBestRx\n";
  
  delete best_rx_timer;
  best_rx_timer = 0;
  
  if (best_rx != 0)
  {
    list<SatRx *>::iterator it;
    for (it=rxs.begin(); it!=rxs.end(); ++it)
    {
      if (*it != best_rx)
      {
	(*it)->mute(true);
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

