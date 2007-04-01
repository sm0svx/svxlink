/**
@file	 QsoImpl.cpp
@brief   Data for one EchoLink Qso.
@author  Tobias Blomberg / SM0SVX
@date	 2004-06-02

This file contains a class that implementes the things needed for one
EchoLink Qso.

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

#include <cassert>
#include <sigc++/bind.h>
#include <sstream>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncConfig.h>
//#include <Module.h>

#include <MsgHandler.h>
#include <EventHandler.h>
#include <AudioPacer.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "ModuleEchoLink.h"
#include "QsoImpl.h"



/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/

using namespace std;
using namespace Async;
using namespace EchoLink;



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
  : Qso(station.ip()), module(module), event_handler(0), msg_handler(0),
    msg_pacer(0), init_ok(false), reject_qso(false), last_message(""),
    last_info_msg(""), idle_timer(0), disc_when_done(false), idle_timer_cnt(0),
    idle_timeout(0), destroy_timer(0), station(station)
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
  setLocalCallsign(local_callsign);
  
  string sysop_name;
  if (!cfg.getValue(cfg_name, "SYSOPNAME", sysop_name))
  {
    cerr << "*** ERROR: Config variable " << cfg_name
      	 << "/SYSOPNAME not set\n";
    return;
  }
  setLocalName(sysop_name);
  
  string description;
  if (!cfg.getValue(cfg_name, "DESCRIPTION", description))
  {
    cerr << "*** ERROR: Config variable " << cfg_name
      	 << "/DESCRIPTION not set\n";
    return;
  }
  setLocalInfo(description);
  
  string event_handler_script;
  if (!cfg.getValue(module->logicName(), "EVENT_HANDLER", event_handler_script))
  {
    cerr << "*** ERROR: Config variable " << module->logicName()
      	 << "/EVENT_HANDLER not set\n";
    return;
  }
  
  string idle_timeout_str;
  if (cfg.getValue(cfg_name, "LINK_IDLE_TIMEOUT", idle_timeout_str))
  {
    idle_timeout = atoi(idle_timeout_str.c_str());
    idle_timer = new Timer(1000, Timer::TYPE_PERIODIC);
    idle_timer->expired.connect(slot(*this, &QsoImpl::idleTimeoutCheck));
  }
  
  msg_handler = new MsgHandler(8000);
  
  msg_pacer = new AudioPacer(8000, 160*4, 500);
  msg_handler->writeAudio.connect(slot(*msg_pacer, &AudioPacer::audioInput));
  msg_handler->allMsgsWritten.connect(
      	  slot(*msg_pacer, &AudioPacer::flushAllAudio));
  msg_pacer->audioInputBufFull.connect(
      	  slot(*msg_handler, &MsgHandler::writeBufferFull));
  msg_pacer->allAudioFlushed.connect(
      	  slot(*this, &QsoImpl::allRemoteMsgsWritten));
  msg_pacer->audioOutput.connect(slot(*this, &Qso::sendAudio));
  
  event_handler = new EventHandler(event_handler_script, 0);
  event_handler->playFile.connect(slot(*msg_handler, &MsgHandler::playFile));
  event_handler->playSilence.connect(
      	  slot(*msg_handler, &MsgHandler::playSilence));
  event_handler->playTone.connect(slot(*msg_handler, &MsgHandler::playTone));

    // Workaround: Need to set the ID config variable and "logic_name"
    // variable to load the TCL script.
  event_handler->processEvent("namespace eval EchoLink {}");
  event_handler->setVariable("EchoLink::CFG_ID", "0");
  event_handler->setVariable("logic_name", "Default");
  
  event_handler->initialize();
  
  Qso::infoMsgReceived.connect(slot(*this, &QsoImpl::onInfoMsgReceived));
  Qso::chatMsgReceived.connect(slot(*this, &QsoImpl::onChatMsgReceived));
  Qso::stateChange.connect(slot(*this, &QsoImpl::onStateChange));
  Qso::isReceiving.connect(bind(isReceiving.slot(), this));
  Qso::audioReceived.connect(bind(audioReceived.slot(), this));
  Qso::audioReceivedRaw.connect(bind(audioReceivedRaw.slot(), this));
  
  init_ok = true;
  
} /* QsoImpl::QsoImpl */


QsoImpl::~QsoImpl(void)
{
  delete event_handler;
  delete msg_handler;
  delete msg_pacer;
  delete idle_timer;
  delete destroy_timer;
} /* QsoImpl::~QsoImpl */


bool QsoImpl::initOk(void)
{
  return Qso::initOk() && init_ok;
} /* QsoImpl::initOk */


int QsoImpl::sendAudio(float *buf, int len)
{
  idle_timer_cnt = 0;
  
  if (!msg_handler->isWritingMessage())
  {
    len = Qso::sendAudio(buf, len);
  }
  
  return len;
  
} /* QsoImpl::sendAudio */


bool QsoImpl::sendAudioRaw(GsmVoicePacket *packet)
{
  idle_timer_cnt = 0;
  
  if (!msg_handler->isWritingMessage())
  {
    return Qso::sendAudioRaw(packet);
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
  return Qso::connect();
} /* QsoImpl::connect */


bool QsoImpl::accept(void)
{
  cout << remoteCallsign() << ": Accepting connection. EchoLink ID is "
       << station.id() << "...\n";
  bool success = Qso::accept();
  if (success)
  {
    msg_handler->begin();
    event_handler->processEvent(string(module->name()) + "::remote_greeting");
    msg_handler->end();
  }
  
  return success;
  
} /* QsoImpl::accept */


void QsoImpl::reject(bool perm)
{
  cout << "Rejecting connection from " << remoteCallsign()
       << (perm ? " permanently" : " temporarily") << endl;
  reject_qso = true;
  bool success = Qso::accept();
  if (success)
  {
    sendChatData("The connection was rejected");
    msg_handler->begin();
    stringstream ss;
    ss << module->name() << "::reject_remote_connection "
       << (perm ? "1" : "0");
    event_handler->processEvent(ss.str());
    msg_handler->end();
  }
} /* QsoImpl::reject */



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
void QsoImpl::allRemoteMsgsWritten(void)
{
  flushAudioSendBuffer();
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
      destroy_timer->expired.connect(slot(*this, &QsoImpl::destroyMeNow));
      break;
    case Qso::STATE_CONNECTING:
      cout << "CONNECTING\n";
      break;
    case Qso::STATE_CONNECTED:
      cout << "CONNECTED\n";
      if (!reject_qso)
      {
	if (isRemoteInitiated())
	{
      	  stringstream ss;
	  ss << "remote_connected " << remoteCallsign();
      	  module->processEvent(ss.str());
	}
	else
	{
	  module->processEvent("connected");
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
  if (receivingAudio())
  {
    idle_timer_cnt = 0;
    return;
  }  
  
  if (++idle_timer_cnt == idle_timeout)
  {
    cout << remoteCallsign() << ": EchoLink connection idle timeout. "
      	 "Disconnecting...\n";
    module->processEvent("link_inactivity_timeout");
    disc_when_done = true;
    msg_handler->begin();
    event_handler->processEvent(string(module->name()) + "::remote_timeout");
    msg_handler->end();
  }
} /* idleTimeoutCheck */


void QsoImpl::destroyMeNow(Timer *t)
{
  destroyMe(this);
} /* destroyMeNow */



/*
 * This file has not been truncated
 */

