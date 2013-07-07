/**
@file	 LinkManager.cpp
@brief   Contains the manager to link different logics together
         implemented as singleton
@author  Adi Bier (DL1HRC) / Christian Stussak (University of Halle/Saale)
         Tobias Blomberg / SM0SVX
@date	 2011-11-24

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2011 Tobias Blomberg / SM0SVX

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

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <limits>
#include <string>
#include <algorithm>
#include <iterator>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncConfig.h>
#include <AsyncTimer.h>
#include <common.h>
#include <AsyncAudioSource.h>
#include <AsyncAudioSink.h>
#include <AsyncAudioSplitter.h>
#include <AsyncAudioSelector.h>
#include <AsyncAudioPassthrough.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "LinkManager.h"
#include "CmdParser.h"
#include "Logic.h"
#include "LogicCmds.h"


/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/

using namespace std;
using namespace Async;


  // Put all locally defined types, functions and variables in an anonymous
  // namespace to make them file local. The "static" keyword has been
  // deprecated in C++ so it should not be used.
namespace
{

/****************************************************************************
 *
 * Defines & typedefs
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Prototypes for local functions
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Local Global Variables
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Local class definitions
 *
 ****************************************************************************/


} // End of anonymous namespace

/****************************************************************************
 *
 * Exported Global Variables
 *
 ****************************************************************************/

LinkManager* LinkManager::_instance = 0;


/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/

bool LinkManager::initialize(const Async::Config &cfg,
                             const std::string &link_names)
{
  assert(!LinkManager::hasInstance());
  LinkManager::_instance = new LinkManager;

  bool init_ok = true;

    // Split the line into an array of section names
    // GLOBAL/LINKS=Link1,Link2,Link3,...
  vector<string> linklist;
  SvxLink::splitStr(linklist, link_names, ",");

    // Loop to create a link object for each single linkdefinition
    // in the "LINKS=Link1, Link2, Link3" set
  for (vector<string>::const_iterator name = linklist.begin();
       name != linklist.end();
       name++)
  {
    Link &link = LinkManager::instance()->links[*name];

      // The link name is the same as the config section name
    link.name = *name;

      // Logic1:70:name1,Logic2:71:name2,...
    string connect_logics;
    cfg.getValue(*name, "CONNECT_LOGICS", connect_logics);

    vector<string> logic_specs;
    SvxLink::splitStr(logic_specs, connect_logics, ",");
    if (logic_specs.size() < 2)
    {
      cerr << "*** ERROR: " << *name << "/CONNECT_LOGICS=? You must "
           << "have at least two logics to connect.\n";
      init_ok = false;
    }

      // Configure the list of logics to connect
      // CONNECT_LOGICS=Logic1:70:Name1,Logic2:71,Logic3:72:Name2,...
    for(vector<string>::iterator it = logic_specs.begin();
        it != logic_specs.end();
        ++it)
    {
      vector<string> logic_spec;
      SvxLink::splitStr(logic_spec, *it, ":");
      logic_spec.resize(3);
      if (logic_spec.at(0).size() > 0)
      {
        string logic_name = logic_spec.at(0);
        CmdProperties cmd_props;
        cmd_props.logic_cmd =  logic_spec.at(1);
        cmd_props.announcement_name = logic_spec.at(2);
        link.cmd_props.insert(make_pair(logic_name, cmd_props));
      }
      else
      {
        cout << "*** ERROR: Bad configuration for " << *name
             << "/CONNECT_LOGICS=" << connect_logics << ". Legal format: "
             << "<logic name>:<command>:<announcement name>,...\n";
        init_ok = false;
      }
    }

    cfg.getValue(*name, "TIMEOUT", link.timeout);
    link.timeout *= 1000;
    if (link.timeout > 0)
    {
      link.timeout_timer = new Timer(link.timeout);
      link.timeout_timer->setEnable(false);
      link.timeout_timer->expired.connect(
          bind(mem_fun(LinkManager::instance(), &LinkManager::linkTimeout),
               &link));
    }

      // Automatically activate the link, if one (or more) logics
      // has activity, e.g. squelch open.
    string autoconnect_on_sql;
    if (cfg.getValue(*name, "AUTOCONNECT_ON_SQL", autoconnect_on_sql))
    {
      SvxLink::splitStr(link.auto_connect, autoconnect_on_sql, ",");

        // An automatically connected link should be disconnected after a
        // while so the TIMEOUT configuration variable must be set.
      if (link.timeout == 0)
      {
        cout << "*** WARNING: missing param " << *name
             << "/TIMEOUT=??, set to default (30 sec)\n";
        link.timeout = 30000;
      }
    }

    cfg.getValue(*name, "DEFAULT_CONNECT", link.default_connect);
    cfg.getValue(*name, "NO_DISCONNECT", link.no_disconnect);
  }

  if(!init_ok)
  {
    deleteInstance();
  }

  return init_ok;

} /* LinkManager::initialize */


void LinkManager::addLogic(Logic *logic)
{
    // Create a splitter to split the source audio from the logic being added
    // to all other logics.
  AudioSplitter *splitter = new AudioSplitter;
  logic->logicConOut()->registerSink(splitter);

    // Register the new logic source
  sources[logic->name()].source = logic->logicConOut();
  sources[logic->name()].splitter = splitter;

    // Create a selector for the logic being added to receive audio from all
    // other sinks.
  AudioSelector *selector = new AudioSelector;
  selector->registerSink(logic->logicConIn());

    // Register the new logic sink
  sinks[logic->name()].sink = logic->logicConIn();
  sinks[logic->name()].selector = selector;

    // Now create a connection from the new logic source to each sink.
  for (SinkMap::iterator it=sinks.begin(); it != sinks.end(); ++it)
  {
    AudioPassthrough *connector = new AudioPassthrough;
    splitter->addSink(connector);
    AudioSelector *other_selector = (*it).second.selector;
    other_selector->addSource(connector);
    (*it).second.connectors[logic->name()] = connector;
  }

    // Now create a connection from each existing logic source to the new sink.
  for (SourceMap::iterator it=sources.begin(); it!=sources.end(); ++it)
  {
    AudioPassthrough *connector = new AudioPassthrough;
    (*it).second.splitter->addSink(connector);
    selector->addSource(connector);
    sinks[logic->name()].connectors[(*it).first] = connector;
  }

  LogicInfo logic_info;
  logic_info.logic = logic;

    // Keep track of the newly added logics idle state so that we can start
    // and stop timeout timers.
  logic_info.idle_state_changed_con = logic->idleStateChanged.connect(
      bind(mem_fun(*this, &LinkManager::logicIdleStateChanged), logic));

    // Add the logic to the logic map
  logic_map[logic->name()] = logic_info;

    // Create command objects associated with this logic
  for (LinkMap::iterator it = links.begin(); it != links.end(); ++it)
  {
    Link &link = it->second;
    CmdPropMap::const_iterator cpmit(link.cmd_props.find(logic->name()));
    if (cpmit != link.cmd_props.end())
    {
      const CmdProperties &cmd_props = cpmit->second;
      if (atoi(cmd_props.logic_cmd.c_str()) > 0)
      {
        LinkCmd *link_cmd = new LinkCmd(logic, link);
        if (!link_cmd->initialize(cmd_props.logic_cmd))
        {
          cout << "*** WARNING: Can not setup command " << cmd_props.logic_cmd
               << " for the logic " << logic->name() << endl;
        }
      }
    }
  }
} /* LinkManager::addLogic */


void LinkManager::deleteLogic(Logic *logic)
{
    // Verify that the logic has been previously added
  LogicMap::iterator lmit = logic_map.find(logic->name());
  assert(lmit != logic_map.end());
  LogicInfo &logic_info = (*lmit).second;
  assert(logic_info.logic == logic);
  assert(sources.find(logic->name()) != sources.end());
  assert(sinks.find(logic->name()) != sinks.end());

    // Disconnect the idleStateChanged signal that was connected when the
    // logic was first registered.
  logic_info.idle_state_changed_con.disconnect();

    // Delete the logic source splitter and all connections associated with it
  AudioSplitter *splitter = sources[logic->name()].splitter;
  for (SinkMap::iterator smit=sinks.begin(); smit!=sinks.end(); ++smit)
  {
    SinkInfo &sink_info = (*smit).second;
    ConMap::iterator cmit = sink_info.connectors.find(logic->name());
    assert(cmit != sink_info.connectors.end());
    AudioPassthrough *connector = (*cmit).second;
    sink_info.selector->removeSource(connector);
    sink_info.connectors.erase(logic->name());
    splitter->removeSink(connector);
    delete connector;
  }
  delete splitter;
  sources.erase(logic->name());
  
    // Delete the logic sink and all connections associated with it
  AudioSelector *selector = sinks[logic->name()].selector;
  ConMap &cons = sinks[logic->name()].connectors;
  for (ConMap::iterator cmit = cons.begin(); cmit != cons.end(); ++cmit)
  {
    AudioPassthrough *connector = (*cmit).second;
    selector->removeSource(connector);
    const string &source_name = (*cmit).first;
    sources[source_name].splitter->removeSink(connector);
    delete connector;
  }
  delete selector;
  sinks.erase(logic->name());

    // Finally remove the logic from the logic_map
  logic_map.erase(logic->name());

} /* LinkManager::deleteLogic */


void LinkManager::allLogicsStarted(void)
{
  for (LinkMap::iterator it = links.begin(); it != links.end(); ++it)
  {
    Link &link = it->second;
    if (link.default_connect)
    {
      activateLink(link);
    }
  }
} /* LinkManager::allLogicsStarted */


string LinkManager::cmdReceived(LinkRef link, Logic *logic,
                                const string &subcmd)
{
  /*
  cout << "### LinkManager::cmdReceived: link=" << link.name
       << " logic=" << logic->name()
       << " subcmd=" << subcmd << endl;
  */

  stringstream ss;

  CmdPropMap::iterator cpmit = link.cmd_props.find(logic->name());
  assert(cpmit != link.cmd_props.end());
  CmdProperties &cmd_props = cpmit->second;
  if (subcmd == "0") // Disconnecting Link1 <-X-> Link2
  {
    if (link.no_disconnect)
    {
        // Not possible due to configuration
      ss << "deactivating_link_not_possible \"";
    }
    else if (!link.is_connected)
    {
      ss << "link_not_active \"";
    }
    else
    {
      ConnectResult tconnect = deactivateLink(link);
      switch (tconnect)
      {
        case ERROR:
          ss << "deactivating_link_failed \"";
          break;
        case OK:
          ss << "deactivating_link \"";
          break;
        case ALREADY_CONNECTED:
          ss << "deactivating_link_failed_other_connection \"";
          break;
      }
    }
    ss << cmd_props.announcement_name << "\"";
  }
  else if (subcmd == "1") // Connecting Logic1 <---> Logic2 (two ways)
  {
    if (link.is_connected)
    {
      ss << "link_already_active \"";
    }
    else
    {
      ConnectResult tconnect = activateLink(link);
      switch (tconnect)
      {
        case ERROR:
          ss << "activating_link_failed \"";
          break;
        case OK:
          ss << "activating_link \"";
          break;
        case ALREADY_CONNECTED:
          ss << "activating_link_failed_other_connection \"";
          break;
      }
    }
    ss << cmd_props.announcement_name + "\"";
  }
  else
  {
    ss << "unknown_command " << cmd_props.logic_cmd << subcmd;
  }
  return ss.str();
} /* LinkManager::cmdReceived */



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

bool LinkManager::sourceIsAdded(const string& logicname)
{
 return sources.count(logicname) > 0;
} /* LinkManager::sourceIsAdded */


bool LinkManager::sinkIsAdded(const string &logicname)
{
  return sinks.count(logicname) > 0;
} /* LinkManager::sinkIsAdded */


/**
 * @brief Connects the logics by checking the connect state before
 */
LinkManager::ConnectResult LinkManager::activateLink(Link &link)
{
  cout << "Activating link " << link.name << endl;

  ConnectResult check = ERROR;

    // Get the wanted connect matrix depending from the linkname, e.g.
    // <RepeaterLogic, MicSpkeLogic>
    // <RepeaterLogic, SimplexLogic>
    // <SimplexLogic, RepeaterLogic>
    // <SimplexLogic, MicSpkrLogic>
    // <MicSpkrLogic, RepeaterLogic>
    // <MicSpkrLogic, SimplexLogic>
  LogicConSet want = getMatrix(link.name);

    // Gets the difference of both, the result is the matrix to be connected
    // "current_cons" contains the actual state of the connected logics, e.g.
    // <RepeaterLogic, SimplexLogic>
    // <SimplexLogic, RepeaterLogic>
  LogicConSet diff = getDifference(current_cons, want);

    // If the logics are already connected by other linkdefinitions, we
    // have to tell the others about this situation
    // check = 0 -> problems when connecting the links
    // check = 1 -> the connect is established
    // check = 2 -> the request could not made because the logics are already
    //              connected by other linkdefinitions
  if (diff.size() == 0)
  {
    return ALREADY_CONNECTED;
  }

    // Diff contains the resulting logics that must beeing connected e.g.
    // <RepeaterLogic, MicSpkeLogic>
    // <SimplexLogic, MicSpkrLogic>
    // <MicSpkrLogic, RepeaterLogic>
    // <MicSpkrLogic, SimplexLogic>
  for (LogicConSet::iterator it = diff.begin(); it != diff.end(); ++it)
  {
    cout << "### " << it->first << " ===> " << it->second << endl;
    sinks[it->first].selector->
        enableAutoSelect(sinks[it->first].connectors[it->second], 0);

      // Store all connections in "current_cons"
    current_cons.insert(*it);
    check = OK;
  }

    // Mark link as connected
  link.is_connected = true;

    // Check if the timeout timer should be enabled or disabled
  checkTimeoutTimer(link);

  return check;
} /* LinkManager::activateLink */


LinkManager::LogicConSet LinkManager::getDifference(LogicConSet is,
                                                    LogicConSet want)
{
  LogicConSet ret = want;

  for (LogicConSet::iterator it = want.begin(); it != want.end(); it++)
  {
    if (is.find(*it) != is.end())
    {
      ret.erase(*it);
    }
  }

  return ret;
} /* Linkmanager::getDifference */


/**
 * @brief Returns a set of logics to be connected
 */
LinkManager::LogicConSet LinkManager::getLogics(const string& linkname)
{
  LogicConSet ret;
  LinkMap::iterator it = links.find(linkname);

  for (CmdPropMap::iterator xi = it->second.cmd_props.begin();
       xi != it->second.cmd_props.end();
       ++xi)
  {
    for (CmdPropMap::iterator xj = it->second.cmd_props.begin();
         xj != it->second.cmd_props.end();
         ++xj)
    {
      if (xj->first != xi->first)
      {
        if ((logic_map.find(xj->first) != logic_map.end()) &&
            (logic_map.find(xi->first) != logic_map.end()))
        {
          ret.insert(make_pair(xi->first, xj->first));
        }
        else
        {
          cout << "*** WARNING: One of the logics " << xj->first << " or "
               << xi->first << " is missing in link " << linkname << endl;
        }
      }
    }
  }
  return ret;
} /* LinkManager::getLogics */


LinkManager::LogicConSet LinkManager::getMatrix(const string& linkname)
{
  LinkMap::iterator it = links.find(linkname);
  LogicConSet ret = getLogics(linkname);

  for (LinkMap::iterator iu = links.begin(); iu != links.end(); ++iu)
  {
    if (iu->second.is_connected)
    {
      for (CmdPropMap::iterator xj = it->second.cmd_props.begin();
           xj != it->second.cmd_props.end();
           ++xj)
      {
          // look in the actual connected linkset if there is a logicname
          // that exists in another already connected link definition
        CmdPropMap::iterator cn = iu->second.cmd_props.find(xj->first);
        if (cn != iu->second.cmd_props.end())
        {
          for (CmdPropMap::iterator dn = iu->second.cmd_props.begin();
               dn != iu->second.cmd_props.end();
               ++dn)
          {
            if (dn->first != cn->first)
            {
              for (CmdPropMap::iterator xi = it->second.cmd_props.begin();
                   xi != it->second.cmd_props.end(); xi++)
              {
                if (dn->first != xi->first)
                {
                  ret.insert(make_pair(dn->first, xi->first));
                  ret.insert(make_pair(xi->first, dn->first));
                }
              }
            }
          }
        }
      }
    }
  }

  return ret;
} /* LinkManager::getMatrix */


/**
 * @brief Returns the logics that must be disconnected
 */
LinkManager::LogicConSet LinkManager::getToDisconnect(const string& name)
{
  LogicConSet ret = current_cons;  // Get all current connections

    // Go through all link definitions and check if they are connected
  for (LinkMap::iterator it = links.begin(); it != links.end(); ++it)
  {
    if (it->second.is_connected && (name != it->second.name))
    {
      LogicConSet t_ret = getLogics(it->second.name);
      for (LogicConSet::iterator s_it = t_ret.begin();
           s_it != t_ret.end();
           ++s_it)
      {
        if (ret.find(*s_it) != ret.end())
        {
          ret.erase(*s_it);
        }
      }
    }
  }

  return ret;
} /* LinkManager::getToDisconnect */


LinkManager::ConnectResult LinkManager::deactivateLink(Link &link)
{
  cout << "Deactivating link " << link.name << endl;

  ConnectResult check = ERROR;

  assert(sources.size() > 0);
  assert(sinks.size() > 0);

  LogicConSet diff = getToDisconnect(link.name);

  if (diff.size() == 0)
  {
    check = ALREADY_CONNECTED;
  }

  for (LogicConSet::iterator it = diff.begin(); it != diff.end(); ++it)
  {
    if ((logic_map.find(it->first) != logic_map.end()) &&
        (logic_map.find(it->second) != logic_map.end()))
    {
      cout << "### " << it->first << " =X=> " << it->second
           << endl;
      sinks[it->first].selector->
        disableAutoSelect(sinks[it->first].connectors[it->second]);

        // Delete the link-connect information
      current_cons.erase(*it);
      check = OK;
    }
    else
    {
      cout << "*** WARNING: Missing logics entry in [GLOBAL]/LINKS=???, "
           << "             check your configuration." << endl;
    }
  }

    // Reset the connected flag
  link.is_connected = false;

    // Check if the timeout timer should be enabled or disabled
  checkTimeoutTimer(link);

  return check;
} /* LinkManager::deactivateLink */


bool LinkManager::isConnected(const string& source_name,
      	                      const string& sink_name)
{
  assert(sources.count(source_name) == 1);
  assert(sinks.count(sink_name) == 1);
  assert(sinks[sink_name].connectors.count(source_name) == 1);

  AudioPassthrough *connector = sinks[sink_name].connectors[source_name];
  AudioSelector *selector = sinks[sink_name].selector;
  return selector->autoSelectEnabled(connector);
} /* LinkManager::isConnected */


/**
 * @brief Returns all linknames in that the specific logic is involved
 */
vector<string> LinkManager::getLinkNames(const string& logicname)
{
  vector<string> t_linknames;
  LinkMap::iterator it;

  for (LinkMap::iterator it=links.begin(); it!=links.end(); ++it)
  {
    if (it->second.cmd_props.find(logicname) != it->second.cmd_props.end())
    {
      t_linknames.push_back(it->first);
    }
  }
  return t_linknames;
} /* LinkManager::getLinkNames */


void LinkManager::linkTimeout(Async::Timer *t, Link *link)
{
  if (link->is_connected && !link->default_connect && !link->no_disconnect)
  {
    deactivateLink(*link);
  }

  if (!link->is_connected && link->default_connect)
  {
    activateLink(*link);
  }
} /* LinkManager::linkTimeout */


void LinkManager::logicIdleStateChanged(bool is_idle, const Logic *logic)
{
  /*
  cout << "### LinkManager::logicIdleStateChanged:"
       << " is_idle=" << is_idle
       << " logic_name=" << logic->name()
       << endl;
  */

    // We need all "linknames" where the "logic_name" is included in
  vector<string> link_names = getLinkNames(logic->name());

    // Loop through all links associated with the logic to see if we should
    // enable or disable any timeout timers.
  for (std::vector<string>::iterator lnit = link_names.begin();
       lnit != link_names.end();
       ++lnit)
  {
    const string &link_name = *lnit;
    LinkMap::iterator lcit = links.find(link_name);
    assert(lcit != links.end());
    Link &link = (*lcit).second;
    
      // Check if the logic that updated its idle state is defined as a
      // "auto_connect" logic. If not idle, all logics of the link shall
      // be connected if they were disconnected before.
      // FIXME: We now activate the link even on announcements since that
      // changes the idle status of a logic core. Probably not what we want.
    if (!is_idle && !link.is_connected)
    {
      StrSet::iterator acit = link.auto_connect.find(logic->name());
      if (acit != link.auto_connect.end())
      {
        cout << "### Activating link " << link_name <<
          " due to AUTOCONNECT_ON_SQL from " << logic->name() << endl;
        activateLink(link);
      }
    }

    checkTimeoutTimer(link);
  }
} /* LinkManager::logicIdleStateChanged */


void LinkManager::checkTimeoutTimer(Link &link)
{
  if (link.timeout_timer == 0)
  {
    return;
  }

  bool all_logics_idle = true;
  for (CmdPropMap::iterator cnit = link.cmd_props.begin();
       cnit != link.cmd_props.end();
       ++cnit)
  {
    const string &logic_name = (*cnit).first;
    LogicMap::iterator lmit = logic_map.find(logic_name);
    assert(lmit != logic_map.end());
    Logic *logic = (*lmit).second.logic;
    all_logics_idle &= logic->isIdle();
  }

  if (all_logics_idle && (link.is_connected != link.default_connect))
  {
    //cout << "### Enabling timeout timer for link " << link.name << endl;
    link.timeout_timer->setEnable(true);
  }
  else
  {
    //cout << "### Disabling timeout timer for link " << link.name << endl;
    link.timeout_timer->setEnable(false);
  }
} /* LinkManager::checkTimeoutTimer */



/*
 * This file has not been truncated
 */
