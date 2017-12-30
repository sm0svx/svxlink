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

#include <cstdlib>
#include <iostream>
#include <sstream>
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
    // in the "LINKS=Link1,Link2,Link3" set
  for (vector<string>::const_iterator name_it = linklist.begin();
       name_it != linklist.end();
       name_it++)
  {
    if (LinkManager::instance()->links.find(*name_it) !=
        LinkManager::instance()->links.end())
    {
      continue;
    }

      // This assignment will create a new link definition
    Link &link = LinkManager::instance()->links[*name_it];

      // The link name is the same as the config section name
    link.name = *name_it;

      // Logic1:70:name1,Logic2:71:name2,...
    string connect_logics;
    cfg.getValue(link.name, "CONNECT_LOGICS", connect_logics);

    vector<string> logic_specs;
    SvxLink::splitStr(logic_specs, connect_logics, ",");

      // Configure the list of logics to connect
      // CONNECT_LOGICS=Logic1:70:Name1,Logic2:71,Logic3:72:Name2,...
    for(vector<string>::iterator it = logic_specs.begin();
        it != logic_specs.end();
        ++it)
    {
      vector<string> logic_spec;
      SvxLink::splitStr(logic_spec, *it, ":");
      logic_spec.resize(3);
      string logic_name = logic_spec.at(0);
      if (!logic_name.empty())
      {
        LogicProperties logic_props;
        logic_props.cmd =  logic_spec.at(1);
        logic_props.announcement_name = logic_spec.at(2);
        pair<LogicPropMap::iterator, bool> ret =
            link.logic_props.insert(make_pair(logic_name, logic_props));
        if (!ret.second)
        {
          cout << "*** ERROR: Duplicate logic \"" << logic_name
               << "\" specified in link \"" << link.name << "\".\n";
          init_ok = false;
        }
      }
      else
      {
        cout << "*** ERROR: Bad configuration for " << link.name
             << "/CONNECT_LOGICS=" << connect_logics << ". Legal format: "
             << "<logic name>:<command>:<announcement name>,...\n";
        init_ok = false;
      }
    }

    if (link.logic_props.size() < 2)
    {
      cerr << "*** ERROR: " << link.name << "/CONNECT_LOGICS=? You must "
           << "have at least two logics to connect.\n";
      init_ok = false;
    }

    cfg.getValue(link.name, "TIMEOUT", link.timeout);
    link.timeout *= 1000;
    if (link.timeout > 0)
    {
      link.timeout_timer = new Timer(link.timeout);
      link.timeout_timer->setEnable(false);
      link.timeout_timer->expired.connect(sigc::bind(
          mem_fun(LinkManager::instance(), &LinkManager::linkTimeout),
          &link));
    }

      // Automatically activate the link, if one (or more) logics
      // has activity, e.g. squelch open.
    string autoactivate_on_sql;
    if (cfg.getValue(link.name, "AUTOACTIVATE_ON_SQL", autoactivate_on_sql))
    {
      SvxLink::splitStr(link.auto_activate, autoactivate_on_sql, ",");

        // An automatically connected link should be disconnected after a
        // while so the TIMEOUT configuration variable must be set.
      if (link.timeout == 0)
      {
        cout << "*** WARNING: missing param " << link.name
             << "/TIMEOUT=??, set to default (30 sec)\n";
        link.timeout = 30000;
      }
    }

    cfg.getValue(link.name, "DEFAULT_ACTIVE", link.default_active);
  }

  if(!init_ok)
  {
    deleteInstance();
  }

  return init_ok;

} /* LinkManager::initialize */


void LinkManager::addLogic(LogicBase *logic)
{
    // Make sure that we have not added this logic before
  assert(logic_map.find(logic->name()) == logic_map.end());

    // Make sure that the logic connection objects have been created
  assert(logic->logicConOut() != 0);
  assert(logic->logicConIn() != 0);

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
    splitter->addSink(connector, true);
    AudioSelector *other_selector = (*it).second.selector;
    other_selector->addSource(connector);
    (*it).second.connectors[logic->name()] = connector;
  }

    // Now create a connection from each existing logic source to the new sink.
  for (SourceMap::iterator it=sources.begin(); it!=sources.end(); ++it)
  {
    AudioPassthrough *connector = new AudioPassthrough;
    (*it).second.splitter->addSink(connector, true);
    selector->addSource(connector);
    sinks[logic->name()].connectors[(*it).first] = connector;
  }

  LogicInfo logic_info;
  logic_info.logic = logic;

    // Keep track of the newly added logics idle state so that we can start
    // and stop timeout timers.
  logic_info.idle_state_changed_con = logic->idleStateChanged.connect(
      sigc::bind(mem_fun(*this, &LinkManager::logicIdleStateChanged), logic));

    // Add the logic to the logic map
  logic_map[logic->name()] = logic_info;

    // Create command objects associated with this logic
  Logic *cmd_logic = dynamic_cast<Logic*>(logic);
  if (cmd_logic != 0)
  {
    for (LinkMap::iterator it = links.begin(); it != links.end(); ++it)
    {
      Link &link = it->second;
      LogicPropMap::const_iterator prop_it(link.logic_props.find(logic->name()));
      if (prop_it != link.logic_props.end())
      {
        const LogicProperties &logic_props = prop_it->second;
        if (atoi(logic_props.cmd.c_str()) > 0)
        {
          LinkCmd *link_cmd = new LinkCmd(cmd_logic, link);
          if (!link_cmd->initialize(logic_props.cmd))
          {
            cout << "*** WARNING: Can not setup command " << logic_props.cmd
                 << " for the logic " << logic->name() << endl;
          }
        }
      }
    }
  }
} /* LinkManager::addLogic */


void LinkManager::deleteLogic(LogicBase *logic)
{
  LogicMap::iterator lmit = logic_map.find(logic->name());
  if (lmit == logic_map.end())
  {
    return;
  }

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
    //delete connector;
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
    //delete connector;
  }
  delete selector;
  sinks.erase(logic->name());

    // Finally remove the logic from the logic_map
  logic_map.erase(logic->name());

} /* LinkManager::deleteLogic */


void LinkManager::allLogicsStarted(void)
{
  all_logics_started = true;

  for (LinkMap::iterator it = links.begin(); it != links.end(); ++it)
  {
    Link &link = it->second;

      // Check that all logics that are specified for this link has been
      // registered
    LogicPropMap::iterator prop_it = link.logic_props.begin();
    while (prop_it != link.logic_props.end())
    {
      const string &logic_name = (*prop_it).first;
      if (logic_map.find(logic_name) == logic_map.end())
      {
        cout << "*** WARNING: Logic " << logic_name
             << " has been specified in logic link " << link.name
             << " but that logic is missing. Removing logic from link.\n";
        LogicPropMap::iterator remove_it = prop_it++;
        link.logic_props.erase(remove_it);
      }
      else
      {
        ++prop_it;
      }
    }

      // If default active, activate the link
    if (link.default_active)
    {
      activateLink(link);
    }
  }
} /* LinkManager::allLogicsStarted */


string LinkManager::cmdReceived(LinkRef link, LogicBase *logic,
                                const string &subcmd)
{
  /* cout << "### LinkManager::cmdReceived: link=" << link.name
       << " logic=" << logic->name()
       << " subcmd=" << subcmd << endl;
  */

  stringstream ss;

  LogicPropMap::iterator prop_it = link.logic_props.find(logic->name());
  assert(prop_it != link.logic_props.end());
  LogicProperties &logic_props = prop_it->second;
  if (subcmd == "0") // Disconnecting Link1 <-X-> Link2
  {
    if (!link.is_activated)
    {
      ss << "link_not_active \"";
    }
    else
    {
      deactivateLink(link);
      ss << "deactivating_link \"";
    }
    ss << logic_props.announcement_name << "\"";
  }
  else if (subcmd == "1") // Connecting Logic1 <---> Logic2 (two ways)
  {
    if (!link.is_activated)
    {
      activateLink(link);
      ss << "activating_link \"";
    }
    else
    {
      ss << "link_already_active \"";
    }
    ss << logic_props.announcement_name + "\"";
  }
  else
  {
    ss << "unknown_command " << logic_props.cmd << subcmd;
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

LinkManager::~LinkManager(void)
{
  LogicMap::iterator it = logic_map.begin();
  while (it != logic_map.end())
  {
    deleteLogic(it->second.logic);
    it = logic_map.begin();
  }
} /* LinkManager::~LinkManager */


/**
 * @brief Find out which logics that should be connected
 * @param want The connections we want
 *
 * This function will calculate which logic connections that should currently
 * be established to get the correct audio flow in the currently activated
 * logic links. It will not set up the logic connections though so the caller
 * have to figure out which currently active connections to break and which to
 * establish to get to the set of connections reported by this function.
 */
void LinkManager::wantedConnections(LogicConSet &want)
{
    // Fill "want" with the base set of connections that are explicitly
    // specified in the link configuration for active links. The connection
    // graph will not be complete if two links share one or more logics. This
    // will be solved in the next step below.
  for (LinkMap::iterator lit = links.begin(); lit != links.end(); ++lit)
  {
    Link &link = (*lit).second;
    if (link.is_activated)
    {
        // oit=outer iterator
      for (LogicPropMap::iterator oit = link.logic_props.begin();
           oit != link.logic_props.end();
           ++oit)
      {
          // iit=inner iterator
        LogicPropMap::iterator iit = oit;
        for (++iit; iit != link.logic_props.end(); ++iit)
        {
          if ((*oit).first != (*iit).first)
          {
            want.insert(make_pair((*oit).first, (*iit).first));
            want.insert(make_pair((*iit).first, (*oit).first));
          }
        }
      }
    }
  }

    // Now we need to complete the graph with connections between logics that
    // are implicitly connected. That is, logics that are connected to each
    // other via other logics.
    //
    // The algoritm below is not very optimized but that should not be needed
    // since SvxLink most often just have a few logics defined. If a lot of
    // logics is used, the code below may have to be optimized.
    //
    // The outer loop (do {} while) is needed since new connections may be
    // added and then new implicit connections may appear. We are done when
    // all connections have been gone through and no new connections was
    // added.
  size_t prev_size;
  do
  {
      // Store the current size of the connection set so that we can check
      // later if new connections have been added.
    prev_size = want.size();

      // Now compare all connections to each other. This requires two loops,
      // an outer (oit=outer iterator) and an inner (iit=inner iterator).
    for (LogicConSet::iterator oit = want.begin();
         oit != want.end();
         ++oit)
    {
      for (LogicConSet::iterator iit = want.begin();
           iit != want.end();
           ++iit)
      {
          // If the sink of the outer connection belongs to the same logic
          // as the source of the inner connection, then we need to also
          // connect the source of the outer connection to the sink of the
          // inner connection. To exemplify, if the set contains the
          // connections: <L1,L2>,<L2,L3> we also need to add <L1,L3> since
          // L1 is implicitly connected to L3 via L2.
          // The comparison below also make sure that a connection is not
          // added which connect back to the same logic. To exemplify this,
          // if the set contains the connections: <L1,L2>,<L2,L1> we should
          // not connect <L1,L1>.
        if ((oit->second == iit->first) && (oit->first != iit->second))
        {
          want.insert(make_pair(oit->first, iit->second));
        }
      }
    }
  } while (want.size() > prev_size);
} /* LinkManager::wantedConnections */


/**
 * @brief Activates the specified link
 */
void LinkManager::activateLink(Link &link)
{
  if (link.is_activated)
  {
    return;
  }

  cout << "Activating link " << link.name << endl;

    // Mark link as activated
  link.is_activated = true;

    // Get the wanted logic connections based on which links that are activated
  LogicConSet want;
  wantedConnections(want);

    // Calculate the difference between the wanted connection set and the
    // current connection set. This is easily done using the difference set
    // operation. After this operation the "to_connect" variable will contain
    // the set of logic connections that have to be established.
  LogicConSet to_connect;
  set_difference(want.begin(), want.end(),
                 current_cons.begin(), current_cons.end(),
                 inserter(to_connect, to_connect.end()));

    // Establish missing connections
  for (LogicConSet::iterator it = to_connect.begin();
       it != to_connect.end();
       ++it)
  {
    const string &src_name = it->first;
    const string &sink_name = it->second;
    //cout << "### " << src_name << " ===> " << sink_name << endl;
    SinkInfo &sink = sinks[sink_name];
    sink.selector->enableAutoSelect(sink.connectors[src_name], 0);

      // Store all connections in "current_cons" (current connections)
    current_cons.insert(*it);
  }

    // Check if the timeout timer should be enabled or disabled
  checkTimeoutTimer(link);

} /* LinkManager::activateLink */


void LinkManager::deactivateLink(Link &link)
{
  if (!link.is_activated)
  {
    return;
  }

  cout << "Deactivating link " << link.name << endl;

    // Clear the activation flag
  link.is_activated = false;

    // Get the wanted connections
  LogicConSet want;
  wantedConnections(want);

    // Calculate which of the current connections should be broken.
    // This is easily done with a simple difference set operation.
  LogicConSet to_disconnect;
  set_difference(current_cons.begin(), current_cons.end(),
                 want.begin(), want.end(),
                 inserter(to_disconnect, to_disconnect.end()));

  for (LogicConSet::iterator it = to_disconnect.begin();
       it != to_disconnect.end();
       ++it)
  {
    const string &src_name = it->first;
    const string &sink_name = it->second;
    //cout << "### " << src_name << " =X=> " << sink_name << endl;

    SinkMap::iterator sink_it = sinks.find(sink_name);
    assert(sink_it != sinks.end());
    SinkInfo &sink = (*sink_it).second;

      // Disconnect the audio path from source logic to sink logic
    sink.selector->disableAutoSelect(sink.connectors[src_name]);

      // Delete the link connect information
    current_cons.erase(*it);
  }

    // Check if the timeout timer should be enabled or disabled
  checkTimeoutTimer(link);

} /* LinkManager::deactivateLink */


#if 0
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
#endif


/**
 * @brief Returns all linknames in that the specific logic is involved
 */
vector<string> LinkManager::getLinkNames(const string& logicname)
{
  vector<string> linknames;
  for (LinkMap::iterator it=links.begin(); it!=links.end(); ++it)
  {
    Link &link = (*it).second;
    if (link.logic_props.find(logicname) != link.logic_props.end())
    {
      linknames.push_back(link.name);
    }
  }
  return linknames;
} /* LinkManager::getLinkNames */


void LinkManager::linkTimeout(Async::Timer *t, Link *link)
{
  if (link->is_activated && !link->default_active)
  {
    deactivateLink(*link);
  }

  if (!link->is_activated && link->default_active)
  {
    activateLink(*link);
  }
} /* LinkManager::linkTimeout */


void LinkManager::logicIdleStateChanged(bool is_idle, const LogicBase *logic)
{
  /* cout << "### LinkManager::logicIdleStateChanged:"
       << " is_idle=" << is_idle
       << " logic_name=" << logic->name()
       << endl;
  */

  if (!all_logics_started)
  {
    return;
  }

    // We need all linknames where the logic is included in
  vector<string> link_names = getLinkNames(logic->name());

    // Loop through all links associated with the logic to see if we should
    // enable or disable any timeout timers.
  for (std::vector<string>::iterator lnit = link_names.begin();
       lnit != link_names.end();
       ++lnit)
  {
    const string &link_name = *lnit;
    LinkMap::iterator lmit = links.find(link_name);
    assert(lmit != links.end());
    Link &link = (*lmit).second;
    
      // Check if the logic that updated its idle state is defined as a
      // "auto_activate" logic. If not idle, all logics of the link shall
      // be connected if they were disconnected before.
      // FIXME: We now activate the link even on announcements since that
      // changes the idle status of a logic core. Probably not what we want.
    if (!is_idle && !link.is_activated)
    {
      StrSet::iterator acit = link.auto_activate.find(logic->name());
      if (acit != link.auto_activate.end())
      {
        /* cout << "### Activating link " << link_name
             << " due to AUTOCONNECT_ON_SQL from " << logic->name() << endl;
        */
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
  for (LogicPropMap::iterator cpit = link.logic_props.begin();
       cpit != link.logic_props.end();
       ++cpit)
  {
    const string &logic_name = (*cpit).first;
    LogicMap::iterator lmit = logic_map.find(logic_name);
    assert(lmit != logic_map.end());
    LogicBase *logic = (*lmit).second.logic;
    all_logics_idle &= logic->isIdle();
  }

  if (all_logics_idle && (link.is_activated != link.default_active))
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
