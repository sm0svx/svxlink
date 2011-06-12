/**
@file	 ModuleEchoLink.cpp
@brief   A module that provides EchoLink connection possibility
@author  Tobias Blomberg / SM0SVX
@date	 2004-03-07

\verbatim
A module (plugin) for the multi purpose tranciever frontend system.
Copyright (C) 2004-2011 Tobias Blomberg / SM0SVX

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
#include <cstdlib>
#include <vector>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncTimer.h>
#include <AsyncConfig.h>
#include <AsyncAudioSplitter.h>
#include <AsyncAudioValve.h>
#include <AsyncAudioSelector.h>
#include <EchoLinkDirectory.h>
#include <EchoLinkDispatcher.h>
#include <LocationInfo.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "version/MODULE_ECHOLINK.h"
#include "ModuleEchoLink.h"
#include "QsoImpl.h"


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
    max_connections(1), max_qsos(1), talker(0), squelch_is_open(false),
    state(STATE_NORMAL), cbc_timer(0), drop_incoming_regex(0),
    reject_incoming_regex(0), accept_incoming_regex(0),
    reject_outgoing_regex(0), accept_outgoing_regex(0), splitter(0),
    listen_only_valve(0), selector(0)
{
  cout << "\tModule EchoLink v" MODULE_ECHOLINK_VERSION " starting...\n";
  
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
    cerr << "*** ERROR: Config variable " << cfgName() << "/SERVER not set\n";
    return false;
  }
  
  if (!cfg().getValue(cfgName(), "CALLSIGN", mycall))
  {
    cerr << "*** ERROR: Config variable " << cfgName() << "/CALLSIGN not set\n";
    return false;
  }
  if (mycall == "MYCALL-L")
  {
    cerr << "*** ERROR: Please set the EchoLink callsign (" << cfgName()
      	 << "/CALLSIGN) to a real callsign\n";
    return false;
  }
  
  string password;
  if (!cfg().getValue(cfgName(), "PASSWORD", password))
  {
    cerr << "*** ERROR: Config variable " << cfgName() << "/PASSWORD not set\n";
    return false;
  }
  if (password == "MyPass")
  {
    cerr << "*** ERROR: Please set the EchoLink password (" << cfgName()
      	 << "/PASSWORD) to a real password\n";
    return false;
  }
  
  if (!cfg().getValue(cfgName(), "LOCATION", location))
  {
    cerr << "*** ERROR: Config variable " << cfgName() << "/LOCATION not set\n";
    return false;
  }
  
  if (location.size() > Directory::MAX_DESCRIPTION_SIZE)
  {
    cerr << "*** WARNING: The value of " << cfgName() << "/LOCATION is too "
      	    "long. Maximum length is " << Directory::MAX_DESCRIPTION_SIZE <<
	    " characters.\n";
    location.resize(Directory::MAX_DESCRIPTION_SIZE);
  }
  
  if (!cfg().getValue(cfgName(), "SYSOPNAME", sysop_name))
  {
    cerr << "*** ERROR: Config variable " << cfgName()
      	 << "/SYSOPNAME not set\n";
    return false;
  }
  
  if (!cfg().getValue(cfgName(), "DESCRIPTION", description))
  {
    cerr << "*** ERROR: Config variable " << cfgName()
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
    cerr << "*** ERROR: The value of " << cfgName() << "/MAX_CONNECTIONS ("
      	 << max_connections << ") must be greater or equal to the value of "
	 << cfgName() << "/MAX_QSOS (" << max_qsos << ").\n";
    return false;
  }
  
  cfg().getValue(cfgName(), "ALLOW_IP", allow_ip);
  
  if (!cfg().getValue(cfgName(), "DROP_INCOMING", value))
  {
    value = "^$";
  }
  drop_incoming_regex = new regex_t;
  int err = regcomp(drop_incoming_regex, value.c_str(),
                    REG_EXTENDED | REG_NOSUB | REG_ICASE);
  if (err != 0)
  {
    size_t msg_size = regerror(err, drop_incoming_regex, 0, 0);
    char msg[msg_size];
    size_t err_size = regerror(err, drop_incoming_regex, msg, msg_size);
    assert(err_size == msg_size);
    cerr << "*** ERROR: Syntax error in " << cfgName() << "/DROP_INCOMING: "
         << msg << endl;
    moduleCleanup();
    return false;
  }
  
  if (!cfg().getValue(cfgName(), "REJECT_INCOMING", value))
  {
    value = "^$";
  }
  reject_incoming_regex = new regex_t;
  err = regcomp(reject_incoming_regex, value.c_str(),
                REG_EXTENDED | REG_NOSUB | REG_ICASE);
  if (err != 0)
  {
    size_t msg_size = regerror(err, reject_incoming_regex, 0, 0);
    char msg[msg_size];
    size_t err_size = regerror(err, reject_incoming_regex, msg, msg_size);
    assert(err_size == msg_size);
    cerr << "*** ERROR: Syntax error in " << cfgName() << "/REJECT_INCOMING: "
         << msg << endl;
    moduleCleanup();
    return false;
  }
  
  if (!cfg().getValue(cfgName(), "ACCEPT_INCOMING", value))
  {
    value = "^.*$";
  }
  accept_incoming_regex = new regex_t;
  err = regcomp(accept_incoming_regex, value.c_str(),
                REG_EXTENDED | REG_NOSUB | REG_ICASE);
  if (err != 0)
  {
    size_t msg_size = regerror(err, accept_incoming_regex, 0, 0);
    char msg[msg_size];
    size_t err_size = regerror(err, accept_incoming_regex, msg, msg_size);
    assert(err_size == msg_size);
    cerr << "*** ERROR: Syntax error in " << cfgName() << "/ACCEPT_INCOMING: "
         << msg << endl;
    moduleCleanup();
    return false;
  }

  if (!cfg().getValue(cfgName(), "REJECT_OUTGOING", value))
  {
    value = "^$";
  }
  reject_outgoing_regex = new regex_t;
  err = regcomp(reject_outgoing_regex, value.c_str(),
                REG_EXTENDED | REG_NOSUB | REG_ICASE);
  if (err != 0)
  {
    size_t msg_size = regerror(err, reject_outgoing_regex, 0, 0);
    char msg[msg_size];
    size_t err_size = regerror(err, reject_outgoing_regex, msg, msg_size);
    assert(err_size == msg_size);
    cerr << "*** ERROR: Syntax error in " << cfgName() << "/REJECT_OUTGOING: "
         << msg << endl;
    moduleCleanup();
    return false;
  }

  if (!cfg().getValue(cfgName(), "ACCEPT_OUTGOING", value))
  {
    value = "^.*$";
  }
  accept_outgoing_regex = new regex_t;
  err = regcomp(accept_outgoing_regex, value.c_str(),
                REG_EXTENDED | REG_NOSUB | REG_ICASE);
  if (err != 0)
  {
    size_t msg_size = regerror(err, accept_outgoing_regex, 0, 0);
    char msg[msg_size];
    size_t err_size = regerror(err, accept_outgoing_regex, msg, msg_size);
    assert(err_size == msg_size);
    cerr << "*** ERROR: Syntax error in " << cfgName() << "/ACCEPT_OUTGOING: "
         << msg << endl;
    moduleCleanup();
    return false;
  }
  
    // Initialize directory server communication
  dir = new Directory(server, mycall, password, location);
  dir->statusChanged.connect(slot(*this, &ModuleEchoLink::onStatusChanged));
  dir->stationListUpdated.connect(
      	  slot(*this, &ModuleEchoLink::onStationListUpdated));
  dir->error.connect(slot(*this, &ModuleEchoLink::onError));
  dir->makeOnline();
  
    // Start listening to the EchoLink UDP ports
  if (Dispatcher::instance() == 0)
  {
    cerr << "*** ERROR: Could not create EchoLink listener (Dispatcher) "
      	    "object\n";
    moduleCleanup();
    return false;
  }
  Dispatcher::instance()->incomingConnection.connect(
      slot(*this, &ModuleEchoLink::onIncomingConnection));

    // Create audio pipe chain for audio transmitted to the remote EchoLink
    // stations: <from core> -> Valve -> Splitter (-> QsoImpl ...)
  listen_only_valve = new AudioValve;
  AudioSink::setHandler(listen_only_valve);
  
  splitter = new AudioSplitter;
  listen_only_valve->registerSink(splitter);

    // Create audio pipe chain for audio received from the remove EchoLink
    // stations: (QsoImpl -> ) Selector -> Fifo -> <to core>
  selector = new AudioSelector;
  AudioSource::setHandler(selector);
  
  return true;
  
} /* ModuleEchoLink::initialize */



/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/

void ModuleEchoLink::logicIdleStateChanged(bool is_idle)
{
  /*
  printf("ModuleEchoLink::logicIdleStateChanged: is_idle=%s\n",
      is_idle ? "TRUE" : "FALSE");
  */

  if (qsos.size() > 0)
  {
    list<QsoImpl*>::iterator it;
    for (it=qsos.begin(); it!=qsos.end(); ++it)
    {
      (*it)->logicIdleStateChanged(is_idle);
    }
  }
  
  checkIdle();
  
} /* ModuleEchoLink::logicIdleStateChanged */



/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/


void ModuleEchoLink::moduleCleanup(void)
{
  //FIXME: Delete qso objects
  
  if (accept_incoming_regex != 0)
  {
    regfree(accept_incoming_regex);
    delete accept_incoming_regex;
    accept_incoming_regex = 0;
  }
  if (reject_incoming_regex != 0)
  {
    regfree(reject_incoming_regex);
    delete reject_incoming_regex;
    reject_incoming_regex = 0;
  }
  if (drop_incoming_regex != 0)
  {
    regfree(drop_incoming_regex);
    delete drop_incoming_regex;
    drop_incoming_regex = 0;
  }
  if (accept_outgoing_regex != 0)
  {
    regfree(accept_outgoing_regex);
    delete accept_outgoing_regex;
    accept_outgoing_regex = 0;
  }
  if (reject_outgoing_regex != 0)
  {
    regfree(reject_outgoing_regex);
    delete reject_outgoing_regex;
    reject_outgoing_regex = 0;
  }
  
  delete dir_refresh_timer;
  dir_refresh_timer = 0;
  delete Dispatcher::instance();
  delete dir;
  dir = 0;
  delete cbc_timer;
  cbc_timer = 0;
  state = STATE_NORMAL;
  
  AudioSink::clearHandler();
  delete splitter;
  splitter = 0;
  delete listen_only_valve;
  listen_only_valve = 0;
  
  AudioSource::clearHandler();
  delete selector;
  selector = 0;
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
  updateEventVariables();
  state = STATE_NORMAL;
  listen_only_valve->setOpen(true);
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
  list<QsoImpl*> qsos_tmp(qsos);
  list<QsoImpl*>::iterator it;
  for (it=qsos_tmp.begin(); it!=qsos_tmp.end(); ++it)
  {
    if ((*it)->currentState() != Qso::STATE_DISCONNECTED)
    {
      (*it)->disconnect();
    }
  }

  outgoing_con_pending.clear();

  remote_activation = false;
  delete cbc_timer;
  cbc_timer = 0;
  state = STATE_NORMAL;
  listen_only_valve->setOpen(false);
} /* deactivateCleanup */


/*
 *----------------------------------------------------------------------------
 * Method:    dtmfDigitReceived
 * Purpose:   Called by the core system when a DTMF digit has been
 *    	      received.
 * Input:     digit   	- The DTMF digit received (0-9, A-D, *, #)
 *            duration	- The length in milliseconds of the received digit
 * Output:    Return true if the digit is handled or false if not
 * Author:    Tobias Blomberg / SM0SVX
 * Created:   2004-03-07
 * Remarks:   
 * Bugs:      
 *----------------------------------------------------------------------------
 */
#if 0
bool ModuleEchoLink::dtmfDigitReceived(char digit, int duration)
{
  //cout << "DTMF digit received in module " << name() << ": " << digit << endl;
  
  return false;
  
} /* dtmfDigitReceived */
#endif


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
  cout << "DTMF command received in module " << name() << ": " << cmd << endl;
  
  remote_activation = false;
  
  if (state == STATE_CONNECT_BY_CALL)
  {
    handleConnectByCall(cmd);
    return;
  }
  
  if (cmd.size() == 0)      	    // Disconnect node or deactivate module
  {
    if ((qsos.size() != 0) &&
      	(qsos.back()->currentState() != Qso::STATE_DISCONNECTED))
    {
      qsos.back()->disconnect();
    }
    else if (outgoing_con_pending.empty())
    {
      deactivateMe();
    }
  }
  /*
  else if (cmd[0] == '*')   // Connect by callsign
  {
    connectByCallsign(cmd);
  }
  */
  else if ((cmd.size() < 4) || (cmd[1] == '*'))  // Dispatch to command handling
  {
    handleCommand(cmd);
  }
  else
  {
    connectByNodeId(atoi(cmd.c_str()));
  }
} /* dtmfCmdReceived */


void ModuleEchoLink::dtmfCmdReceivedWhenIdle(const std::string &cmd)
{
  if (cmd == "2")   // Play own node id
  {
    stringstream ss;
    ss << "play_node_id ";
    const StationData *station = dir->findCall(dir->callsign());
    ss << (station ? station->id() : 0);
    processEvent(ss.str());
  }
  else
  {
    commandFailed(cmd);
  }
} /* dtmfCmdReceivedWhenIdle */


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
  broadcastTalkerStatus();  
} /* squelchOpen */


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
  if (!outgoing_con_pending.empty())
  {
    list<QsoImpl*>::iterator it;
    for (it=outgoing_con_pending.begin(); it!=outgoing_con_pending.end(); ++it)
    {
      (*it)->connect();
    }
    //outgoing_con_pending->connect();
    updateDescription();
    broadcastTalkerStatus();
    outgoing_con_pending.clear();
  }
} /* allMsgsWritten */


/*
 *----------------------------------------------------------------------------
 * Method:    reportState
 * Purpose:   Called by the core system when the state of the module should
 *    	      be reported.
 * Input:     None
 * Output:    None
 * Author:    Tobias Blomberg / SM0SVX
 * Created:   2005-02-12
 * Remarks:   FIXME: Is this function needed?? Status reports are handled in
 *    	      TCL only I think.
 * Bugs:      
 *----------------------------------------------------------------------------
 */
void ModuleEchoLink::reportState(void)
{
  stringstream ss;
  ss << "status_report " << numConnectedStations();
  processEvent(ss.str());
} /* reportState */


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
  
    // Update status at aprs.echolink.org
  if (LocationInfo::has_instance())
  {
    LocationInfo::instance()->updateDirectoryStatus(status);
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
      createOutgoingConnection(*station);
    }
    else
    {
      cout << "The EchoLink ID " << pending_connect_id
      	   << " could not be found.\n";
      stringstream ss;
      ss << "station_id_not_found " << pending_connect_id;
      processEvent(ss.str());
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
  cerr << "*** ERROR: " << msg << endl;
  
  if (pending_connect_id > 0)
  {
    stringstream ss;
    ss << "lookup_failed " << pending_connect_id;
    processEvent(ss.str());
  }
  
} /* onError */



/*
 *----------------------------------------------------------------------------
 * Method:    onIncomingConnection
 * Purpose:   Called by the EchoLink::Dispatcher object when a new remote
 *    	      connection is coming in.
 * Input:     callsign	- The callsign of the remote station
 *    	      name    	- The name of the remote station
 *            priv      - A private string for passing connection parameters
 * Output:    None
 * Author:    Tobias Blomberg / SM0SVX
 * Created:   2004-03-07
 * Remarks:   
 * Bugs:      
 *----------------------------------------------------------------------------
 */
void ModuleEchoLink::onIncomingConnection(const IpAddress& ip,
      	      	      	      	      	  const string& callsign,
      	      	      	      	      	  const string& name,
      	      	      	      	      	  const string& priv)
{
  cout << "Incoming EchoLink connection from " << callsign
       << " (" << name << ") at " << ip << "\n";
  
  if (regexec(drop_incoming_regex, callsign.c_str(), 0, 0, 0) == 0)
  {
    cerr << "*** WARNING: Dropping incoming connection due to configuration.\n";
    return;
  }
  
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
  
  if (station->ip() != ip)
  {
    cerr << "*** WARNING: Ignoring incoming connection from " << callsign
      	 << " since the IP address registered in the directory server "
	 << "(" << station->ip() << ") is not the same as the remote IP "
	 << "address (" << ip << ") of the incoming connection\n";
    return;
  }

    // Create a new Qso object to accept the connection
  QsoImpl *qso = new QsoImpl(*station, this);
  if (!qso->initOk())
  {
    delete qso;
    cerr << "*** ERROR: Creation of Qso object failed\n";
    return;
  }
  qsos.push_back(qso);
  updateEventVariables();
  qso->setRemoteCallsign(callsign);
  qso->setRemoteName(name);
  qso->setRemoteParams(priv);
  qso->stateChange.connect(slot(*this, &ModuleEchoLink::onStateChange));
  qso->chatMsgReceived.connect(slot(*this, &ModuleEchoLink::onChatMsgReceived));
  qso->isReceiving.connect(slot(*this, &ModuleEchoLink::onIsReceiving));
  qso->audioReceivedRaw.connect(
      	  slot(*this, &ModuleEchoLink::audioFromRemoteRaw));
  qso->destroyMe.connect(slot(*this, &ModuleEchoLink::destroyQsoObject));

  splitter->addSink(qso);
  selector->addSource(qso);
  selector->enableAutoSelect(qso, 0);

  if (qsos.size() > max_qsos)
  {
    qso->reject(false);
    return;
  }
  
  if ((regexec(reject_incoming_regex, callsign.c_str(), 0, 0, 0) == 0) ||
      (regexec(accept_incoming_regex, callsign.c_str(), 0, 0, 0) != 0))
  {
    qso->reject(true);
    return;
  }
  
  if (!isActive())
  {
    remote_activation = true;
  }
  
  if (!activateMe())
  {
    qso->reject(false);
    cerr << "*** WARNING: Could not accept incoming connection from "
      	 << callsign
	 << " since the frontend was busy doing something else.\n";
    return;
  }
  
  qso->accept();
  broadcastTalkerStatus();
  updateDescription();

  if (LocationInfo::has_instance())
  {
    list<string> call_list;
    listQsoCallsigns(call_list);
    
    LocationInfo::instance()->updateQsoStatus(2, callsign, name, call_list);
  }
  
  checkIdle();
  
} /* onIncomingConnection */


/*
 *----------------------------------------------------------------------------
 * Method:    onStateChange
 * Purpose:   Called by the EchoLink::QsoImpl object when a state change has
 *    	      occured on the connection.
 * Input:     qso     	- The QSO object
 *    	      qso_state - The new QSO connection state
 * Output:    None
 * Author:    Tobias Blomberg / SM0SVX
 * Created:   2006-03-12
 * Remarks:   
 * Bugs:      
 *----------------------------------------------------------------------------
 */
void ModuleEchoLink::onStateChange(QsoImpl *qso, Qso::State qso_state)
{
  switch (qso_state)
  {
    case Qso::STATE_DISCONNECTED:
    {
      list<QsoImpl*>::iterator it = find(qsos.begin(), qsos.end(), qso);
      assert (it != qsos.end());
      qsos.erase(it);
      qsos.push_front(qso);
      updateEventVariables();
      
      last_disc_stn = qso->stationData();
      
      if (remote_activation &&
      	  (qsos.back()->currentState() == Qso::STATE_DISCONNECTED))
      {
      	deactivateMe();
      }

      broadcastTalkerStatus();
      updateDescription();
      break;
    }
    
    default:
      updateEventVariables();
      break;
  }  
} /* ModuleEchoLink::onStateChange */


/*
 *----------------------------------------------------------------------------
 * Method:    onChatMsgReceived
 * Purpose:   Called by the EchoLink::Qso object when a chat message has been
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
void ModuleEchoLink::onIsReceiving(bool is_receiving, QsoImpl *qso)
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
  }
} /* onIsReceiving */


void ModuleEchoLink::destroyQsoObject(QsoImpl *qso)
{
  //cout << qso->remoteCallsign() << ": Destroying QSO object" << endl;
  string callsign = qso->remoteCallsign();

  splitter->removeSink(qso);
  selector->removeSource(qso);
      
  list<QsoImpl*>::iterator it = find(qsos.begin(), qsos.end(), qso);
  assert (it != qsos.end());
  qsos.erase(it);

  updateEventVariables();
  delete qso;
  
  if (talker == qso)
  {
    talker = findFirstTalker();
  }

  it = find(outgoing_con_pending.begin(), outgoing_con_pending.end(), qso);
  if (it != outgoing_con_pending.end())
  {
    outgoing_con_pending.erase(it);
  }

  qso = 0;
  
  //broadcastTalkerStatus();
  //updateDescription();

  if (LocationInfo::has_instance())
  {
    list<string> call_list;
    listQsoCallsigns(call_list);
    
    LocationInfo::instance()->updateQsoStatus(0, callsign, "", call_list);
  }

  checkIdle();
  
} /* ModuleEchoLink::destroyQsoObject */


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
    dir->getCalls();

      /* FIXME: Do we really need periodic updates of the directory list ? */
    dir_refresh_timer = new Timer(600000);
    dir_refresh_timer->expired.connect(
      	    slot(*this, &ModuleEchoLink::getDirectoryList));
  }
} /* ModuleEchoLink::getDirectoryList */


void ModuleEchoLink::createOutgoingConnection(const StationData &station)
{
  if (station.callsign() == mycall)
  {
    cerr << "Cannot connect to myself (" << mycall << "/" << station.id()
      	 << ")...\n";
    processEvent("self_connect");
    return;
  }

  if ((regexec(reject_outgoing_regex, station.callsign().c_str(),
	       0, 0, 0) == 0) ||
      (regexec(accept_outgoing_regex, station.callsign().c_str(),
	       0, 0, 0) != 0))
  {
    cerr << "Rejecting outgoing connection to " << station.callsign() << " ("
	 << station.id() << ")\n";
    stringstream ss;
    ss << "reject_outgoing_connection " << station.callsign();
    processEvent(ss.str());
    return;
  }

  cout << "Connecting to " << station.callsign() << " (" << station.id()
       << ")\n";
  
  QsoImpl *qso = 0;
  
  list<QsoImpl*>::iterator it;
  for (it=qsos.begin(); it!=qsos.end(); ++it)
  {
    if ((*it)->remoteCallsign() == station.callsign())
    {
      if ((*it)->currentState() != Qso::STATE_DISCONNECTED)
      {
	cerr << "*** WARNING: Already connected to " << station.callsign()
      	     << ". Ignoring connect request.\n";
	stringstream ss;
	ss << "already_connected_to " << station.callsign();
	processEvent(ss.str());
	return;
      }
      qsos.erase(it);
      qso = *it;
      qsos.push_back(qso);
      break;
    }
  }

  if (qso == 0)
  {
    qso = new QsoImpl(station, this);
    if (!qso->initOk())
    {
      delete qso;
      cerr << "*** ERROR: Creation of Qso failed\n";
      processEvent("internal_error");
      return;
    }
    qsos.push_back(qso);
    updateEventVariables();    
    qso->setRemoteCallsign(station.callsign());
    qso->stateChange.connect(slot(*this, &ModuleEchoLink::onStateChange));
    qso->chatMsgReceived.connect(slot(*this, &ModuleEchoLink::onChatMsgReceived));
    qso->isReceiving.connect(slot(*this, &ModuleEchoLink::onIsReceiving));
    qso->audioReceivedRaw.connect(
      	    slot(*this, &ModuleEchoLink::audioFromRemoteRaw));
    qso->destroyMe.connect(slot(*this, &ModuleEchoLink::destroyQsoObject));

    splitter->addSink(qso);
    selector->addSource(qso);
    selector->enableAutoSelect(qso, 0);
  }
    
  stringstream ss;
  ss << "connecting_to " << qso->remoteCallsign();
  processEvent(ss.str());
  outgoing_con_pending.push_back(qso);
  
  if (LocationInfo::has_instance())
  {
    stringstream info;
    info << station.id();

    list<string> call_list;
    listQsoCallsigns(call_list);

    LocationInfo::instance()->updateQsoStatus(1, station.callsign(), info.str(), call_list);
  }
    
  checkIdle();
  
} /* ModuleEchoLink::createOutgoingConnection */


void ModuleEchoLink::audioFromRemoteRaw(Qso::RawPacket *packet,
      	QsoImpl *qso)
{
  if (!listen_only_valve->isOpen())
  {
    return;
  }

  if ((qso == talker) && !squelch_is_open)
  {
    list<QsoImpl*>::iterator it;
    for (it=qsos.begin(); it!=qsos.end(); ++it)
    {
      if (*it != qso)
      {
	(*it)->sendAudioRaw(packet);
      }
    }
  }
} /* ModuleEchoLink::audioFromRemoteRaw */


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
  if (max_qsos < 2)
  {
    return;
  }
  
  stringstream msg;
  msg << "SvxLink " << SVXLINK_VERSION << " - " << mycall
      << " (" << numConnectedStations() << ")\n\n";

  if (squelch_is_open)
  {
    msg << "> " << mycall << "         " << sysop_name << "\n\n";
  }
  else
  {
    if (talker != 0)
    {
      msg << "> " << talker->remoteCallsign() << "         "
      	  << talker->remoteName() << "\n\n";
    }
    msg << mycall << "         " << sysop_name << "\n";
  }
  
  list<QsoImpl*>::const_iterator it;
  for (it=qsos.begin(); it!=qsos.end(); ++it)
  {
    if ((*it)->currentState() == Qso::STATE_DISCONNECTED)
    {
      continue;
    }
    if ((*it != talker) || squelch_is_open)
    {
      msg << (*it)->remoteCallsign() << "         "
      	  << (*it)->remoteName() << "\n";
    }
  }
  
  for (it=qsos.begin(); it!=qsos.end(); ++it)
  {
    (*it)->sendInfoData(msg.str());
  }
  
} /* ModuleEchoLink::broadcastTalkerStatus */


void ModuleEchoLink::updateDescription(void)
{
  if (max_qsos < 2)
  {
    return;
  }
  
  string desc(location);
  if (numConnectedStations() > 0)
  {
    stringstream sstr;
    sstr << " (" << numConnectedStations() << ")";
    desc.resize(Directory::MAX_DESCRIPTION_SIZE - sstr.str().size(), ' ');
    desc += sstr.str();
  }
    
  dir->setDescription(desc);
  dir->refreshRegistration();
  
} /* ModuleEchoLink::updateDescription */


void ModuleEchoLink::updateEventVariables(void)
{
  stringstream ss;
  ss << numConnectedStations();
  string var_name(name());
  var_name +=  "::num_connected_stations";
  setEventVariable(var_name, ss.str());
} /* ModuleEchoLink::updateEventVariables */


void ModuleEchoLink::connectByCallsign(string cmd)
{
  stringstream ss;

  if (cmd.length() < 5)
  {
    ss << "cbc_too_short_cmd " << cmd;
    processEvent(ss.str());
    return;
  }

  string code;
  bool exact;
  if (cmd[cmd.size()-1] == '*')
  {
    code = cmd.substr(2, cmd.size() - 3);
    exact = false;
  }
  else
  {
    code = cmd.substr(2);
    exact = true;
  }

  cout << "Looking up callsign code: " << code << " "
       << (exact ? "(exact match)" : "(wildcard match)") << endl;
  dir->findStationsByCode(cbc_stns, code, exact);
  cout << "Found " << cbc_stns.size() << " stations:\n";
  StnList::const_iterator it;
  int cnt = 0;
  for (it = cbc_stns.begin(); it != cbc_stns.end(); ++it)
  {
    cout << *it << endl;
    if (++cnt >= 9)
    {
      break;
    }
  }

  if (cbc_stns.size() == 0)
  {
    ss << "cbc_no_match " << code;
    processEvent(ss.str());
    return;
  }

  if (cbc_stns.size() > 9)
  {
    cout << "Too many matches. The search must be narrowed down.\n";
    processEvent("cbc_too_many_matches");
    return;
  }

  ss << "cbc_list [list";
  for (it = cbc_stns.begin(); it != cbc_stns.end(); ++it)
  {
    ss << " " << (*it).callsign();
  }
  ss << "]";
  processEvent(ss.str());
  state = STATE_CONNECT_BY_CALL;
  delete cbc_timer;
  cbc_timer = new Timer(60000);
  cbc_timer->expired.connect(slot(*this, &ModuleEchoLink::cbcTimeout));

} /* ModuleEchoLink::connectByCallsign */


void ModuleEchoLink::handleConnectByCall(const string& cmd)
{
  if (cmd.empty())
  {
    processEvent("cbc_aborted");
    cbc_stns.clear();
    delete cbc_timer;
    cbc_timer = 0;
    state = STATE_NORMAL;
    return;
  }
  
  unsigned idx = static_cast<unsigned>(atoi(cmd.c_str()));
  stringstream ss;

  if (idx == 0)
  {
    ss << "cbc_list [list";
    StnList::const_iterator it;
    for (it = cbc_stns.begin(); it != cbc_stns.end(); ++it)
    {
      ss << " " << (*it).callsign();
    }
    ss << "]";
    processEvent(ss.str());
    cbc_timer->reset();
    return;
  }

  if (idx > cbc_stns.size())
  {
    ss << "cbc_index_out_of_range " << idx;
    processEvent(ss.str());
    cbc_timer->reset();
    return;
  }

  createOutgoingConnection(cbc_stns[idx-1]);
  cbc_stns.clear();
  delete cbc_timer;
  cbc_timer = 0;
  state = STATE_NORMAL;
} /* ModuleEchoLink::handleConnectByCall  */


void ModuleEchoLink::cbcTimeout(Timer *t)
{
  delete cbc_timer;
  cbc_timer = 0;
  cbc_stns.clear();
  state = STATE_NORMAL;
  cout << "Connect by call command timeout\n";
  processEvent("cbc_timeout");
} /* ModuleEchoLink::cbcTimeout  */


int ModuleEchoLink::numConnectedStations(void)
{
  int cnt = 0;
  list<QsoImpl*>::iterator it;
  for (it=qsos.begin(); it!=qsos.end(); ++it)
  {
    if ((*it)->currentState() != Qso::STATE_DISCONNECTED)
    {
      ++cnt;
    }
  }
  
  return cnt;
  
} /* ModuleEchoLink::numConnectedStations */


int ModuleEchoLink::listQsoCallsigns(list<string>& call_list)
{
  call_list.clear();
  list<QsoImpl*>::iterator it;
  for (it=qsos.begin(); it!=qsos.end(); ++it)
  {
    call_list.push_back((*it)->remoteCallsign());
  }
  
  return call_list.size();
  
} /* ModuleEchoLink::listQsoCallsigns */


void ModuleEchoLink::handleCommand(const string& cmd)
{
  if (cmd[0] == '0')	    // Help
  {
    playHelpMsg();
  }
  else if (cmd[0] == '1')   // Connection status
  {
    if (cmd.size() != 1)
    {
      commandFailed(cmd);
      return;
    }
    
    stringstream ss;
    ss << "list_connected_stations [list";
    list<QsoImpl*>::iterator it;
    for (it=qsos.begin(); it!=qsos.end(); ++it)
    {
      if ((*it)->currentState() != Qso::STATE_DISCONNECTED)
      {
      	ss << " " << (*it)->remoteCallsign();
      }
    }
    ss << "]";
    processEvent(ss.str());
  }
  else if (cmd[0] == '2')   // Play own node id
  {
    if (cmd.size() != 1)
    {
      commandFailed(cmd);
      return;
    }
    
    stringstream ss;
    ss << "play_node_id ";
    const StationData *station = dir->findCall(dir->callsign());
    ss << (station ? station->id() : 0);
    processEvent(ss.str());
  }
  else if (cmd[0] == '3')   // Random connect
  {
    stringstream ss;
    
    if (cmd.size() != 2)
    {
      commandFailed(cmd);
      return;
    }
    
    vector<StationData> nodes;
    
    if (cmd[1] == '1')	// Random connect to link or repeater
    {
      const list<StationData>& links = dir->links();
      const list<StationData>& repeaters = dir->repeaters();
      list<StationData>::const_iterator it;
      for (it=links.begin(); it!=links.end(); it++)
      {
	nodes.push_back(*it);
      }
      for (it=repeaters.begin(); it!=repeaters.end(); it++)
      {
	nodes.push_back(*it);
      }
    }
    else if (cmd[1] == '2') // Random connect to conference
    {
      const list<StationData>& conferences = dir->conferences();
      list<StationData>::const_iterator it;
      for (it=conferences.begin(); it!=conferences.end(); it++)
      {
	nodes.push_back(*it);
      }
    }
    else
    {
      commandFailed(cmd);
      return;
    }

    double count = nodes.size();
    if (count > 0)
    {
      srand(time(NULL));
      size_t random_idx = (size_t)(count * ((double)rand() / (1.0 + RAND_MAX)));
      StationData station = nodes[random_idx];
      
      cout << "Creating random connection to node:\n";
      cout << station << endl;
      
      createOutgoingConnection(station);
    }
    else
    {
      commandFailed(cmd);
      return;
    }
  }
  else if (cmd[0] == '4')   // Reconnect to the last disconnected station
  {
    if ((cmd.size() != 1) || last_disc_stn.callsign().empty())
    {
      commandFailed(cmd);
      return;
    }
    
    cout << "Trying to reconnect to " << last_disc_stn.callsign() << endl;
    connectByNodeId(last_disc_stn.id());
  }
  else if (cmd[0] == '5')   // Listen only
  {
    if (cmd.size() < 2)
    {
      commandFailed(cmd);
      return;
    }
    
    bool activate = (cmd[1] != '0');
    
    stringstream ss;
    ss << "listen_only " << (!listen_only_valve->isOpen() ? "1 " : "0 ")
       << (activate ? "1" : "0");
    processEvent(ss.str());
    
    listen_only_valve->setOpen(!activate);
  }
  else if (cmd[0] == '6')   // Connect by callsign
  {
    connectByCallsign(cmd);
  }
  else
  {
    stringstream ss;
    ss << "unknown_command " << cmd;
    processEvent(ss.str());
  }

} /* ModuleEchoLink::handleCommand */


void ModuleEchoLink::commandFailed(const string& cmd)
{
  stringstream ss;
  ss << "command_failed " << cmd;
  processEvent(ss.str());
} /* ModuleEchoLink::commandFailed */


void ModuleEchoLink::connectByNodeId(int node_id)
{
  if (qsos.size() < max_qsos)
  {
    if ((dir->status() == StationData::STAT_OFFLINE) ||
      	(dir->status() == StationData::STAT_UNKNOWN))
    {
      cout << "*** ERROR: Directory server offline (status="
      	   << dir->statusStr() << "). Can't create outgoing connection.\n";
      processEvent("directory_server_offline");
      return;
    }
    
    const StationData *station = dir->findStation(node_id);
    if (station != 0)
    {
      createOutgoingConnection(*station);
    }
    else
    {
      cout << "EchoLink ID " << node_id << " is not in the list. "
      	      "Refreshing the list...\n";
      getDirectoryList();
      pending_connect_id = node_id;
    }
  }
  else
  {
    processEvent("no_more_connections_allowed");
  }
} /* ModuleEchoLink::connectByNodeId */


void ModuleEchoLink::checkIdle(void)
{
  setIdle(qsos.empty() &&
      	  logicIsIdle() &&
	  (state == STATE_NORMAL));
} /* ModuleEchoLink::checkIdle */


/*
 * This file has not been truncated
 */
