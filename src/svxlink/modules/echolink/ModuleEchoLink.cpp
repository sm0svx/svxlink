/**
@file	 ModuleEchoLink.cpp
@brief   A module that provides EchoLink connection possibility
@author  Tobias Blomberg / SM0SVX
@date	 2004-03-07

\verbatim
A module (plugin) for the multi purpose tranciever frontend system.
Copyright (C) 2004 Tobias Blomberg / SM0SVX

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

#include <stdio.h>

#include <algorithm>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <version/MODULE_ECHOLINK.h>
#include <AsyncTimer.h>
#include <AsyncConfig.h>
#include <EchoLinkDirectory.h>
#include <EchoLinkDispatcher.h>
#include <EchoLinkQso.h>

#include <MsgHandler.h>
#include <AudioPacer.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "ModuleEchoLink.h"



/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/

using namespace std;
using namespace SigC;
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
 * Pure C-functions
 *
 ****************************************************************************/


extern "C" {
  Module *module_init(void *dl_handle, Logic *logic, const char *cfg_name)
  { 
    return new ModuleEchoLink(dl_handle, logic, cfg_name);
  }
} /* extern "C" */



/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/


ModuleEchoLink::ModuleEchoLink(void *dl_handle, Logic *logic,
      	      	      	       const string& cfg_name)
  : Module(dl_handle, logic, cfg_name), dir(0), qso(0), dir_refresh_timer(0),
    remote_activation(false)
{
  cout << "\tModule " << name()
       << " v" MODULE_ECHOLINK_VERSION " starting...\n";
  
} /* ModuleEchoLink */


ModuleEchoLink::~ModuleEchoLink(void)
{
  moduleCleanup();
} /* ~ModuleEchoLink */


bool ModuleEchoLink::initialize(void)
{
  if (!Module::initialize())
  {
    return false;
  }
  
  string server;
  if (!cfg().getValue(cfgName(), "SERVER", server))
  {
    cerr << "*** Error: Config variable " << cfgName() << "/SERVER not set\n";
    return false;
  }
  
  if (!cfg().getValue(cfgName(), "CALLSIGN", callsign))
  {
    cerr << "*** Error: Config variable " << cfgName() << "/CALLSIGN not set\n";
    return false;
  }
  
  string password;
  if (!cfg().getValue(cfgName(), "PASSWORD", password))
  {
    cerr << "*** Error: Config variable " << cfgName() << "/PASSWORD not set\n";
    return false;
  }
  
  string location;
  if (!cfg().getValue(cfgName(), "LOCATION", location))
  {
    cerr << "*** Error: Config variable " << cfgName() << "/LOCATION not set\n";
    return false;
  }
  
  if (!cfg().getValue(cfgName(), "SYSOPNAME", sysop_name))
  {
    cerr << "*** Error: Config variable " << cfgName()
      	 << "/SYSOPNAME not set\n";
    return false;
  }
  
  if (!cfg().getValue(cfgName(), "DESCRIPTION", description))
  {
    cerr << "*** Error: Config variable " << cfgName()
      	 << "/DESCRIPTION not set\n";
    return false;
  }
  
  cfg().getValue(cfgName(), "ALLOW_IP", allow_ip);
  
  string sound_base_dir;
  if (!cfg().getValue(logicName(), "SOUNDS", sound_base_dir))
  {
    cerr << "*** Error: Config variable " << logicName()
      	 << "/SOUNDS not set\n";
    return false;
  }
  
    // Initialize directory server communication
  dir = new Directory(server, callsign, password, location);
  dir->statusChanged.connect(slot(this, &ModuleEchoLink::onStatusChanged));
  dir->stationListUpdated.connect(
      	  slot(this, &ModuleEchoLink::onStationListUpdated));
  dir->error.connect(slot(this, &ModuleEchoLink::onError));
  //dir->makeBusy();
  dir->makeOnline();
  
  msg_handler = new MsgHandler(sound_base_dir);
  
  msg_pacer = new AudioPacer(8000, 160*4, 500);
  msg_handler->writeAudio.connect(slot(msg_pacer, &AudioPacer::audioInput));
  msg_handler->allMsgsWritten.connect(
      	  slot(msg_pacer, &AudioPacer::flushAllAudio));
  msg_pacer->audioInputBufFull.connect(
      	  slot(msg_handler, &MsgHandler::writeBufferFull));
  msg_pacer->allAudioFlushed.connect(
      	  slot(this, &ModuleEchoLink::allMsgsWritten));
  
    // Start listening to the EchoLink UDP ports
  if (Dispatcher::instance() == 0)
  {
    cerr << "*** Error: Could not create EchoLink listener (Dispatcher) "
      	    "object\n";
    moduleCleanup();
    return false;
  }
  Dispatcher::instance()->incomingConnection.connect(
      slot(this, &ModuleEchoLink::onIncomingConnection));
  
  return true;
  
} /* ModuleEchoLink::initialize */




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


void ModuleEchoLink::moduleCleanup(void)
{
  delete msg_pacer;
  msg_pacer = 0;
  delete msg_handler;
  msg_handler = 0;
  delete dir_refresh_timer;
  dir_refresh_timer = 0;
  delete qso;
  qso = 0;
  delete Dispatcher::instance();
  delete dir;
  dir = 0;
} /* ModuleEchoLink::moduleCleanup */


/*
 *----------------------------------------------------------------------------
 * Method:    activateInit
 * Purpose:   Called by the core system when this module is activated.
 * Input:     None
 * Output:    None
 * Author:    Tobias Blomberg / SM0SVX
 * Created:   2004-03-07
 * Remarks:   
 * Bugs:      
 *----------------------------------------------------------------------------
 */
void ModuleEchoLink::activateInit(void)
{
  
} /* activateInit */


/*
 *----------------------------------------------------------------------------
 * Method:    deactivateCleanup
 * Purpose:   Called by the core system when this module is deactivated.
 * Input:     None
 * Output:    None
 * Author:    Tobias Blomberg / SM0SVX
 * Created:   2004-03-07
 * Remarks:   Do NOT call this function directly unless you really know what
 *    	      you are doing. Use Module::deactivate() instead.
 * Bugs:      
 *----------------------------------------------------------------------------
 */
void ModuleEchoLink::deactivateCleanup(void)
{
  remote_activation = false;
} /* deactivateCleanup */


/*
 *----------------------------------------------------------------------------
 * Method:    dtmfDigitReceived
 * Purpose:   Called by the core system when a DTMF digit has been
 *    	      received.
 * Input:     digit - The DTMF digit received (0-9, A-D, *, #)
 * Output:    None
 * Author:    Tobias Blomberg / SM0SVX
 * Created:   2004-03-07
 * Remarks:   
 * Bugs:      
 *----------------------------------------------------------------------------
 */
void ModuleEchoLink::dtmfDigitReceived(char digit)
{
  printf("DTMF digit received in module %s: %c\n", name(), digit);
  
} /* dtmfDigitReceived */


/*
 *----------------------------------------------------------------------------
 * Method:    dtmfCmdReceived
 * Purpose:   Called by the core system when a DTMF command has been
 *    	      received. A DTMF command consists of a string of digits ended
 *    	      with a number sign (#). The number sign is not included in the
 *    	      command string.
 * Input:     cmd - The received command.
 * Output:    None
 * Author:    Tobias Blomberg / SM0SVX
 * Created:   2004-03-07
 * Remarks:   
 * Bugs:      
 *----------------------------------------------------------------------------
 */
void ModuleEchoLink::dtmfCmdReceived(const string& cmd)
{
  printf("DTMF command received in module %s: %s\n", name(), cmd.c_str());
  
  if (cmd == "")
  {
    if (qso != 0)
    {
      qso->disconnect();
    }
    else
    {
      deactivateMe();
    }
  }
  else if (qso == 0)
  {
    const StationData *station = dir->findStation(atoi(cmd.c_str()));
    if (station != 0)
    {
      qso = new Qso(station->ip(), callsign, sysop_name, description);
      if (!qso->initOk())
      {
	delete qso;
	cerr << "Creation of Qso failed\n";
	playMsg("operation_failed");
	return;
      }
      qso->infoMsgReceived.connect(
      	      slot(this, &ModuleEchoLink::onInfoMsgReceived));
      qso->stateChange.connect(slot(this, &ModuleEchoLink::onStateChange));
      qso->isReceiving.connect(slot(this, &ModuleEchoLink::onIsReceiving));
      qso->audioReceived.connect(slot(this, &Module::audioFromModule));
      qso->connect();
    }
    else
    {
      spellWord(cmd);
      playMsg("not_found");
    }
  }
  else
  {
    playMsg("link_busy");
  }
} /* dtmfCmdReceived */


/*
 *----------------------------------------------------------------------------
 * Method:    playHelpMsg
 * Purpose:   Called by the core system to play a help message for this
 *    	      module.
 * Input:     None
 * Output:    None
 * Author:    Tobias Blomberg / SM0SVX
 * Created:   2004-03-07
 * Remarks:   
 * Bugs:      
 *----------------------------------------------------------------------------
 */
void ModuleEchoLink::playHelpMsg(void)
{
  playMsg("help");
} /* playHelpMsg */


/*
 *----------------------------------------------------------------------------
 * Method:    squelchOpen
 * Purpose:   Called by the core system when activity is detected
 *    	      on the receiver.
 * Input:     is_open - true if the squelch is open or else false.
 * Output:    None
 * Author:    Tobias Blomberg / SM0SVX
 * Created:   2004-03-07
 * Remarks:   
 * Bugs:      
 *----------------------------------------------------------------------------
 */
void ModuleEchoLink::squelchOpen(bool is_open)
{
  //printf("RX squelch is %s...\n", is_open ? "open" : "closed");
  
} /* squelchOpen */


/*
 *----------------------------------------------------------------------------
 * Method:    audioFromRx
 * Purpose:   Called by the core system when the audio is received.
 * Input:     samples - The received samples
 *    	      count   - The number of samples received
 * Output:    Returns the number of samples processed.
 * Author:    Tobias Blomberg / SM0SVX
 * Created:   2004-03-07
 * Remarks:   
 * Bugs:      
 *----------------------------------------------------------------------------
 */
int ModuleEchoLink::audioFromRx(short *samples, int count)
{
    // FIXME: FIFO for saving samples if writing message
  if ((qso != 0) && (!msg_handler->isWritingMessage()))
  {
    count = qso->sendAudio(samples, count);
  }
  
  return count;
  
} /* audioFromRx */





/*
 *----------------------------------------------------------------------------
 * Method:    onStatusChanged
 * Purpose:   Called by the EchoLink::Directory object when the status of
 *    	      the registration is changed in the directory server.
 * Input:     status  - The new status
 * Output:    None
 * Author:    Tobias Blomberg / SM0SVX
 * Created:   2004-03-07
 * Remarks:   
 * Bugs:      
 *----------------------------------------------------------------------------
 */
void ModuleEchoLink::onStatusChanged(StationData::Status status)
{
  cout << "EchoLink directory status changed to "
       << StationData::statusStr(status) << endl;
  if (status == StationData::STAT_ONLINE)
  {
    getDirectoryList();
  }
} /* onStatusChanged */


/*
 *----------------------------------------------------------------------------
 * Method:    onStationListUpdated
 * Purpose:   Called by the EchoLink::Directory object when the station list
 *    	      has been updated.
 * Input:     None
 * Output:    None
 * Author:    Tobias Blomberg / SM0SVX
 * Created:   2004-03-07
 * Remarks:   
 * Bugs:      
 *----------------------------------------------------------------------------
 */
void ModuleEchoLink::onStationListUpdated(void)
{
  /*
  const list<StationData>& stations = dir->stations();
  list<StationData>::const_iterator it;
  for (it = stations.begin(); it != stations.end(); ++it)
  {
    cerr << *it << endl;
  }
  */

  cout << "--- EchoLink directory server message: ---" << endl;
  cout << dir->message() << endl;

} /* onStationListUpdated */


/*
 *----------------------------------------------------------------------------
 * Method:    onError
 * Purpose:   Called by the EchoLink::Directory object when a communication
 *    	      error occurs.
 * Input:     msg - The error message
 * Output:    None
 * Author:    Tobias Blomberg / SM0SVX
 * Created:   2004-03-07
 * Remarks:   
 * Bugs:      
 *----------------------------------------------------------------------------
 */
void ModuleEchoLink::onError(const string& msg)
{
  cerr << "*** EchoLink directory server error: " << msg << endl;
  //Application::app().quit();
} /* onError */



/*
 *----------------------------------------------------------------------------
 * Method:    onIncomingConnection
 * Purpose:   Called by the EchoLink::Dispatcher object when a new remote
 *    	      connection is coming in.
 * Input:     callsign	- The callsign of the remote station
 *    	      name    	- The name of the remote station
 * Output:    None
 * Author:    Tobias Blomberg / SM0SVX
 * Created:   2004-03-07
 * Remarks:   
 * Bugs:      
 *----------------------------------------------------------------------------
 */
void ModuleEchoLink::onIncomingConnection(const IpAddress& ip,
      	      	      	      	      	  const string& callsign,
      	      	      	      	      	  const string& name)
{
  cout << "Incoming EchoLink connection from " << callsign
       << " (" << name << ")\n";
  
  if (qso != 0) // A connection is already active
  {
    // FIXME: Send BYE or maybe first an audio info message to remote station
    cerr << "EchoLink is BUSY...\n";
    return;
  }
  
  const StationData *station;
  StationData tmp_stn_data;
  if (ip.isWithinSubet(allow_ip))
  {
    tmp_stn_data.setIp(ip);
    tmp_stn_data.setCallsign(callsign);
    station = &tmp_stn_data;
  }
  else
  {
      // Check if the incoming callsign is valid
    station = dir->findCall(callsign);
    if (station == 0)
    {
      getDirectoryList();
      return;
    }
  }
  
    // Create a new Qso object to accept the connection
  qso = new Qso(station->ip(), callsign, sysop_name, description);
  if (!qso->initOk())
  {
    delete qso;
    cerr << "*** Error: Creation of Qso object failed\n";
    return;
  }
  qso->setRemoteCallsign(callsign);
  qso->setRemoteName(name);
  qso->infoMsgReceived.connect(slot(this, &ModuleEchoLink::onInfoMsgReceived));
  qso->stateChange.connect(slot(this, &ModuleEchoLink::onStateChange));
  qso->isReceiving.connect(slot(this, &ModuleEchoLink::onIsReceiving));
  qso->audioReceived.connect(slot(this, &Module::audioFromModule));
  msg_pacer->audioOutput.connect(slot(qso, &Qso::sendAudio));
  
  if (!isActive())
  {
    if (!activateMe())
    {
      // FIXME: Send BYE or maybe first an audio info message to remote station
      delete qso;
      cerr << "*** Warning: Could not accept incoming connection from "
      	   << callsign << " since the frontend was busy doing something else.";
      return;
    }
    remote_activation = true;
  }
  qso->accept();
  
  msg_handler->playMsg("EchoLink", "greeting");
  
} /* onIncomingConnection */


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
void ModuleEchoLink::onInfoMsgReceived(const string& msg)
{
  cout << "--- EchoLink info message received from " << qso->remoteCallsign()
       << " ---" << endl
       << msg << endl;
  //qso->disconnect();
} /* onInfoMsgReceived */


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
void ModuleEchoLink::onStateChange(Qso::State state)
{
  cout << "EchoLink QSO state changed to ";
  switch (state)
  {
    case Qso::STATE_DISCONNECTED:
      cout << "DISCONNECTED\n";
      spellCallsign(qso->remoteCallsign());
      playMsg("disconnected");
      delete qso;
      qso = 0;
      if (remote_activation)
      {
      	deactivateMe();
      }
      break;
    case Qso::STATE_CONNECTING:
      cout << "CONNECTING\n";
      playMsg("connecting");
      //spellCallsign(qso->remoteCallsign());
      break;
    case Qso::STATE_CONNECTED:
      cout << "CONNECTED\n";
      playMsg("connected");
      spellCallsign(qso->remoteCallsign());
      break;
    case Qso::STATE_BYE_RECEIVED:
      break;
    default:
      cout << "???\n";
      break;
  }
} /* onStateChange */


/*
 *----------------------------------------------------------------------------
 * Method:    onIsReceiving
 * Purpose:   Called by the EchoLink::Qso object to indicate whether the
 *    	      remote station is transmitting or not.
 * Input:     is_receiving  - true=remote station is transmitting
 *    	      	      	      false=remote station is not transmitting
 * Output:    None
 * Author:    Tobias Blomberg / SM0SVX
 * Created:   2004-03-07
 * Remarks:   
 * Bugs:      
 *----------------------------------------------------------------------------
 */
void ModuleEchoLink::onIsReceiving(bool is_receiving)
{
  //cerr << "EchoLink receiving" << (is_receiving ? "true" : "false") << endl;
  
  transmit(is_receiving);
} /* onIsReceiving */


/*
 *----------------------------------------------------------------------------
 * Method:    onAudioReceived
 * Purpose:   Called by the EchoLink::Qso object each time a packet of audio
 *    	      data has been received from the remote station.
 * Input:     sample  - An array of samples
 *    	      count   - The number of samples received.
 * Output:    None
 * Author:    Tobias Blomberg / SM0SVX
 * Created:   2004-03-07
 * Remarks:   
 * Bugs:      
 *----------------------------------------------------------------------------
 */
void ModuleEchoLink::onAudioReceived(short *samples, int count)
{
  //cerr << "Samples received from remote station\n";
  audioFromModule(samples, count);  
} /* onAudioReceived */


/*
 *----------------------------------------------------------------------------
 * Method:    getDirectoryList
 * Purpose:   Initiate the process of getting a directory list from the
 *    	      directory server. A timer is also setup to automatically
 *    	      refresh the directory listing.
 * Input:     timer - The timer instance (not used)
 * Output:    None
 * Author:    Tobias Blomberg / SM0SVX
 * Created:   2004-03-07
 * Remarks:   
 * Bugs:      
 *----------------------------------------------------------------------------
 */
void ModuleEchoLink::getDirectoryList(Timer *timer)
{
  cout << "Refreshing directory list...\n";
  dir->getCalls();
  delete dir_refresh_timer;
  dir_refresh_timer = new Timer(600000);
  dir_refresh_timer->expired.connect(
      	  slot(this, &ModuleEchoLink::getDirectoryList));
} /* ModuleEchoLink::getDirectoryList */


void ModuleEchoLink::spellCallsign(const string& callsign)
{
  string call(callsign);
  char type = 'S';
  string::const_iterator dash = find(callsign.begin(), callsign.end(), '-');
  if (dash != callsign.end())
  {
    call = string(callsign.begin(), dash);
    ++dash;
    if (dash != callsign.end())
    {
      type = *dash;
    }
  }
  
  if (callsign[0] == '*')
  {
    type = 'C';
  }
  
  //cout << "Call=" << call << " type=" << type << endl;
  
  switch (type)
  {
    case 'S':
      spellWord(call);
      break;
    case 'L':
      spellWord(call);
      playMsg("link");
      break;
    case 'R':
      spellWord(call);
      playMsg("repeater");
      break;
    case 'C':
      playMsg("conference");
      spellWord(call);
      break;
  }
  
} /* spellCallsign */


void ModuleEchoLink::allMsgsWritten(void)
{
  //printf("ModuleEchoLink::allMsgsWritten\n");
  if (qso != 0)
  {
    qso->flushAudioSendBuffer();
  }
} /* ModuleEchoLink::allMsgsWritten */


/*
 * This file has not been truncated
 */
