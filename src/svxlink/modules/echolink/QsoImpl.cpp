/**
@file	 QsoImpl.cpp
@brief   Data for one EchoLink Qso.
@author  Tobias Blomberg / SM0SVX
@date	 2004-06-02

This file contains a class that implementes the things needed for one
EchoLink Qso.

\verbatim
A module (plugin) for the multi purpose tranciever frontend system.
Copyright (C) 2004-2025 Tobias Blomberg / SM0SVX

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

#include <cassert>
#include <cstdlib>
#include <sigc++/bind.h>
#include <sstream>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncConfig.h>
#include <AsyncAudioPacer.h>
#include <AsyncAudioSelector.h>
#include <AsyncAudioPassthrough.h>
#include <AsyncAudioFifo.h>
#include <AsyncAudioDecimator.h>
#include <AsyncAudioInterpolator.h>
#include <AsyncAudioDebugger.h>

#include <MsgHandler.h>
#include <EventHandler.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "ModuleEchoLink.h"
#include "QsoImpl.h"
#include "multirate_filter_coeff.h"


/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/

using namespace std;
using namespace Async;
using namespace EchoLink;
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


QsoImpl::QsoImpl(const StationData &station, ModuleEchoLink *module)
  : m_qso(station.ip()), module(module), event_handler(0), msg_handler(0),
    output_sel(0), init_ok(false), reject_qso(false), last_message(""),
    last_info_msg(""), idle_timer(0), disc_when_done(false), idle_timer_cnt(0),
    idle_timeout(0), destroy_timer(0), station(station), sink_handler(0),
    logic_is_idle(true)
{
  assert(module != 0);

  Config &cfg = module->cfg();
  const string &cfg_name = module->cfgName();
  
  string local_callsign;
  if (!cfg.getValue(cfg_name, "CALLSIGN", local_callsign))
  {
    cerr << "*** ERROR: Config variable " << cfg_name << "/CALLSIGN not set\n";
    return;
  }
  m_qso.setLocalCallsign(local_callsign);
  
  bool use_gsm_only = false;
  if (cfg.getValue(cfg_name, "USE_GSM_ONLY", use_gsm_only) && use_gsm_only)
  {
    std::cout << module->cfgName() << ": Using GSM codec only" << std::endl;
    m_qso.setUseGsmOnly();
  }

  if (!cfg.getValue(cfg_name, "SYSOPNAME", sysop_name))
  {
    cerr << "*** ERROR: Config variable " << cfg_name
      	 << "/SYSOPNAME not set\n";
    return;
  }
  m_qso.setLocalName(sysop_name);
  
  string description;
  if (!cfg.getValue(cfg_name, "DESCRIPTION", description))
  {
    cerr << "*** ERROR: Config variable " << cfg_name
      	 << "/DESCRIPTION not set\n";
    return;
  }
  m_qso.setLocalInfo(description);

  string event_handler_script;
  if (cfg.getValue(module->logicName(), "EVENT_HANDLER", event_handler_script))
  {
    std::size_t slashpos = event_handler_script.find_last_of("/");
    event_handler_script = event_handler_script.substr(0, slashpos);
    event_handler_script += "/events.d/EchoLinkRemote.tcl";
  }
  cfg.getValue(module->cfgName(), "REMOTE_EVENT_HANDLER", event_handler_script);
  if (event_handler_script.empty())
  {
    std::cerr << "*** ERROR: Config variable " << module->logicName()
              << "/EVENT_HANDLER or " << module->cfgName()
              << "/REMOTE_EVENT_HANDLER not set" << std::endl;
    return;
  }

  string idle_timeout_str;
  if (cfg.getValue(cfg_name, "LINK_IDLE_TIMEOUT", idle_timeout_str))
  {
    idle_timeout = atoi(idle_timeout_str.c_str());
    idle_timer = new Timer(1000, Timer::TYPE_PERIODIC);
    idle_timer->expired.connect(mem_fun(*this, &QsoImpl::idleTimeoutCheck));
  }
  
  sink_handler = new AudioPassthrough;
  AudioSink::setHandler(sink_handler);

  msg_handler = new MsgHandler(INTERNAL_SAMPLE_RATE);
  msg_handler->allMsgsWritten.connect(
      	  mem_fun(*this, &QsoImpl::allRemoteMsgsWritten));
	  
  AudioPacer *msg_pacer = new AudioPacer(INTERNAL_SAMPLE_RATE,
      	      	                         160*4*(INTERNAL_SAMPLE_RATE / 8000),
					 500);
  msg_handler->registerSink(msg_pacer, true);
  
  output_sel = new AudioSelector;
  output_sel->addSource(sink_handler);
  output_sel->enableAutoSelect(sink_handler, 0);
  output_sel->addSource(msg_pacer);
  output_sel->enableAutoSelect(msg_pacer, 10);
  AudioSource *prev_src = output_sel;

#if INTERNAL_SAMPLE_RATE == 16000
  AudioDecimator *down_sampler = new AudioDecimator(
          2, coeff_16_8, coeff_16_8_taps);
  prev_src->registerSink(down_sampler, true);
  prev_src = down_sampler;
#endif

  prev_src->registerSink(&m_qso);
  prev_src = 0;

  event_handler = new EventHandler(event_handler_script,
      module->logicName() + ", module " + module->cfgName());
  event_handler->playFile.connect(
      sigc::bind(mem_fun(*msg_handler, &MsgHandler::playFile), false));
  event_handler->playSilence.connect(
      sigc::bind(mem_fun(*msg_handler, &MsgHandler::playSilence), false));
  event_handler->playTone.connect(
      sigc::bind(mem_fun(*msg_handler, &MsgHandler::playTone), false));
  event_handler->getConfigValue.connect(
      sigc::mem_fun(*this, &QsoImpl::getConfigValue));

  event_handler->setVariable("logic_name", module->logicName());
  event_handler->setVariable("module_name", module->cfgName());
  event_handler->setVariable("module_type", module->name());

  event_handler->processEvent(
      std::string("namespace eval ") + module->logicName() + "::" +
      module->cfgName() + " {}");
  for (const auto& cfgvar : cfg.listSection(module->cfgName()))
  {
    std::string var =
      module->logicName() + "::" + module->cfgName() + "::CFG_" + cfgvar;
    std::string value;
    cfg.getValue(module->cfgName(), cfgvar, value);
    event_handler->setVariable(var, value);
  }

  if (!event_handler->initialize())
  {
    return;
  }

  m_qso.infoMsgReceived.connect(mem_fun(*this, &QsoImpl::onInfoMsgReceived));
  m_qso.chatMsgReceived.connect(mem_fun(*this, &QsoImpl::onChatMsgReceived));
  m_qso.stateChange.connect(mem_fun(*this, &QsoImpl::onStateChange));
  m_qso.isReceiving.connect(sigc::bind(isReceiving.make_slot(), this));
  m_qso.audioReceivedRaw.connect(
      sigc::bind(audioReceivedRaw.make_slot(), this));
  
  prev_src = &m_qso;
  
  AudioFifo *input_fifo = new AudioFifo(2048);
  input_fifo->setOverwrite(true);
  input_fifo->setPrebufSamples(1024);
  prev_src->registerSink(input_fifo, true);
  prev_src = input_fifo;
  
#if INTERNAL_SAMPLE_RATE == 16000
  AudioInterpolator *up_sampler = new AudioInterpolator(
          2, coeff_16_8, coeff_16_8_taps);
  prev_src->registerSink(up_sampler, true);
  prev_src = up_sampler;
#endif

  AudioSource::setHandler(prev_src);
  
  init_ok = true;
  
} /* QsoImpl::QsoImpl */


QsoImpl::~QsoImpl(void)
{
  AudioSink::clearHandler();
  AudioSource::clearHandler();
  delete event_handler;
  delete output_sel;
  delete msg_handler;
  delete sink_handler;
  delete idle_timer;
  delete destroy_timer;
} /* QsoImpl::~QsoImpl */


bool QsoImpl::initOk(void)
{
  return m_qso.initOk() && init_ok;
} /* QsoImpl::initOk */


void QsoImpl::logicIdleStateChanged(bool is_idle)
{
  /*
  printf("QsoImpl::logicIdleStateChanged: is_idle=%s\n",
      is_idle ? "TRUE" : "FALSE");
  */

  logic_is_idle = is_idle;
} /* QsoImpl::logicIdleStateChanged */


bool QsoImpl::sendAudioRaw(Qso::RawPacket *packet)
{
  idle_timer_cnt = 0;
  
  if (!msg_handler->isWritingMessage())
  {
    return m_qso.sendAudioRaw(packet);
  }
  
  return true;
  
} /* QsoImpl::sendAudioRaw */


bool QsoImpl::connect(void)
{
  if (destroy_timer != 0)
  {
    delete destroy_timer;
    destroy_timer = 0;
  }
  return m_qso.connect();
} /* QsoImpl::connect */


bool QsoImpl::accept(void)
{
  cout << remoteCallsign() << ": Accepting connection. EchoLink ID is "
       << station.id() << "...\n";
  bool success = m_qso.accept();
  if (success)
  {
    processEvent("remote_greeting " + remoteCallsign());
  }

  return success;
  
} /* QsoImpl::accept */


void QsoImpl::reject(bool perm)
{
  cout << "Rejecting connection from " << remoteCallsign()
       << (perm ? " permanently" : " temporarily") << endl;
  reject_qso = true;
  bool success = m_qso.accept();
  if (success)
  {
    sendChatData("The connection was rejected");
    stringstream ss;
    ss << "reject_remote_connection " << (perm ? "1" : "0");
    processEvent(ss.str());
  }
} /* QsoImpl::reject */


void QsoImpl::setListenOnly(bool enable)
{
  event_handler->setVariable(
      module->logicName() + "::" + module->cfgName() + "::listen_only_active",
      enable ? "1" : "0");
  if (enable)
  {
    string str("[listen only] ");
    str += sysop_name;
    m_qso.setLocalName(str);
  }
  else
  {
    m_qso.setLocalName(sysop_name);
  }
} /* QsoImpl::setListenOnly */


void QsoImpl::squelchOpen(bool is_open)
{
  if (currentState() == Qso::STATE_CONNECTED)
  {
    processEvent(std::string("squelch_open ") + (is_open ? "1": "0"));
  }
} /* QsoImpl::squelchOpen */


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
void QsoImpl::allRemoteMsgsWritten(void)
{
  if (reject_qso || disc_when_done)
  {
    disconnect();
  }
} /* QsoImpl::allRemoteMsgsWritten */


/*
 *----------------------------------------------------------------------------
 * Method:    onInfoMsgReceived
 * Purpose:   Called by the EchoLink::Qso object when an info message is
 *    	      received from the remote station.
 * Input:     msg - The received message
 * Output:    None
 * Author:    Tobias Blomberg / SM0SVX
 * Created:   2004-03-07
 * Remarks:   
 * Bugs:      
 *----------------------------------------------------------------------------
 */
void QsoImpl::onInfoMsgReceived(const string& msg)
{
  if (msg != last_info_msg)
  {
    cout << "--- EchoLink info message received from " << remoteCallsign()
	 << " ---" << endl
	 << msg << endl;
    last_info_msg = msg;
    infoMsgReceived(this, msg);
  }  
} /* onInfoMsgReceived */


/*
 *----------------------------------------------------------------------------
 * Method:    onChatMsgReceived
 * Purpose:   Called by the EchoLink::Qso object when a chat message is
 *    	      received from the remote station.
 * Input:     msg - The received message
 * Output:    None
 * Author:    Tobias Blomberg / SM0SVX
 * Created:   2004-07-29
 * Remarks:   
 * Bugs:      
 *----------------------------------------------------------------------------
 */
void QsoImpl::onChatMsgReceived(const string& msg)
{
  cout << "--- EchoLink chat message received from " << remoteCallsign()
       << " ---" << endl
       << msg << endl;
  chatMsgReceived(this, msg);
} /* onChatMsgReceived */


/*
 *----------------------------------------------------------------------------
 * Method:    onStateChange
 * Purpose:   Called by the EchoLink::Qso object when the connection state
 *    	      changes.
 * Input:     state - The state new state of the QSO
 * Output:    None
 * Author:    Tobias Blomberg / SM0SVX
 * Created:   2004-03-07
 * Remarks:   
 * Bugs:      
 *----------------------------------------------------------------------------
 */
void QsoImpl::onStateChange(Qso::State state)
{
  cout << remoteCallsign() << ": EchoLink QSO state changed to ";
  switch (state)
  {
    case Qso::STATE_DISCONNECTED:
      cout << "DISCONNECTED\n";
      if (!reject_qso)
      {
      	stringstream ss;
	ss << "disconnected " << remoteCallsign();
      	module->processEvent(ss.str());
      }
      destroy_timer = new Timer(5000);
      destroy_timer->expired.connect(mem_fun(*this, &QsoImpl::destroyMeNow));
      break;
    case Qso::STATE_CONNECTING:
      cout << "CONNECTING\n";
      break;
    case Qso::STATE_CONNECTED:
      cout << "CONNECTED\n";
      if (!reject_qso)
      {
	if (m_qso.isRemoteInitiated())
	{
      	  stringstream ss;
	  ss << "remote_connected " << remoteCallsign();
      	  module->processEvent(ss.str());
	}
	else
	{
          stringstream ss;
          ss << "connected " << remoteCallsign();
          module->processEvent(ss.str());
	}
      }
      break;
    case Qso::STATE_BYE_RECEIVED:
      cout << "BYE_RECEIVED\n";
      break;
    default:
      cout << "???\n";
      break;
  }
  stateChange(this, state);
} /* onStateChange */


void QsoImpl::idleTimeoutCheck(Timer *t)
{
  if (receivingAudio() || !logic_is_idle)
  {
    idle_timer_cnt = 0;
    return;
  }

  if (++idle_timer_cnt == idle_timeout)
  {
    cout << remoteCallsign()
         << ": EchoLink connection idle timeout. Disconnecting..." << endl;
    module->processEvent("link_inactivity_timeout");
    disc_when_done = true;
    processEvent("remote_timeout");
    if (!msg_handler->isWritingMessage())
    {
      disconnect();
    }
  }
} /* idleTimeoutCheck */


void QsoImpl::destroyMeNow(Timer *t)
{
  destroyMe(this);
} /* destroyMeNow */


bool QsoImpl::getConfigValue(const std::string& section, const std::string& tag,
                             std::string& value)
{
  const Config &cfg = module->cfg();
  return cfg.getValue(section, tag, value, true);
} /* QsoImpl::getConfigValue */


void QsoImpl::processEvent(const std::string& event)
{
  msg_handler->begin();
  event_handler->processEvent(
      module->logicName() + "::" + module->cfgName() + "::" + event);
  msg_handler->end();
} /* QsoImpl::processEvent */


/*
 * This file has not been truncated
 */

