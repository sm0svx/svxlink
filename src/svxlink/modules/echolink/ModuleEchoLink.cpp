/**
@file	 ModuleEchoLink.cpp
@brief   A module that provides EchoLink connection possibility
@author  Tobias Blomberg / SM0SVX
@date	 2004-03-07

\verbatim
A module (plugin) for the multi purpose tranciever frontend system.
Copyright (C) 2004-2014 Tobias Blomberg / SM0SVX

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
#include <time.h>

#include <algorithm>
#include <cassert>
#include <sstream>
#include <cstdlib>
#include <vector>

#include <string.h>

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
#include <EchoLinkProxy.h>
#include <LocationInfo.h>
#include <common.h>


#include <AsyncPty.h>
#include <AsyncPtyStreamBuf.h>

/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "version/MODULE_ECHO_LINK.h"
#include "ModuleEchoLink.h"
#include "QsoImpl.h"


/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/

using namespace std;
using namespace sigc;
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

namespace {
  const char* CFG_DROP_INCOMING   = "DROP_INCOMING";
  const char* CFG_REJECT_INCOMING = "REJECT_INCOMING";
  const char* CFG_ACCEPT_INCOMING = "ACCEPT_INCOMING";
  const char* CFG_REJECT_OUTGOING = "REJECT_OUTGOING";
  const char* CFG_ACCEPT_OUTGOING = "ACCEPT_OUTGOING";
};


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
    state(STATE_NORMAL), cbc_timer(0), dbc_timer(0), drop_incoming_regex(0),
    reject_incoming_regex(0), accept_incoming_regex(0),
    reject_outgoing_regex(0), accept_outgoing_regex(0), splitter(0),
    listen_only_valve(0), selector(0), num_con_max(0), num_con_ttl(5*60),
    num_con_block_time(120*60), num_con_update_timer(0), reject_conf(false),
    autocon_echolink_id(0), autocon_time(DEFAULT_AUTOCON_TIME),
    autocon_timer(0), proxy(0), pty(0)
{
  cout << "\tModule EchoLink v" MODULE_ECHO_LINK_VERSION " starting...\n";
  
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
  
  vector<string> servers;
  if (!cfg().getValue(cfgName(), "SERVERS", servers))
  {
    string server;
    if (cfg().getValue(cfgName(), "SERVER", server))
    {
      cerr << "*** WARNING: Config variable " << cfgName()
           << "/SERVER is deprecated. Use SERVERS instead.\n";
      servers.push_back(server);
    }
  }
  if (servers.empty())
  {
    cerr << "*** ERROR: Config variable " << cfgName() << "/SERVERS not set "
            "or empty\n";
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

  if (!setDropIncomingRegex())
  {
    moduleCleanup();
    return false;
  }

    // To reduce the number of senseless connects
  if (cfg().getValue(cfgName(), "CHECK_NR_CONNECTS", value))
  {
    vector<unsigned> params;
    if (SvxLink::splitStr(params, value, ",") != 3)
    {
       cerr << "*** ERROR: Syntax error in " << cfgName()
            << "/CHECK_NR_CONNECTS\n"
            << "Example: CHECK_NR_CONNECTS=3,300,60 where\n"
            << "  3   = max number of connects\n"
            << "  300 = time in seconds that a connection is remembered\n"
            << "  60  = time in minutes that the party is blocked\n";
       return false;
    }
    num_con_max = params[0];
    num_con_ttl = params[1];
    num_con_block_time = params[2] * 60;
  }

  if (!setRejectIncomingRegex() ||
      !setAcceptIncomingRegex() ||
      !setRejectOutgoingRegex() ||
      !setAcceptOutgoingRegex())
  {
    moduleCleanup();
    return false;
  }

  cfg().getValue(cfgName(), "REJECT_CONF", reject_conf);
  cfg().getValue(cfgName(), "AUTOCON_ECHOLINK_ID", autocon_echolink_id);
  int autocon_time_secs = autocon_time / 1000;
  cfg().getValue(cfgName(), "AUTOCON_TIME", autocon_time_secs);
  autocon_time = 1000 * max(autocon_time_secs, 5); // At least five seconds

  string proxy_server;
  cfg().getValue(cfgName(), "PROXY_SERVER", proxy_server);
  uint16_t proxy_port = 8100;
  cfg().getValue(cfgName(), "PROXY_PORT", proxy_port);
  string proxy_password;
  cfg().getValue(cfgName(), "PROXY_PASSWORD", proxy_password);
  
  if (!proxy_server.empty())
  {
    proxy = new Proxy(proxy_server, proxy_port, mycall, proxy_password);
    proxy->connect();
  }

  IpAddress bind_addr;
  if (cfg().getValue(cfgName(), "BIND_ADDR", bind_addr) && bind_addr.isEmpty())
  {
    cerr << "*** ERROR: Invalid configuration value for " << cfgName()
         << "/BIND_ADDR specified.\n";
    moduleCleanup();
    return false;
  }

    // Initialize directory server communication
  dir = new Directory(servers, mycall, password, location, bind_addr);
  dir->statusChanged.connect(mem_fun(*this, &ModuleEchoLink::onStatusChanged));
  dir->stationListUpdated.connect(
      	  mem_fun(*this, &ModuleEchoLink::onStationListUpdated));
  dir->error.connect(mem_fun(*this, &ModuleEchoLink::onError));
  dir->makeOnline();
  
    // Start listening to the EchoLink UDP ports
  Dispatcher::setBindAddr(bind_addr);
  if (Dispatcher::instance() == 0)
  {
    cerr << "*** ERROR: Could not create EchoLink listener (Dispatcher) "
      	    "object\n";
    moduleCleanup();
    return false;
  }
  bool drop_all_incoming = false;
  if (!cfg().getValue(cfgName(), "DROP_ALL_INCOMING", drop_all_incoming) ||
      !drop_all_incoming)
  {
    Dispatcher::instance()->incomingConnection.connect(
        mem_fun(*this, &ModuleEchoLink::onIncomingConnection));
  }

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
  
    // Periodic updates of the "watch num connects" list
  if (num_con_max > 0)
  {
    num_con_update_timer = new Timer(6000000); // One hour
    num_con_update_timer->expired.connect(sigc::hide(
        mem_fun(*this, &ModuleEchoLink::numConUpdate)));
  }

  if (autocon_echolink_id > 0)
  {
      // Initially set the timer to 15 seconds for quick activation on statup
    autocon_timer = new Timer(15000, Timer::TYPE_PERIODIC);
    autocon_timer->expired.connect(
        mem_fun(*this, &ModuleEchoLink::checkAutoCon));
  }

  string pty_path;
  if(cfg().getValue(cfgName(), "COMMAND_PTY", pty_path))
  {
    pty = new Pty(pty_path);
    if (!pty->open())
    {
      cerr << "*** ERROR: Could not open echolink PTY "
           << pty_path << " as specified in configuration variable "
           << name() << "/" << "COMMAND_PTY" << endl;
      return false;
    }
    pty->dataReceived.connect(
        sigc::mem_fun(*this, &ModuleEchoLink::onCommandPtyInput));
  }

  cfg().valueUpdated.connect(
      sigc::mem_fun(*this, &ModuleEchoLink::cfgValueUpdated));

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
    vector<QsoImpl*>::iterator it;
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

void ModuleEchoLink::handlePtyCommand(const std::string &full_command)
{
  istringstream is(full_command);
  string command;
  if (!(is >> command))
  {
    return;
  }

  if (command == "KILL") // Disconnect active talker
  {
    if (talker == 0)
    {
      cout << "EchoLink: Trying to KILL, but no active talker" << endl;
    }
    else
    {
      cout << "EchoLink: Killing talker: " << talker->remoteCallsign() << endl;
      talker->disconnect();
    }
  }
  else if (command == "DISC") // Disconnect client by callsign
  {
    string callsign;
    if (!(is >> callsign))
    {
      cerr << "*** WARNING: Malformed EchoLink PTY disconnect command: \""
           << full_command << "\"" << endl;
      return;
    }
    vector<QsoImpl *>::iterator it;
    for (it = qsos.begin(); it != qsos.end(); ++it)
    {
      if ((*it)->remoteCallsign() == callsign)
      {
        cout << "EchoLink: Disconnecting user "
             << (*it)->remoteCallsign() << endl;
        (*it)->disconnect();
        return;
      }
    }
    cerr << "*** WARNING: Could not find EchoLink user \"" << callsign
         << "\" in PTY command \"DISC\"" << endl;
  }
  else
  {
    cerr << "*** WARNING: Unknown EchoLink PTY command received: \""
         << full_command << "\"" << endl;
  }
} /* ModuleEchoLink::handlePtyCommand */


void ModuleEchoLink::onCommandPtyInput(const void *buf, size_t count)
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
} /* ModuleEchoLink::onCommandPtyInput */


void ModuleEchoLink::moduleCleanup(void)
{
  //FIXME: Delete qso objects
  
  delete num_con_update_timer;
  num_con_update_timer = 0;

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
  Dispatcher::deleteInstance();
  delete dir;
  dir = 0;
  delete proxy;
  proxy = 0;
  delete cbc_timer;
  cbc_timer = 0;
  delete dbc_timer;
  dbc_timer = 0;
  state = STATE_NORMAL;
  delete autocon_timer;
  autocon_timer = 0;
  
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
  vector<QsoImpl*> qsos_tmp(qsos);
  vector<QsoImpl*>::iterator it;
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
  delete dbc_timer;
  dbc_timer = 0;
  state = STATE_NORMAL;
  listen_only_valve->setOpen(true);
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
  
  if (state == STATE_DISCONNECT_BY_CALL)
  {
    handleDisconnectByCall(cmd);
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
  if (listen_only_valve->isOpen())
  {
    broadcastTalkerStatus();  
  }

  for (vector<QsoImpl*>::iterator it=qsos.begin(); it!=qsos.end(); ++it)
  {
    (*it)->squelchOpen(is_open);
  }
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
    vector<QsoImpl*>::iterator it;
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
 * Method:    clientListChanged
 * Purpose:   Called on connect or disconnect of a remote client to send an
 *    	      event to list the connected stations.
 * Input:     None
 * Output:    None
 * Author:    Wim Fournier / PH7WIM
 * Created:   2016-01-11
 * Remarks:
 * Bugs:
 *----------------------------------------------------------------------------
 */
void ModuleEchoLink::clientListChanged(void)
{
  stringstream ss;
  ss << "client_list_changed [list";
  for (vector<QsoImpl *>::iterator it = qsos.begin(); it != qsos.end(); ++it)
  {
    if ((*it)->currentState() != Qso::STATE_DISCONNECTED)
    {
      ss << " " << (*it)->remoteCallsign();
    }
  }
  ss << "]";
  processEvent(ss.str());
} /* clientListChanged */


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
    getDirectoryList();
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
  qso->setListenOnly(!listen_only_valve->isOpen());
  qso->stateChange.connect(mem_fun(*this, &ModuleEchoLink::onStateChange));
  qso->chatMsgReceived.connect(
          mem_fun(*this, &ModuleEchoLink::onChatMsgReceived));
  qso->infoMsgReceived.connect(
          mem_fun(*this, &ModuleEchoLink::onInfoMsgReceived));
  qso->isReceiving.connect(mem_fun(*this, &ModuleEchoLink::onIsReceiving));
  qso->audioReceivedRaw.connect(
      	  mem_fun(*this, &ModuleEchoLink::audioFromRemoteRaw));
  qso->destroyMe.connect(mem_fun(*this, &ModuleEchoLink::destroyQsoObject));

  splitter->addSink(qso);
  selector->addSource(qso);
  selector->enableAutoSelect(qso, 0);

  if (qsos.size() > max_qsos)
  {
    qso->reject(false);
    return;
  }
  
    // Check if it is a station that connects very often senselessly
  if ((num_con_max > 0) && !numConCheck(callsign))
  {
    qso->reject(false);
    return;
  }

  if ((regexec(reject_incoming_regex, callsign.c_str(), 0, 0, 0) == 0) ||
      (regexec(accept_incoming_regex, callsign.c_str(), 0, 0, 0) != 0) ||
      (reject_conf && (name.size() > 3) &&
       (name.rfind("CONF") == (name.size()-4))))
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
      vector<QsoImpl*>::iterator it = find(qsos.begin(), qsos.end(), qso);
      assert (it != qsos.end());
      qsos.erase(it);
      it=qsos.begin();
      qsos.insert(it,qso);
      updateEventVariables();
      
      if (!qso->connectionRejected())
      {
        last_disc_stn = qso->stationData();
      }
              
      if (remote_activation &&
      	  (qsos.back()->currentState() == Qso::STATE_DISCONNECTED))
      {
      	deactivateMe();
      }

      if (autocon_timer != 0)
      {
        autocon_timer->setTimeout(autocon_time);
      }

      broadcastTalkerStatus();
      updateDescription();
      clientListChanged();
      break;
    }
    
    case Qso::STATE_CONNECTED:
      updateEventVariables();
      clientListChanged();
      break;

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
  
  vector<QsoImpl*>::iterator it;
  for (it=qsos.begin(); it!=qsos.end(); ++it)
  {
    if (*it != qso)
    {
      (*it)->sendChatData(msg);
    }
  }

    // Escape TCL control characters
  string escaped(msg);
  replaceAll(escaped, "\\", "\\\\");
  replaceAll(escaped, "{", "\\{");
  replaceAll(escaped, "}", "\\}");
  stringstream ss;
    // FIXME: This TCL specific code should not be here
  ss << "chat_received [subst -nocommands -novariables {";
  ss << escaped;
  ss << "}]";
  processEvent(ss.str());
} /* onChatMsgReceived */


/*
 *----------------------------------------------------------------------------
 * Method:    onInfoMsgReceived
 * Purpose:   Called by the EchoLink::Qso object when a info message has been
 *    	      received from the remote station.
 * Input:     qso - The QSO object
 *    	      msg - The received message
 * Output:    None
 * Author:    Tobias Blomberg / SM0SVX
 * Created:   2017-05-13
 * Remarks:
 * Bugs:
 *----------------------------------------------------------------------------
 */
void ModuleEchoLink::onInfoMsgReceived(QsoImpl *qso, const string& msg)
{
    // Escape TCL control characters
  string escaped(msg);
  replaceAll(escaped, "\\", "\\\\");
  replaceAll(escaped, "{", "\\{");
  replaceAll(escaped, "}", "\\}");
  stringstream ss;
    // FIXME: This TCL specific code should not be here
  ss << "info_received \"" << qso->remoteCallsign()
     << "\" [subst -nocommands -novariables {";
  ss << escaped;
  ss << "}]";
  processEvent(ss.str());
} /* onInfoMsgReceived */


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
  
  stringstream ss;
  ss << "is_receiving " << (is_receiving ? "1" : "0")
     << " " << qso->remoteCallsign();
  processEvent(ss.str());

  if ((talker == 0) && is_receiving)
  {
    if (reject_conf)
    {
      string name = qso->remoteName();
      if ((name.size() > 3) && (name.rfind("CONF") == (name.size()-4)))
      {
	qso->sendChatData("Connects from a conference are not allowed");
	qso->disconnect();
	return;
      }
    }
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
      
  vector<QsoImpl*>::iterator it = find(qsos.begin(), qsos.end(), qso);
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

    dir_refresh_timer = new Timer(600000);
    dir_refresh_timer->expired.connect(
      	    mem_fun(*this, &ModuleEchoLink::getDirectoryList));
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

  if (qsos.size() >= max_qsos)
  {
    cerr << "Couldn't connect to " << station.callsign() << " due to the "
         << "number of active connections (" << qsos.size() << " > "
         << max_qsos << ")" << endl;
    processEvent("no_more_connections_allowed");
    return;
  }

  cout << "Connecting to " << station.callsign() << " (" << station.id()
       << ")\n";
  
  QsoImpl *qso = 0;
  
  vector<QsoImpl*>::iterator it;
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
      qso = *it;
      qsos.erase(it);
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
    qso->setListenOnly(!listen_only_valve->isOpen());
    qso->stateChange.connect(mem_fun(*this, &ModuleEchoLink::onStateChange));
    qso->chatMsgReceived.connect(
        mem_fun(*this, &ModuleEchoLink::onChatMsgReceived));
    qso->infoMsgReceived.connect(
        mem_fun(*this, &ModuleEchoLink::onInfoMsgReceived));
    qso->isReceiving.connect(mem_fun(*this, &ModuleEchoLink::onIsReceiving));
    qso->audioReceivedRaw.connect(
      	    mem_fun(*this, &ModuleEchoLink::audioFromRemoteRaw));
    qso->destroyMe.connect(mem_fun(*this, &ModuleEchoLink::destroyQsoObject));

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

    LocationInfo::instance()->updateQsoStatus(1, station.callsign(),
                                              info.str(), call_list);
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
    vector<QsoImpl*>::iterator it;
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
  vector<QsoImpl*>::const_iterator it;
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
  msg << "SvxLink " << SVXLINK_APP_VERSION << " - " << mycall
      << " (" << numConnectedStations() << ")\n\n";

  if (squelch_is_open && listen_only_valve->isOpen())
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
    msg << mycall << "         ";
    if (!listen_only_valve->isOpen())
    {
      msg << "[listen only] ";
    }
    msg << sysop_name << "\n";
  }
  
  vector<QsoImpl*>::const_iterator it;
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
  cbc_timer->expired.connect(mem_fun(*this, &ModuleEchoLink::cbcTimeout));

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


void ModuleEchoLink::disconnectByCallsign(const string &cmd)
{
  if ((cmd.size() != 1) || qsos.empty())
  {
    commandFailed(cmd);
    return;
  }

  stringstream ss;
  ss << "dbc_list [list";
  vector<QsoImpl*>::iterator it;
  for (it=qsos.begin(); it!=qsos.end(); ++it)
  {
    if ((*it)->currentState() != Qso::STATE_DISCONNECTED)
    {
    	ss << " " << (*it)->remoteCallsign();
    }
  }
  ss << "]";
  processEvent(ss.str());
  state = STATE_DISCONNECT_BY_CALL;
  delete dbc_timer;
  dbc_timer = new Timer(60000);
  dbc_timer->expired.connect(mem_fun(*this, &ModuleEchoLink::dbcTimeout));
} /* ModuleEchoLink::disconnectByCallsign  */


void ModuleEchoLink::handleDisconnectByCall(const string& cmd)
{
  if (cmd.empty())
  {
    processEvent("dbc_aborted");
    delete dbc_timer;
    dbc_timer = 0;
    state = STATE_NORMAL;
    return;
  }
  
  unsigned idx = static_cast<unsigned>(atoi(cmd.c_str()));
  stringstream ss;

  if (idx == 0)
  {
    ss << "dbc_list [list";
    vector<QsoImpl*>::const_iterator it;
    for (it = qsos.begin(); it != qsos.end(); ++it)
    {
      ss << " " << (*it)->remoteCallsign();
    }
    ss << "]";
    processEvent(ss.str());
    dbc_timer->reset();
    return;
  }

  if (idx > qsos.size())
  {
    ss << "dbc_index_out_of_range " << idx;
    processEvent(ss.str());
    dbc_timer->reset();
    return;
  }

  qsos[idx-1]->disconnect();
  delete dbc_timer;
  dbc_timer = 0;
  state = STATE_NORMAL;

} /* ModuleEchoLink::handleDisconnectByCall  */


void ModuleEchoLink::dbcTimeout(Timer *t)
{
  delete dbc_timer;
  dbc_timer = 0;
  state = STATE_NORMAL;
  cout << "Disconnect by call command timeout\n";
  processEvent("dbc_timeout");
} /* ModuleEchoLink::dbcTimeout  */


int ModuleEchoLink::numConnectedStations(void)
{
  int cnt = 0;
  vector<QsoImpl*>::iterator it;
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
  vector<QsoImpl*>::iterator it;
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
    vector<QsoImpl*>::iterator it;
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
        // coverity[dont_call]
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
    
    vector<QsoImpl*>::iterator it;
    for (it=qsos.begin(); it!=qsos.end(); ++it)
    {
      (*it)->setListenOnly(activate);
    }

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
  else if (cmd[0] == '7')   // Disconnect by callsign
  {
    disconnectByCallsign(cmd);
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
} /* ModuleEchoLink::connectByNodeId */


void ModuleEchoLink::checkIdle(void)
{
  setIdle(qsos.empty() &&
      	  logicIsIdle() &&
	  (state == STATE_NORMAL));
} /* ModuleEchoLink::checkIdle */


/*
 *----------------------------------------------------------------------------
 * Method:    checkAutoCon
 * Purpose:   Initiate the process of connecting to autocon_echolink_id
 * Input:     timer - the timer instance (not used)
 * Output:    None
 * Author:    Robbie De Lise / ON4SAX
 * Created:   2010-07-30
 * Remarks:
 * Bugs:
 *----------------------------------------------------------------------------
 */
void ModuleEchoLink::checkAutoCon(Timer *) 
{
    // Only try to activate the link if we are online and not
    // currently connected to any station. A connection will only be attempted
    // if module activation is successful.
  if ((dir->status() == StationData::STAT_ONLINE)
      && (numConnectedStations() == 0)
      && activateMe())
  {
    cout << "ModuleEchoLink: Trying autoconnect to "
         << autocon_echolink_id << "\n";
    connectByNodeId(autocon_echolink_id);
  }
} /* ModuleEchoLink::checkAutoCon */


bool ModuleEchoLink::numConCheck(const std::string &callsign)
{
    // Get current time
  struct timeval con_time;
  gettimeofday(&con_time, NULL);

    // Refresh connect watch list
  numConUpdate();

  NumConMap::iterator cit = num_con_map.find(callsign);
  if (cit != num_con_map.end())
  {
      // Get an alias (reference) to the callsign and NumConStn objects
    const string &t_callsign = (*cit).first;
    NumConStn &stn = (*cit).second;

      // Calculate time difference from last connection
    struct timeval diff_tv;
    timersub(&con_time, &stn.last_con, &diff_tv);

      // Bug in Win-Echolink? Number of connect requests up to 5/sec
      // do not count stations if it's requesting 3-5/seconds
    if (diff_tv.tv_sec > 3)
    {
      ++stn.num_con;
      stn.last_con = con_time;
      cout << "### Station " << t_callsign << ", count " << stn.num_con << " of "
           << num_con_max << " possible number of connects" << endl;
    }

      // Number of connects are too high
    if (stn.num_con > num_con_max)
    {
      time_t next = con_time.tv_sec + num_con_block_time;
      char time_str[64];
      struct tm tm;
      strftime(time_str, sizeof(time_str), "%c", localtime_r(&next, &tm));
      cerr << "*** WARNING: Ingnoring incoming connection because "
           << "the station (" << callsign << ") has connected " 
           << "to often (" << stn.num_con << " times). " 
           << "Next connect is possible after " << time_str << ".\n";
      return false;
    }
  }
  else
  {
      // Insert initial entry on first connect
    cout << "### Register incoming station, count 1 of " << num_con_max
         << " possible number of connects" << endl;
    num_con_map.insert(make_pair(callsign, NumConStn(1, con_time)));
  }

  return true;

} /* ModuleEchoLink::numConCheck */


void ModuleEchoLink::numConUpdate(void)
{
    // Get current time
  struct timeval now;
  gettimeofday(&now, NULL);

  NumConMap::iterator cit = num_con_map.begin();
  while (cit != num_con_map.end())
  {
      // Get an alias (reference) to the callsign and NumConStn objects
    const string &t_callsign = (*cit).first;
    const NumConStn &stn = (*cit).second;

    struct timeval remove_at = stn.last_con;
    if (stn.num_con > num_con_max)
    {
      remove_at.tv_sec += num_con_block_time;
    }
    else
    {
      remove_at.tv_sec += num_con_ttl;
    }

      // If the entry have timed out, delete it
    if (timercmp(&remove_at, &now, <))
    {
      cout << "### Delete " << t_callsign << " from watchlist" << endl;
      num_con_map.erase(cit++);
    }
    else
    {
      if (stn.num_con > num_con_max)
      {
        cout << "### " << t_callsign << " is blocked" << endl;
      }
      ++cit;
    }
  }

  num_con_update_timer->reset();

} /* ModuleEchoLink::numConUpdate */


void ModuleEchoLink::replaceAll(std::string &str, const std::string &from,
                                const std::string &to) const
{
  if(from.empty())
  {
    return;
  }
  size_t start_pos = 0;
  while((start_pos = str.find(from, start_pos)) != std::string::npos)
  {
    str.replace(start_pos, from.length(), to);
    start_pos += to.length();
  }
} /* ModuleEchoLink::replaceAll */


bool ModuleEchoLink::setRegex(regex_t*& regex, const std::string& cfg_tag,
    const std::string& default_regex_str)
{
  std::string regex_str;
  if (!cfg().getValue(cfgName(), cfg_tag, regex_str))
  {
    regex_str = default_regex_str;
  }
  delete regex;
  regex = new regex_t;
  int err = regcomp(regex, regex_str.c_str(),
                    REG_EXTENDED | REG_NOSUB | REG_ICASE);
  if (err != 0)
  {
    size_t msg_size = regerror(err, regex, 0, 0);
    char msg[msg_size];
    size_t err_size = regerror(err, regex, msg, msg_size);
    assert(err_size == msg_size);
    std::cerr << "*** ERROR: Syntax error in " << cfgName()
              << "/" << cfg_tag << ": " << msg << std::endl;
    return false;
  }
  return true;
} /* ModuleEchoLink::setRegex */


bool ModuleEchoLink::setDropIncomingRegex(void)
{
  return setRegex(drop_incoming_regex, CFG_DROP_INCOMING, "^$");
} /* ModuleEchoLink::setDropIncomingRegex */


bool ModuleEchoLink::setRejectIncomingRegex(void)
{
  return setRegex(reject_incoming_regex, CFG_REJECT_INCOMING, "^$");
} /* ModuleEchoLink::setRejectIncomingRegex */


bool ModuleEchoLink::setAcceptIncomingRegex(void)
{
  return setRegex(accept_incoming_regex, CFG_ACCEPT_INCOMING, "^.*$");
} /* ModuleEchoLink::setAcceptIncomingRegex */


bool ModuleEchoLink::setRejectOutgoingRegex(void)
{
  return setRegex(reject_outgoing_regex, CFG_REJECT_OUTGOING, "^$");
} /* ModuleEchoLink::setRejectOutgoingRegex */


bool ModuleEchoLink::setAcceptOutgoingRegex(void)
{
  return setRegex(accept_outgoing_regex, CFG_ACCEPT_OUTGOING, "^.*$");
} /* ModuleEchoLink::setAcceptOutgoingRegex */


void ModuleEchoLink::cfgValueUpdated(const std::string& section,
    const std::string& tag)
{
  if (section != cfgName())
  {
    return;
  }

  if (tag == CFG_DROP_INCOMING)
  {
    setDropIncomingRegex();
  }
  else if (tag == CFG_REJECT_INCOMING)
  {
    setRejectIncomingRegex();
  }
  else if (tag == CFG_ACCEPT_INCOMING)
  {
    setAcceptIncomingRegex();
  }
  else if (tag == CFG_REJECT_OUTGOING)
  {
    setRejectOutgoingRegex();
  }
  else if (tag == CFG_ACCEPT_OUTGOING)
  {
    setAcceptOutgoingRegex();
  }
} /* ModuleEchoLink::cfgValueUpdated */


/*
 * This file has not been truncated
 */
