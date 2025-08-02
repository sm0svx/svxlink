/**
@File	Voter.cpp
@brief  This file contains a class that implement a receiver voter
@author Tobias Blomberg / SM0SVX
@date	2005-04-18

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2021 Tobias Blomberg / SM0SVX

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
#include <iomanip>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <utility>
#include <list>
#include <sigc++/bind.h>
#include <sys/time.h>
#include <json/json.h>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncApplication.h>
#include <AsyncTimer.h>
#include <AsyncAudioFifo.h>
#include <AsyncAudioSelector.h>
#include <AsyncAudioValve.h>
#include <AsyncPty.h>
#include <AsyncPtyStreamBuf.h>


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
using namespace sigc;
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

/**
 * @brief A class that represents a satellite receiver
 * 
 * The main purpose of this class is to handle the voter delay buffer.
 * During a voter delay, the content (audio, dtmf, selcall etc) received for
 * each receiver should be buffered until we know which receiver is going to
 * be chosen. When a receiver has been chosen, it's content is released to
 * its "subscribers".
 * When the receiver close its squelch, the squelch signal is delayed until
 * all audio has been flushed.
 */
class Voter::SatRx : public AudioSource, public sigc::trackable
{
  public:
    SatRx(Config &cfg, const string &rx_name, int id, int fifo_length_ms)
      : rx_id(id), rx(0), fifo(0), sql_open(false), enabled(true),
        mute_state(Rx::MUTE_ALL), sql_open_delay(0)
    {
      rx = RxFactory::createNamedRx(cfg, rx_name);
      if (rx != 0)
      {
        mute_state = rx->muteState();
        rx->dtmfDigitDetected.connect(
                sigc::mem_fun(*this, &SatRx::onDtmfDigitDetected));
        rx->selcallSequenceDetected.connect(
                sigc::mem_fun(*this, &SatRx::onSelcallSequenceDetected));
        rx->squelchOpen.connect(
                sigc::mem_fun(*this, &SatRx::rxSquelchOpen));
        rx->signalLevelUpdated.connect(
                sigc::mem_fun(*this, &SatRx::rxSignalLevelUpdated));
        rx->toneDetected.connect(toneDetected.make_slot());

        // FIXME: We should take care of publishStateEvent

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
    }
    
    ~SatRx(void)
    {
      delete fifo;
      rx->reset();
      delete rx;
    }
    
    bool initialize(void)
    {
      if ((rx == 0) || !rx->initialize())
      {
      	return false;
      }
      rx->setVerbose(false);
      return true;
    }

    bool setEnabled(Rx::MuteState disabled_mute_state)
    {
      if (!enabled && (disabled_mute_state == Rx::MUTE_NONE))
      {
        enabled = true;
        setMuteStateP(mute_state);
        return true;
      }
      else if (disabled_mute_state != rx->muteState())
      {
        setMuteStateP(disabled_mute_state);
        enabled = false;
        return true;
      }
      return false;
    }

    bool isEnabled(void) const { return enabled; }

    const std::string& name(void) const { return rx->name(); }
    
    bool addToneDetector(float fq, int bw, float thresh, int required_duration)
    {
      return rx->addToneDetector(fq, bw, thresh, required_duration);
    }
    
    float signalStrength(void) const { return rx->signalStrength(); }

    const std::string& squelchActivityInfo(void) const
    {
      return rx->squelchActivityInfo();
    }

    void setMuteState(Rx::MuteState new_mute_state)
    {
      mute_state = new_mute_state;
      if (enabled)
      {
        setMuteStateP(new_mute_state);
      }
    }
    
    void reset(void)
    {
      rx->reset();
      mute_state = rx->muteState();
    }
    
    bool squelchIsOpen(void) const { return sql_open; }
    
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
    
    char id(void) const { return rx->sqlRxId(); }

    void setSqlOpenDelay(unsigned new_sql_open_delay)
    {
      sql_open_delay = new_sql_open_delay;
    }
    unsigned sqlOpenDelay(void) const { return sql_open_delay; }

    sigc::signal<void(char, int)>     dtmfDigitDetected;
    sigc::signal<void(string)>        selcallSequenceDetected;
    sigc::signal<void(bool, SatRx*)>  squelchOpen;
    sigc::signal<void(float, SatRx*)> signalLevelUpdated;
    sigc::signal<void(float)>         toneDetected;

  protected:
    virtual void allSamplesFlushed(void)
    {
      AudioSource::allSamplesFlushed();
      setSquelchOpen(rx->squelchIsOpen());
    }
  
  
  private:
    typedef list<pair<char, int> >	DtmfBuf;
    typedef list<string>		SelcallBuf;
    
    int		  rx_id;
    Rx		  *rx;
    AudioFifo 	  *fifo;
    AudioValve	  valve;
    DtmfBuf   	  dtmf_buf;
    SelcallBuf	  selcall_buf;
    bool      	  sql_open;
    bool          enabled;
    Rx::MuteState mute_state;
    unsigned      sql_open_delay;
    
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
    
    void rxSignalLevelUpdated(float siglev)
    {
      if (sql_open)
      {
	signalLevelUpdated(siglev, this);
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

    void setMuteStateP(Rx::MuteState new_mute_state)
    {
      rx->setMuteState(new_mute_state);
      if (new_mute_state != Rx::MUTE_NONE)
      {
        if (fifo != 0)
        {
          fifo->clear();
        }
        dtmf_buf.clear();
        selcall_buf.clear();
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
  : Rx(cfg, name), cfg(cfg), m_verbose(true), selector(0),
    sm(Macho::State<Top>(this)), is_processing_event(false), command_pty(0),
    m_print_sat_squelch(false)
{
} /* Voter::Voter */


Voter::~Voter(void)
{
  delete command_pty;
  command_pty = 0;
  delete selector;
  selector = 0;
  
    // Mute all receivers before deleting them so that we do not get any
    // unexpected updates during deletion
  list<SatRx *>::iterator it;
  for (it=rxs.begin(); it!=rxs.end(); ++it)
  {
    (*it)->setMuteState(Rx::MUTE_ALL);
  }
  for (it=rxs.begin(); it!=rxs.end(); ++it)
  {
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

  string pty_path;
  if(cfg.getValue(name(), "COMMAND_PTY", pty_path))
  {
    command_pty = new Pty(pty_path);
    if (!command_pty->open())
    {
      cerr << "*** ERROR: Could not open voter command PTY "
      << pty_path << " as specified in configuration variable "
      << name() << "/" << "COMMAND_PTY" << endl;
      return false;
    }
    command_pty->dataReceived.connect(
        sigc::mem_fun(*this, &Voter::onCommandPtyInput));
  }

  string receivers;
  if (!cfg.getValue(name(), "RECEIVERS", receivers))
  {
    cerr << "*** ERROR: Config variable " << name() << "/RECEIVERS not set\n";
    return false;
  }

  unsigned voting_delay = DEFAULT_VOTING_DEALAY;
  cfg.getValue(name(), "VOTING_DELAY", voting_delay);
  if (voting_delay > MAX_VOTING_DELAY)
  {
    cerr << "*** ERROR: Config variable " << name() << "/VOTING_DELAY out "
            "of range (" << voting_delay << "). Valid range is 0 to "
	 << MAX_VOTING_DELAY << ".\n";
    return false;
  }
  sm->setVotingDelay(voting_delay);
  
  unsigned buffer_length = voting_delay;
  cfg.getValue(name(), "BUFFER_LENGTH", buffer_length);
  if (buffer_length > MAX_BUFFER_LENGTH)
  {
    cerr << "*** ERROR: Config variable " << name() << "/BUFFER_LENGTH out "
            "of range (" << buffer_length << "). Valid range is 0 to "
	 << MAX_BUFFER_LENGTH << ".\n";
    return false;
  }
  
  float hysteresis = 100.0f * (DEFAULT_HYSTERESIS - 1.0f);
  cfg.getValue(name(), "HYSTERESIS", hysteresis);
  if ((hysteresis < 0.0f)
      || (hysteresis > 100.0f * (MAX_HYSTERESIS - 1.0f)))
  {
    cerr << "*** ERROR: Config variable " << name() << "/HYSTERESIS out "
            "of range (" << hysteresis << "). Valid range is 0 to "
	 << (100.0f * (MAX_HYSTERESIS - 1.0f)) << ".\n";
    return false;
  }
  sm->setHysteresis(hysteresis / 100.0f + 1.0f);

  unsigned sql_close_revote_delay = DEFAULT_SQL_CLOSE_REVOTE_DELAY;
  cfg.getValue(name(), "SQL_CLOSE_REVOTE_DELAY", sql_close_revote_delay);
  if (sql_close_revote_delay > MAX_SQL_CLOSE_REVOTE_DELAY)
  {
    cerr << "*** ERROR: Config variable " << name()
         << "/SQL_CLOSE_REVOTE_DELAY out of range ("
	 << sql_close_revote_delay << "). Valid range is 0 to "
	 << MAX_SQL_CLOSE_REVOTE_DELAY << ".\n";
    return false;
  }
  sm->setSqlCloseRevoteDelay(sql_close_revote_delay);
  
  unsigned revote_interval = DEFAULT_REVOTE_INTERVAL;
  cfg.getValue(name(), "REVOTE_INTERVAL", revote_interval);
  if (revote_interval != 0)
  {
    if ((revote_interval < MIN_REVOTE_INTERVAL) ||
	(revote_interval > MAX_REVOTE_INTERVAL))
    {
      cerr << "*** ERROR: Config variable " << name() << "/REVOTE_INTERVAL out "
	      "of range (" << revote_interval << "). Valid range is "
	  << MIN_REVOTE_INTERVAL << " to "
	  << MAX_REVOTE_INTERVAL << ".\n";
      return false;
    }
  }
  sm->setRevoteInterval(revote_interval);

  unsigned rx_switch_delay = DEFAULT_RX_SWITCH_DELAY;
  cfg.getValue(name(), "RX_SWITCH_DELAY", rx_switch_delay);
  if (rx_switch_delay > MAX_RX_SWITCH_DELAY)
  {
    cerr << "*** ERROR: Config variable " << name()
         << "/RX_SWITCH_DELAY out of range ("
	 << rx_switch_delay << "). Valid range is 0 to "
	 << MAX_RX_SWITCH_DELAY << ".\n";
    return false;
  }
  sm->setRxSwitchDelay(rx_switch_delay);

  cfg.getValue(name(), "VERBOSE", m_print_sat_squelch);

  selector = new AudioSelector;
  setHandler(selector);
  
  string::iterator start(receivers.begin());
  for (;;)
  {
    string::iterator comma = find(start, receivers.end(), ',');
    string::iterator name_end(comma);
    string::iterator colon = find(start, comma, ':');
    unsigned sql_open_delay = 0;
    if (colon != comma)
    {
      name_end = colon;
      if (++colon != comma)
      {
        string sql_open_delay_str(colon, comma);
        istringstream is(sql_open_delay_str);
        is >> sql_open_delay;
      }
    }
    string rx_name(start, name_end);
    if (!rx_name.empty())
    {
      cout << "\tAdding receiver: " << rx_name << endl;
      SatRx *srx = new SatRx(cfg, rx_name, rxs.size() + 1, buffer_length);
      srx->setSqlOpenDelay(sql_open_delay);
      srx->squelchOpen.connect(mem_fun(*this, &Voter::satSquelchOpen));
      srx->signalLevelUpdated.connect(
	      mem_fun(*this, &Voter::satSignalLevelUpdated));
      srx->dtmfDigitDetected.connect(dtmfDigitDetected.make_slot());
      srx->selcallSequenceDetected.connect(selcallSequenceDetected.make_slot());
      
      if ((srx == 0) || !srx->initialize())
      {
      	return false;
      }
      srx->setMuteState(MUTE_ALL);
      srx->toneDetected.connect(toneDetected.make_slot());
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


void Voter::setMuteState(MuteState new_mute_state)
{
  //cout << "Voter::mute: do_mute=" << (do_mute ? "TRUE" : "FALSE") << endl;
  assert(!is_processing_event);
  Rx::setMuteState(new_mute_state);
  dispatchEvent(Macho::Event(&Top::setMuteState, new_mute_state));
} /* Voter::setMuteState */


bool Voter::addToneDetector(float fq, int bw, float thresh,
      	      	      	    int required_duration)
{
  bool success = true;
  list<SatRx *>::iterator it;
  for (it=rxs.begin(); it!=rxs.end(); ++it)
  {
    success &= (*it)->addToneDetector(fq, bw, thresh, required_duration);
  }
  
  return success;
  
} /* Voter::addToneDetector */


float Voter::signalStrength(void) const
{
    // Const cast needed since we cannot declare the signalStrength
    // method const due to how the state machine is implemented.
  return const_cast<Macho::Machine<Top>&>(sm)->signalStrength();
} /* Voter::signalStrength */


char Voter::sqlRxId(void) const
{
  return const_cast<Macho::Machine<Top>&>(sm)->sqlRxId();
} /* Voter::sqlRxId */


void Voter::reset(void)
{
  assert(!is_processing_event);
  dispatchEvent(Macho::Event(&Top::reset));
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

void Voter::dispatchEvent(Macho::IEvent<Top> *event)
{
  if (!is_processing_event)
  {
    is_processing_event = true;
    sm.dispatch(event);
    EventQueue::iterator it;
    for (it=event_queue.begin(); it!=event_queue.end(); ++it)
    {
      sm.dispatch(*it);
    }
    event_queue.clear();
    is_processing_event = false;
  }
  else
  {
    event_queue.push_back(event);
  }
} /* Voter::dispatchEvent */


void Voter::satSquelchOpen(bool is_open, SatRx *srx)
{
  if (m_print_sat_squelch || !srx->isEnabled())
  {
    std::cout << name() << "[" << srx->name() << "]"
         << ": The squelch is " << (is_open ? "OPEN" : "CLOSED")
         << " (siglev="
         << static_cast<int>(std::roundf(srx->signalStrength()))
         << ")";
    if (!srx->isEnabled())
    {
      std::cout << " [MUTED]";
    }
    std::cout << std::endl;
  }

  if (srx->isEnabled())
  {
    dispatchEvent(Macho::Event(&Top::satSquelchOpen, srx, is_open));
  }

  Async::Application::app().runTask([&]{ publishSquelchState(); });
} /* Voter::satSquelchOpen */


void Voter::satSignalLevelUpdated(float siglev, SatRx *srx)
{
  if (srx->isEnabled())
  {
    dispatchEvent(Macho::Event(&Top::satSignalLevelUpdated, srx, siglev));
  }
} /* Voter::satSignalLevelUpdated */


void Voter::muteAllBut(SatRx *srx, Rx::MuteState mute_state)
{
  list<SatRx *>::iterator it;
  for (it=rxs.begin(); it!=rxs.end(); ++it)
  {
    (*it)->setMuteState(*it == srx ? MUTE_NONE : mute_state);
  }
} /* Voter::muteAllBut */


void Voter::unmuteAll(void)
{
  list<SatRx *>::iterator it;
  for (it=rxs.begin(); it!=rxs.end(); ++it)
  {
    (*it)->setMuteState(MUTE_NONE);
  }
} /* Voter::unmuteAll */


void Voter::resetAll(void)
{
  list<SatRx *>::iterator it;
  for (it=rxs.begin(); it!=rxs.end(); ++it)
  {
    (*it)->reset();
  }
} /* Voter::resetAll */


void Voter::publishSquelchState(void)
{
  Json::Value event(Json::arrayValue);
  for (const auto& srx : rxs)
  {
    float siglev = srx->signalStrength();
    bool sql_is_open = srx->squelchIsOpen();
    bool is_enabled = srx->isEnabled();
    bool is_active = sql_is_open && (srx == sm->activeSrx());
    Json::Value rx(Json::objectValue);
    rx["name"] = srx->name();
    char rx_id = srx->id();
    rx["id"] = std::string(&rx_id, &rx_id+1);
    rx["enabled"] = is_enabled;
    rx["sql_open"] = sql_is_open;
    rx["active"] = is_active;
    rx["siglev"] = static_cast<int>(siglev);
    event.append(rx);
  }
  Json::StreamWriterBuilder builder;
  builder["commentStyle"] = "None";
  builder["indentation"] = ""; //The JSON document is written on a single line
  Json::StreamWriter* writer = builder.newStreamWriter();
  stringstream os;
  writer->write(event, &os);
  delete writer;
  publishStateEvent("Voter:sql_state", os.str());
} /* Voter::publishSquelchState */


Voter::SatRx *Voter::findBestRx(void) const
{
  float best_rx_siglev = 0.0f;
  SatRx *best_rx = 0;
  list<SatRx *>::const_iterator it;
  for (it=rxs.begin(); it!=rxs.end(); ++it)
  {
    if ((*it)->isEnabled() && (*it)->squelchIsOpen() &&
	((best_rx == 0) || ((*it)->signalStrength() > best_rx_siglev)))
    {
      best_rx = *it;
      best_rx_siglev = (*it)->signalStrength();
    }
  }
  
  return best_rx;
  
} /* Voter::findBestRx */



/****************************************************************************
 *
 * Top state event handlers
 *
 ****************************************************************************/

void Voter::Top::init(Voter *voter)
{
  box().voter = voter;
  box().event_timer.expired.connect(mem_fun(*this, &Top::eventTimerExpired));
  setState<Muted>();
} /* Voter::Top::init */


void Voter::Top::exit(void)
{
} /* Voter::Top::exit */


void Voter::Top::reset(void)
{
  voter().resetAll();
  setState<Muted>();
} /* Voter::Top::reset */


void Voter::Top::setMuteState(Rx::MuteState new_mute_state)
{
  if (new_mute_state == muteState())
  {
    return;
  }

  switch (new_mute_state)
  {
    case MUTE_NONE:
      voter().unmuteAll();
      break;

    case MUTE_CONTENT:
      voter().muteAll(MUTE_CONTENT);
      break;

    case MUTE_ALL:
      setState<Muted>();
      break;
  }
  box().mute_state = new_mute_state;
} /* Voter::Top::setMuteState */


void Voter::Top::satSquelchOpen(SatRx *srx, bool is_open)
{
  assert(srx != 0);
  
  if (bestSrx() == 0)
  {
    assert(is_open);
    box().best_srx = srx;
  }
  else if (srx == bestSrx())
  {
    if (!is_open)
    {
      box().best_srx = voter().findBestRx();
    }
  }
  else
  {
    if (is_open &&
        (srx->signalStrength() > bestSrx()->signalStrength()))
    {
      box().best_srx = srx;
    }
  }
} /* Voter::Top::satSquelchOpen */


void Voter::Top::satSignalLevelUpdated(SatRx *srx, float siglev)
{
  assert(srx != 0);
  assert(bestSrx() != 0);
  assert(srx->squelchIsOpen());
  
  if (!bestSrx()->squelchIsOpen() ||
      (siglev > bestSrx()->signalStrength()))
  {
    box().best_srx = srx;
  }

  if (srx == activeSrx())
  {
    runTask(sigc::bind(voter().signalLevelUpdated.make_slot(), siglev));
  }
} /* Voter::Top::satSignalLevelUpdated */


void Voter::Top::runTask(sigc::slot<void()> task)
{
  Async::Application::app().runTask(task);
} /* Voter::Top::runTask */


void Voter::Top::startTimer(unsigned time_ms)
{
  box().event_timer.setTimeout(time_ms);
  box().event_timer.setEnable(true);
} /* Voter::Top::startTimer */


void Voter::Top::stopTimer(void)
{
  box().event_timer.setEnable(false);
} /* Voter::Top::stopTimer */


void Voter::Top::eventTimerExpired(Timer *t)
{ 
  voter().dispatchEvent(Macho::Event(&Top::timerExpired));
} /* Voter::Top::eventTimerExpired */



/****************************************************************************
 *
 * Muted state event handlers
 *
 ****************************************************************************/

void Voter::Muted::entry(void)
{
  //cout << "### Muted::entry\n";
  voter().muteAll(MUTE_ALL);
} /* Voter::Muted::entry */


void Voter::Muted::setMuteState(Rx::MuteState new_mute_state)
{
  if ((new_mute_state == Rx::MUTE_NONE)
      || (new_mute_state == Rx::MUTE_CONTENT))
  {
    doUnmute();
  }
  TOP::box().mute_state = new_mute_state;
} /* Voter::Muted::setMuteState */


void Voter::Muted::doUnmute(void)
{
  if (bestSrx() != 0)
  {
    assert(bestSrx()->squelchIsOpen());
    setState<ActiveRxSelected>(bestSrx());
  }
  else
  {
    setState<Idle>();
  }
} /* Voter::Muted::doUnmute */



/****************************************************************************
 *
 * Idle state event handlers
 *
 ****************************************************************************/

void Voter::Idle::entry(void)
{
  //cout << "### Idle::entry\n";
  if (muteState() == Rx::MUTE_NONE)
  {
    voter().unmuteAll();
  }
} /* Voter::Idle::entry */


void Voter::Idle::satSquelchOpen(SatRx *srx, bool is_open)
{
  SUPER::satSquelchOpen(srx, is_open);
  if (is_open)
  {
    if (srx->signalStrength() * hysteresis() > 100.0f)
    {
      setState<ActiveRxSelected>(bestSrx());
    }
    else
    {
      setState<VotingDelay>(srx);
    }
  }
} /* Voter::Idle::satSquelchOpen */



/****************************************************************************
 *
 * VotingDelay state event handlers
 *
 ****************************************************************************/

void Voter::VotingDelay::entry(void)
{
  //cout << "### VotingDelay::entry\n";
  //startTimer(votingDelay());
} /* Voter::VotingDelay::entry */


void Voter::VotingDelay::init(SatRx *srx)
{
  //cout << "### VotingDelay::init\n";
  startTimer(max(votingDelay() - srx->sqlOpenDelay(), 0U));
} /* Voter::VotingDelay::init */


void Voter::VotingDelay::exit(void)
{
  //cout << "### VotingDelay::exit\n";
  stopTimer();
} /* Voter::VotingDelay::exit */


void Voter::VotingDelay::satSquelchOpen(SatRx *srx, bool is_open)
{
  SUPER::satSquelchOpen(srx, is_open);
  if (is_open)
  {
    if (srx->signalStrength() * hysteresis() > 100.0f)
    {
      setState<ActiveRxSelected>(bestSrx());
    }
  }
  else
  {
    if (bestSrx() == 0)
    {
      setState<Idle>();
    }
  }
} /* Voter::VotingDelay::satSquelchOpen */


void Voter::VotingDelay::timerExpired(void)
{
  assert(bestSrx() != 0);
  assert(bestSrx()->squelchIsOpen());
  setState<ActiveRxSelected>(bestSrx());
} /* Voter::VotingDelay::timerExpired */



/****************************************************************************
 *
 * ActiveRxSelected state event handlers
 *
 ****************************************************************************/

void Voter::ActiveRxSelected::init(SatRx *srx)
{
  assert(srx != 0);
  box().active_srx = srx;
  if (muteState() == MUTE_CONTENT)
  {
    voter().muteAll(MUTE_CONTENT);
  }
  else
  {
    voter().muteAllBut(srx, MUTE_CONTENT);
  }
  setState<SquelchOpen>();
} /* Voter::ActiveRxSelected::init */


void Voter::ActiveRxSelected::exit(void)
{
  runTask(sigc::bind(sigc::mem_fun(activeSrx(), &SatRx::stopOutput), true));
} /* Voter::ActiveRxSelected::exit */


void Voter::ActiveRxSelected::setMuteState(Rx::MuteState new_mute_state)
{
  if (new_mute_state != Rx::MUTE_NONE)
  {
    SUPER::setMuteState(new_mute_state);
    return;
  }
  activeSrx()->setMuteState(MUTE_NONE);
  TOP::box().mute_state = Rx::MUTE_NONE;
} /* Voter::ActiveRxSelected::setMuteState */


char Voter::ActiveRxSelected::sqlRxId(void)
{
  return box().active_srx->id();
} /* Voter::ActiveRxSelected::sqlRxId */


void Voter::ActiveRxSelected::changeActiveSrx(SatRx *srx)
{
  voter().selector->selectSource(srx);
  activeSrx()->setMuteState(MUTE_CONTENT);
  box().active_srx = srx;
  if (muteState() == Rx::MUTE_NONE)
  {
    activeSrx()->setMuteState(MUTE_NONE);
  }
} /* Voter::ActiveRxSelected::changeActiveSrx */



/****************************************************************************
 *
 * SquelchOpen state event handlers
 *
 ****************************************************************************/

void Voter::SquelchOpen::entry(void)
{
  std::ostringstream ss;
  if (voter().m_verbose)
  {
    const SatRx *srx = activeSrx();
    ss << srx->name() << "[" << srx->squelchActivityInfo() << "]="
       << static_cast<int>(std::roundf(srx->signalStrength()));
  }

  runTask(bind(mem_fun(voter(), &Voter::setSquelchState), true, ss.str()));
  runTask(bind(mem_fun(activeSrx(), &SatRx::stopOutput), false));
  //runTask(mem_fun(voter(), &Voter::publishSquelchState));
} /* Voter::SquelchOpen::entry */


void Voter::SquelchOpen::init(void)
{
  setState<Receiving>();
} /* Voter::SquelchOpen::init */


void Voter::SquelchOpen::exit(void)
{
  std::ostringstream ss;
  if (voter().m_verbose)
  {
    const SatRx *srx = activeSrx();
    ss << srx->name() << "[" << srx->squelchActivityInfo() << "]="
       << static_cast<int>(std::roundf(srx->signalStrength()));
  }

  runTask(bind(mem_fun(voter(), &Voter::setSquelchState), false, ss.str()));
  //runTask(mem_fun(voter(), &Voter::publishSquelchState));
} /* Voter::SquelchOpen::exit */


void Voter::SquelchOpen::satSquelchOpen(SatRx *srx, bool is_open)
{
  SUPER::satSquelchOpen(srx, is_open);
  if (!is_open && (srx == activeSrx()))
  {
    setState<SqlCloseWait>();
  }
} /* Voter::SquelchOpen::satSquelchOpen */


float Voter::SquelchOpen::signalStrength(void)
{
  return activeSrx()->signalStrength();
} /* Voter::SquelchOpen::signalStrength */


void Voter::SquelchOpen::changeActiveSrx(SatRx *srx)
{
  runTask(bind(mem_fun(activeSrx(), &SatRx::stopOutput), true));
  SUPER::changeActiveSrx(srx);
  runTask(bind(mem_fun(activeSrx(), &SatRx::stopOutput), false));  
} /* Voter::SquelchOpen::changeActiveSrx */



/****************************************************************************
 *
 * SqlCloseWait state event handlers
 *
 ****************************************************************************/

void Voter::SqlCloseWait::entry(void)
{
  //cout << "### SqlCloseWait::entry\n";
  startTimer(sqlCloseRevoteDelay());
} /* Voter::SqlCloseWait::entry */


void Voter::SqlCloseWait::exit(void)
{
  //cout << "### SqlCloseWait::exit\n";
  stopTimer();
} /* Voter::SqlCloseWait::exit */


void Voter::SqlCloseWait::satSquelchOpen(SatRx *srx, bool is_open)
{
  SUPER::satSquelchOpen(srx, is_open);
  if (is_open && (srx == activeSrx()))
  {
    setState<SquelchOpen>();
  }
} /* Voter::SqlCloseWait::satSquelchOpen */


void Voter::SqlCloseWait::timerExpired(void)
{
  if (bestSrx() != 0)
  {
    changeActiveSrx(bestSrx());
    setState<SquelchOpen>();
  }
  else
  {
    setState<Idle>();
  }
} /* Voter::SqlCloseWait::timerExpired */



/****************************************************************************
 *
 * Receiving state event handlers
 *
 ****************************************************************************/

void Voter::Receiving::entry(void)
{
  //cout << "### Receiving::entry\n";
  if (revoteInterval() >= MIN_REVOTE_INTERVAL)
  {
    startTimer(revoteInterval());
  }
} /* Voter::Receiving::entry */


void Voter::Receiving::exit(void)
{
  //cout << "### Receiving::exit\n";
  stopTimer();
} /* Voter::Receiving::exit */


void Voter::Receiving::timerExpired(void)
{
  assert(activeSrx() != 0);

  voter().publishSquelchState();

  if ((bestSrx() != 0) && (bestSrx() != activeSrx()))
  {
    float best_srx_siglev = bestSrx()->signalStrength();
    float active_srx_siglev = activeSrx()->signalStrength();
    if (best_srx_siglev > active_srx_siglev*hysteresis())
    {
      setState<SwitchActiveRx>(bestSrx());
      return;
    }
  }
  if (revoteInterval() >= MIN_REVOTE_INTERVAL)
  {
    startTimer(revoteInterval());
  }
} /* Voter::Receiving::timerExpired */



/****************************************************************************
 *
 * SwitchActiveRx state event handlers
 *
 ****************************************************************************/

void Voter::SwitchActiveRx::entry(void)
{
  //cout << "### SwitchActiveRx::entry\n";
  startTimer(rxSwitchDelay());
} /* Voter::SwitchActiveRx::entry */


void Voter::SwitchActiveRx::init(SatRx *srx)
{
  box().switch_to_srx = srx;
  if (muteState() == Rx::MUTE_NONE)
  {
    srx->setMuteState(MUTE_NONE);
  }
} /* Voter::SwitchActiveRx::init */


void Voter::SwitchActiveRx::exit(void)
{
  //cout << "### SwitchActiveRx::exit\n";
  if (box().switch_to_srx != 0)
  {
    box().switch_to_srx->setMuteState(MUTE_CONTENT);
  }

  stopTimer();
} /* Voter::SwitchActiveRx::exit */


void Voter::SwitchActiveRx::setMuteState(Rx::MuteState new_mute_state)
{
  if (new_mute_state != Rx::MUTE_NONE)
  {
    SUPER::setMuteState(new_mute_state);
    return;
  }
  activeSrx()->setMuteState(MUTE_NONE);
  box().switch_to_srx->setMuteState(MUTE_NONE);
  TOP::box().mute_state = Rx::MUTE_NONE;
} /* Voter::SwitchActiveRx::setMuteState */


void Voter::SwitchActiveRx::timerExpired(void)
{
  SatRx *switch_to_srx = box().switch_to_srx;
  
  assert(activeSrx() != 0);
  assert(switch_to_srx != 0);
  assert (switch_to_srx != activeSrx());
  
  float switch_to_srx_siglev = switch_to_srx->signalStrength();
  float active_srx_siglev = activeSrx()->signalStrength();
  if (switch_to_srx->squelchIsOpen() &&
      (switch_to_srx_siglev > active_srx_siglev*hysteresis()))
  {
    if (voter().m_verbose)
    {
      cout << voter().name() << ": Switching from \"" << activeSrx()->name()
	   << "\" (" << active_srx_siglev << ") to \""
	   << switch_to_srx->name()
	   << "\" (" << switch_to_srx_siglev << ")\n";
    }
    
    changeActiveSrx(switch_to_srx);
    box().switch_to_srx = 0;
  }
  setState<Receiving>();
} /* Voter::SwitchActiveRx::timerExpired */


void Voter::onCommandPtyInput(const void *buf, size_t count)
{
  const char *buffer = reinterpret_cast<const char*>(buf);
  for (size_t i=0; i<count; ++i)
  {
    char ch = buffer[i];
    switch (ch)
    {
      case '\n':  // Execute command on NL
        handlePtyCommand(command_buf);
        command_buf.clear();
        break;

      case '\r':  // Ignore CR
        break;

      default:    // Append character to command buffer
        if (command_buf.size() >= 256)  // Prevent cmd buffer growing too big
        {
          command_buf.clear();
        }
        command_buf += ch;
        break;
    }
  }
} /* Voter::onCommandPtyInput */


void Voter::handlePtyCommand(const std::string &full_command)
{
  istringstream is(full_command);
  string command;
  if (!(is >> command))
  {
    return;
  }

  std::string rx_name;
  if (!(is >> rx_name))
  {
    std::cerr << "*** WARNING: Malformed voter PTY command "
                 "(missing/illegal rx name): \"" << full_command << "\""
              << std::endl;
    return;
  }

  if (command == "ENABLE") // Enable receiver
  {
    setRxEnabled(rx_name, Rx::MUTE_NONE);
  }
  else if (command == "MUTE") // Mute receiver audio
  {
    setRxEnabled(rx_name, Rx::MUTE_CONTENT);
  }
  else if (command == "DISABLE") // Disable receiver
  {
    setRxEnabled(rx_name, Rx::MUTE_ALL);
  }
  else
  {
    cerr << "*** WARNING: Unknown voter PTY command received: \""
         << full_command << "\"" << endl;
  }
} /* Voter::handlePtyCommand */


void Voter::setRxEnabled(const std::string &rx_name,
                         Rx::MuteState disabled_mute_state)
{
  const char* mute_str_map[] = {"Enabling", "Muting", "Disabling"};
  auto mute_str = mute_str_map[disabled_mute_state];
  bool do_enable = (disabled_mute_state == Rx::MUTE_NONE);
  for (auto& srx : rxs)
  {
    if (srx->name() == rx_name)
    {
      if (srx->setEnabled(disabled_mute_state))
      {
        std::cout << name() << ": " << mute_str << " receiver " << srx->name()
                  << std::endl;
        if (srx->squelchIsOpen())
        {
          dispatchEvent(Macho::Event(&Top::satSquelchOpen, srx, do_enable));
        }
        publishSquelchState();
      }
      return;
    }
  }
  std::cerr << "*** WARNING: " << mute_str << " receiver failed: "
          " non-existent receiver \"" << rx_name << "\"" << std::endl;
} /* Voter::setRxEnabled */


/*
 * This file has not been truncated
 */
