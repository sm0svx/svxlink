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
#include <cassert>
#include <sstream>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <version/SVXLINK.h>
#include <AsyncTimer.h>
#include <AsyncConfig.h>
#include <EchoLinkDirectory.h>
#include <EchoLinkDispatcher.h>

#include <MsgHandler.h>
#include <AudioPacer.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "QsoImpl.h"
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
  : Module(dl_handle, logic, cfg_name), dir(0), dir_refresh_timer(0),
    remote_activation(false), pending_connect_id(-1), last_message(""),
    outgoing_con_pending(false), max_connections(1), max_qsos(1), talker(0),
    squelch_is_open(false)
{
  cout << "\tModule " << name()
       << " v" SVXLINK_VERSION " starting...\n";
  
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
  
  if (!cfg().getValue(cfgName(), "CALLSIGN", mycall))
  {
    cerr << "*** Error: Config variable " << cfgName() << "/CALLSIGN not set\n";
    return false;
  }
  if (mycall == "MYCALL-L")
  {
    cerr << "*** Error: Please set the EchoLink callsign (" << cfgName()
      	 << "/CALLSIGN) to a real callsign\n";
    return false;
  }
  
  string password;
  if (!cfg().getValue(cfgName(), "PASSWORD", password))
  {
    cerr << "*** Error: Config variable " << cfgName() << "/PASSWORD not set\n";
    return false;
  }
  if (password == "MyPass")
  {
    cerr << "*** Error: Please set the EchoLink password (" << cfgName()
      	 << "/PASSWORD) to a real password\n";
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
  
  string value;
  if (cfg().getValue(cfgName(), "MAX_CONNECTIONS", value))
  {
    max_connections = atoi(value.c_str());
  }
  
  if (cfg().getValue(cfgName(), "MAX_QSOS", value))
  {
    max_qsos = atoi(value.c_str());
  }
  
  if (max_qsos > max_connections)
  {
    cerr << "*** Error: The value of " << cfgName() << "/MAX_CONNECTIONS ("
      	 << max_connections << ") must be greater or equal to the value of "
	 << cfgName() << "/MAX_QSOS (" << max_qsos << ").\n";
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
  dir = new Directory(server, mycall, password, location);
  dir->statusChanged.connect(slot(this, &ModuleEchoLink::onStatusChanged));
  dir->stationListUpdated.connect(
      	  slot(this, &ModuleEchoLink::onStationListUpdated));
  dir->error.connect(slot(this, &ModuleEchoLink::onError));
  dir->makeOnline();
  
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
    {
      string::iterator end = remove(call.begin(), call.end(), '*');
      string conf_name(call.begin(), end);
      playMsg("conference");
      spellWord(conf_name);
      break;
    }
  }
  
} /* spellCallsign */




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
  //FIXME: Delete qso objects
  
  delete dir_refresh_timer;
  dir_refresh_timer = 0;
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
  //printf("DTMF digit received in module %s: %c\n", name(), digit);
  
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
    if (qsos.size() != 0)
    {
      qsos.back()->disconnect();
    }
    else
    {
      deactivateMe();
    }
  }
  else if (cmd == "0")
  {
    playHelpMsg();
  }
  else if (qsos.size() < max_qsos)
  {
    if ((dir->status() == StationData::STAT_OFFLINE) ||
      	(dir->status() == StationData::STAT_UNKNOWN))
    {
      playMsg("directory_server_offline");
      return;
    }
    
    int station_id = atoi(cmd.c_str());
    const StationData *station = dir->findStation(station_id);
    if (station != 0)
    {
      createOutgoingConnection(station);
    }
    else
    {
      getDirectoryList();
      pending_connect_id = station_id;
    }
  }
  else
  {
    // FIXME: Change message to something like "no more connections allowed".
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
  
  squelch_is_open = is_open;
  setIdle(!is_open && (qsos.size() == 0));
  broadcastTalkerStatus();
  
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
  if (qsos.size() > 0)
  {
    list<QsoImpl*>::iterator it;
    for (it=qsos.begin(); it!=qsos.end(); ++it)
    {
      // FIXME: Take care of the case where not all samples are written
      (*it)->sendAudio(samples, count);
    }
  }
  
  return count;
  
} /* audioFromRx */


/*
 *----------------------------------------------------------------------------
 * Method:    allMsgsWritten
 * Purpose:   Called by the core system when all audio messages queued
 *            for playing have been played.
 * Input:     None
 * Output:    None
 * Author:    Tobias Blomberg / SM0SVX
 * Created:   2004-05-22
 * Remarks:   
 * Bugs:      
 *----------------------------------------------------------------------------
 */
void ModuleEchoLink::allMsgsWritten(void)
{
  if (outgoing_con_pending != 0)
  {
    outgoing_con_pending->connect();
    broadcastTalkerStatus();
  }
  outgoing_con_pending = 0;
} /* allMsgsWritten */



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
  
    // Get the directory list on first connection to the directory server
  if ((status == StationData::STAT_ONLINE) ||
      (status == StationData::STAT_BUSY))
  {
    if (dir_refresh_timer == 0)
    {
      getDirectoryList();
    }
  }
  else
  {
    delete dir_refresh_timer;
    dir_refresh_timer = 0;
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
  if (pending_connect_id > 0)
  {
    const StationData *station = dir->findStation(pending_connect_id);
    if (station != 0)
    {
      createOutgoingConnection(station);
    }
    else
    {
      playNumber(pending_connect_id);
      playMsg("not_found");
    }
    pending_connect_id = -1;
  }
  
  if (dir->message() != last_message)
  {
    cout << "--- EchoLink directory server message: ---" << endl;
    cout << dir->message() << endl;
    last_message = dir->message();
  }
  
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
  
  if (pending_connect_id > 0)
  {
    playMsg("operation_failed");
    pending_connect_id = -1;
  }
  
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
  
  if (qsos.size() >= max_connections)
  {
    cerr << "*** WARNING: Ignoring incoming connection (too many "
      	    "connections)\n";
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
  QsoImpl *qso = new QsoImpl(station->ip(), this);
  if (!qso->initOk())
  {
    delete qso;
    cerr << "*** Error: Creation of Qso object failed\n";
    return;
  }
  qsos.push_back(qso);
  qso->setRemoteCallsign(callsign);
  qso->setRemoteName(name);
  qso->chatMsgReceived.connect(slot(this, &ModuleEchoLink::onChatMsgReceived));
  qso->isReceiving.connect(slot(this, &ModuleEchoLink::onIsReceiving));
  qso->audioReceived.connect(slot(this, &ModuleEchoLink::audioFromRemote));
  qso->destroyMe.connect(slot(this, &ModuleEchoLink::onDestroyMe));
  
  if (qsos.size() > max_qsos)
  {
    qso->reject();
    return;
  }
  
  if (!isActive())
  {
    if (!activateMe())
    {
      qso->reject();
      cerr << "*** Warning: Could not accept incoming connection from "
      	   << callsign << " since the frontend was busy doing something else.";
      return;
    }
    remote_activation = true;
  }
  qso->accept();
  broadcastTalkerStatus();
  
  setIdle(false);

  //msg_handler->playMsg("EchoLink", "greeting");
  
} /* onIncomingConnection */


/*
 *----------------------------------------------------------------------------
 * Method:    onChatMsgReceived
 * Purpose:   Called by the EchoLink::Qso object when a chat message is
 *    	      received from the remote station.
 * Input:     qso - The QSO object
 *    	      msg - The received message
 * Output:    None
 * Author:    Tobias Blomberg / SM0SVX
 * Created:   2004-05-04
 * Remarks:   
 * Bugs:      
 *----------------------------------------------------------------------------
 */
void ModuleEchoLink::onChatMsgReceived(QsoImpl *qso, const string& msg)
{
  //cout << "--- EchoLink chat message received from " << qso->remoteCallsign()
  //     << " ---" << endl
  //     << msg << endl;
  
  list<QsoImpl*>::iterator it;
  for (it=qsos.begin(); it!=qsos.end(); ++it)
  {
    if (*it != qso)
    {
      (*it)->sendChatData(msg);
    }
  }
} /* onChatMsgReceived */


/*
 *----------------------------------------------------------------------------
 * Method:    onIsReceiving
 * Purpose:   Called by the EchoLink::Qso object to indicate whether the
 *    	      remote station is transmitting or not.
 * Input:     qso     	    - The QSO object
 *    	      is_receiving  - true=remote station is transmitting
 *    	      	      	      false=remote station is not transmitting
 * Output:    None
 * Author:    Tobias Blomberg / SM0SVX
 * Created:   2004-03-07
 * Remarks:   
 * Bugs:      
 *----------------------------------------------------------------------------
 */
void ModuleEchoLink::onIsReceiving(QsoImpl *qso, bool is_receiving)
{
  //cerr << qso->remoteCallsign() << ": EchoLink receiving: "
  //     << (is_receiving ? "TRUE" : "FALSE") << endl;
  
  if ((talker == 0) && is_receiving)
  {
    talker = qso;
    broadcastTalkerStatus();
  }
  
  if (talker == qso)
  {
    if (!is_receiving)
    {
      talker = findFirstTalker();
      if (talker != 0)
      {
      	is_receiving = true;
      }
      broadcastTalkerStatus();
    }
    transmit(is_receiving);
  }
} /* onIsReceiving */


void ModuleEchoLink::onDestroyMe(QsoImpl *qso)
{
  printf("ModuleEchoLink::onDestroyMe\n");
  list<QsoImpl*>::iterator it = find(qsos.begin(), qsos.end(), qso);
  assert (it != qsos.end());
  qsos.erase(it);
  delete qso;
  qso = 0;
  
  broadcastTalkerStatus();

  if (remote_activation && (qsos.size() == 0))
  {
    deactivateMe();
  }
} /* ModuleEchoLink::onDestroyMe */


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
  delete dir_refresh_timer;
  dir_refresh_timer = 0;
  
  if ((dir->status() == StationData::STAT_ONLINE) ||
      (dir->status() == StationData::STAT_BUSY))
  {
    //cout << "Refreshing directory list...\n";
    dir->getCalls();

      /* FIXME: Do we really need periodic updates of the directory list ? */
    dir_refresh_timer = new Timer(600000);
    dir_refresh_timer->expired.connect(
      	    slot(this, &ModuleEchoLink::getDirectoryList));
  }
} /* ModuleEchoLink::getDirectoryList */


void ModuleEchoLink::createOutgoingConnection(const StationData *station)
{
  QsoImpl *qso = new QsoImpl(station->ip(), this);
  if (!qso->initOk())
  {
    delete qso;
    cerr << "Creation of Qso failed\n";
    playMsg("operation_failed");
    return;
  }
  qsos.push_back(qso);
  qso->setRemoteCallsign(station->callsign());
  qso->chatMsgReceived.connect(slot(this, &ModuleEchoLink::onChatMsgReceived));
  qso->isReceiving.connect(slot(this, &ModuleEchoLink::onIsReceiving));
  qso->audioReceived.connect(slot(this, &ModuleEchoLink::audioFromRemote));
  qso->destroyMe.connect(slot(this, &ModuleEchoLink::onDestroyMe));

  playMsg("connecting_to");
  spellCallsign(qso->remoteCallsign());
  playSilence(500);
  outgoing_con_pending = qso;
  
} /* ModuleEchoLink::createOutgoingConnection */


int ModuleEchoLink::audioFromRemote(QsoImpl *qso, short *samples, int count)
{
  if ((qso == talker) && !squelch_is_open)
  {
    audioFromModule(samples, count);
    list<QsoImpl*>::iterator it;
    for (it=qsos.begin(); it!=qsos.end(); ++it)
    {
      // FIXME: Take care of the case where not all samples are written
      if (*it != qso)
      {
	(*it)->sendAudio(samples, count);
      }
    }
  }
    
  return count;
  
} /* ModuleEchoLink::audioFromRemote */


QsoImpl *ModuleEchoLink::findFirstTalker(void) const
{
  list<QsoImpl*>::const_iterator it;
  for (it=qsos.begin(); it!=qsos.end(); ++it)
  {
    if ((*it)->receivingAudio())
    {
      return *it;
    }
  }
  
  return 0;
  
} /* ModuleEchoLink::findFirstTalker */


void ModuleEchoLink::broadcastTalkerStatus(void)
{
  if (qsos.size() < 2)
  {
    return;
  }
  
  stringstream msg;
  msg << "SvxLink " << SVXLINK_VERSION << " - " << mycall
      << " (" << qsos.size() << ")\r\n\r\n";

  if (squelch_is_open)
  {
    msg << "> " << mycall << "         " << sysop_name << "\r\n\r\n";
  }
  else
  {
    if (talker != 0)
    {
      msg << "> " << talker->remoteCallsign() << "         "
      	  << talker->remoteName() << "\r\n\r\n";
    }
    msg << mycall << "         " << sysop_name << "\r\n";
  }
  
  list<QsoImpl*>::const_iterator it;
  for (it=qsos.begin(); it!=qsos.end(); ++it)
  {
    if ((*it != talker) || squelch_is_open)
    {
      msg << (*it)->remoteCallsign() << "         "
      	  << (*it)->remoteName() << "\r\n";
    }
  }
  
  //msg << endl;
  
  for (it=qsos.begin(); it!=qsos.end(); ++it)
  {
    (*it)->sendChatData(msg.str(), false);
  }
  
} /* ModuleEchoLink::broadcastTalkerStatus */




/*
 * This file has not been truncated
 */
