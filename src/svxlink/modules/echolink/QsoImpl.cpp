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


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncConfig.h>
//#include <Module.h>

#include <MsgHandler.h>
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


QsoImpl::QsoImpl(const Async::IpAddress& ip, ModuleEchoLink *module)
  : Qso(ip), module(module), msg_handler(0), msg_pacer(0), init_ok(false),
    reject_qso(false), last_message(""), last_info_msg("")
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
  
  string sound_base_dir;
  if (!cfg.getValue(module->logicName(), "SOUNDS", sound_base_dir))
  {
    cerr << "*** ERROR: Config variable " << cfg_name
      	 << "/SOUNDS not set\n";
    return;
  }
  
  msg_handler = new MsgHandler(sound_base_dir, 8000);
  
  msg_pacer = new AudioPacer(8000, 160*4, 500);
  msg_handler->writeAudio.connect(slot(msg_pacer, &AudioPacer::audioInput));
  msg_handler->allMsgsWritten.connect(
      	  slot(msg_pacer, &AudioPacer::flushAllAudio));
  msg_pacer->audioInputBufFull.connect(
      	  slot(msg_handler, &MsgHandler::writeBufferFull));
  msg_pacer->allAudioFlushed.connect(
      	  slot(this, &QsoImpl::allRemoteMsgsWritten));
  msg_pacer->audioOutput.connect(slot(this, &Qso::sendAudio));
  
  Qso::infoMsgReceived.connect(slot(this, &QsoImpl::onInfoMsgReceived));
  Qso::chatMsgReceived.connect(slot(this, &QsoImpl::onChatMsgReceived));
  Qso::stateChange.connect(slot(this, &QsoImpl::onStateChange));
  Qso::isReceiving.connect(bind(isReceiving.slot(), this));
  Qso::audioReceived.connect(bind(audioReceived.slot(), this));
  Qso::audioReceivedRaw.connect(bind(audioReceivedRaw.slot(), this));

  
  init_ok = true;
  
} /* QsoImpl::QsoImpl */


QsoImpl::~QsoImpl(void)
{
  delete msg_pacer;
  delete msg_handler;
} /* QsoImpl::~QsoImpl */


bool QsoImpl::initOk(void)
{
  return Qso::initOk() && init_ok;
} /* QsoImpl::initOk */


int QsoImpl::sendAudio(short *buf, int len)
{
    /* FIXME: Buffer audio until the message has been written. */
  if (!msg_handler->isWritingMessage())
  {
    len = Qso::sendAudio(buf, len);
  }
  
  return len;
  
} /* QsoImpl::sendAudio */


bool QsoImpl::accept(void)
{
  cout << "Accepting connection from " << remoteCallsign() << endl;
  bool success = Qso::accept();
  if (success)
  {
    msg_handler->playMsg("EchoLink", "greeting");
  }
  
  return success;
  
} /* QsoImpl::accept */


void QsoImpl::reject(void)
{
  cout << "Rejecting connection from " << remoteCallsign() << endl;
  reject_qso = true;
  bool success = Qso::accept();
  if (success)
  {
    sendChatData("The connection was rejected");
    msg_handler->playMsg("EchoLink", "reject_connection");
    msg_handler->playSilence(1000);
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
  if (reject_qso)
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
	module->spellCallsign(remoteCallsign());
	module->playMsg("disconnected");
	module->playSilence(500);
      }
      destroyMe(this);
      break;
    case Qso::STATE_CONNECTING:
      cout << "CONNECTING\n";
      break;
    case Qso::STATE_CONNECTED:
      cout << "CONNECTED\n";
      if (!reject_qso)
      {
	module->playMsg("connected");
	if (isRemoteInitiated())
	{
      	  module->spellCallsign(remoteCallsign());
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
} /* onStateChange */



/*
 * This file has not been truncated
 */

