/**
@file	 LinkManager.cpp
@brief   Contains the manager to link different logics together
         implemented as singleton
@author  Adi Bier (DL1HRC) / Christian Stussak (University of Halle/Saale)
         Tobias Blomberg / SM0SVX
@date	 2011-11-24

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

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <algorithm>
#include <iterator>
#include <regex>


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
#include <AsyncAudioValve.h>
#include <AsyncAudioMixer.h>
#include <AsyncAudioAmp.h>


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

bool LinkManager::initialize(Async::Config &cfg,
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
          cerr << "*** ERROR: Duplicate logic \"" << logic_name
               << "\" specified in link \"" << link.name << "\".\n";
          init_ok = false;
        }
      }
      else
      {
        cerr << "*** ERROR: Bad configuration for " << link.name
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

    int timeout = -1;
    cfg.getValue(link.name, "TIMEOUT", timeout);

      // Automatically activate the link, if one (or more) logics
      // has activity, e.g. squelch open, announcement activity etc.
    string activate_on_activity;
    if (cfg.getValue(link.name, "AUTOACTIVATE_ON_SQL", activate_on_activity))
    {
      std::cerr << "*** WARNING: Configuration variable " << link.name
                << "/AUTOACTIVATE_ON_SQL has been renamed to "
                   "ACTIVATE_ON_ACTIVITY"
                << std::endl;
      cfg.setValue(link.name, "ACTIVATE_ON_ACTIVITY", activate_on_activity);
    }
    if (cfg.getValue(link.name, "ACTIVATE_ON_ACTIVITY", activate_on_activity))
    {
      SvxLink::splitStr(link.auto_activate, activate_on_activity, ",");

        // An automatically connected link should be disconnected after a
        // while so the TIMEOUT configuration variable must be set.
      if (timeout <= 0)
      {
        std::cerr << "*** WARNING: missing param " << link.name
                  << "/TIMEOUT=??, set to default (30 sec)" << std::endl;
        timeout = 30;
      }
    }

    if (cfg.getValue(link.name, "ACTIVATE_ON_TG", link.auto_activate_on_tg))
    {
        // An automatically connected link should be disconnected after a
        // while so the TIMEOUT configuration variable must be set.
      if (timeout <= 0)
      {
        std::cerr << "*** WARNING: Missing configuration " << link.name
                  << "/TIMEOUT=??, setting to default (30 sec)" << std::endl;
        timeout = 30;
      }
    }

    if (timeout > 0)
    {
      link.timeout_timer = new Timer(1000 * timeout);
      link.timeout_timer->setEnable(false);
      link.timeout_timer->expired.connect(sigc::bind(
          mem_fun(*LinkManager::instance(), &LinkManager::linkTimeout),
          &link));
    }

    cfg.getValue(link.name, "DEFAULT_ACTIVE", link.default_active);

      // Parse AUDIO_MODE configuration (default: FIRST)
    std::string audio_mode_str;
    if (cfg.getValue(link.name, "AUDIO_MODE", audio_mode_str))
    {
      if (audio_mode_str == "FIRST")
      {
        link.audio_mode = LinkAudioMode::FIRST;
      }
      else if (audio_mode_str == "MIX")
      {
        link.audio_mode = LinkAudioMode::MIX;
      }
      else if (audio_mode_str == "DUCK")
      {
        link.audio_mode = LinkAudioMode::DUCK;
      }
      else if (audio_mode_str == "PRIORITY")
      {
        link.audio_mode = LinkAudioMode::PRIORITY;
      }
      else
      {
        std::cerr << "*** WARNING: Unknown AUDIO_MODE \"" << audio_mode_str
                  << "\" in link " << link.name
                  << ". Valid options: FIRST, MIX, DUCK, PRIORITY. Using FIRST."
                  << std::endl;
      }
    }

      // Parse DUCK_LEVEL_DB configuration (default: -12 dB)
    cfg.getValue(link.name, "DUCK_LEVEL_DB", link.duck_level_db);

      // Parse PRIORITY_MUTE_DB configuration (default: -60 dB)
    cfg.getValue(link.name, "PRIORITY_MUTE_DB", link.priority_mute_db);
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
    // other sinks. Used in FIRST mode.
  AudioSelector *selector = new AudioSelector;
  selector->registerSink(logic->logicConIn());

    // Create a mixer for MIX mode. The mixer output will be connected
    // to the logic input when MIX mode connections are activated.
  AudioMixer *mixer = new AudioMixer;

    // Register the new logic sink
  sinks[logic->name()].sink = logic->logicConIn();
  sinks[logic->name()].selector = selector;
  sinks[logic->name()].mixer = mixer;

    // Now create a connection from the new logic source to each sink.
  for (SinkMap::iterator it=sinks.begin(); it != sinks.end(); ++it)
  {
      // Passthrough for FIRST mode (selector path)
    AudioPassthrough *connector = new AudioPassthrough;
    splitter->addSink(connector, true);
    AudioSelector *other_selector = (*it).second.selector;
    other_selector->addSource(connector);
    (*it).second.connectors[logic->name()] = connector;

      // Valve + Amp for MIX/DUCK mode (mixer path)
    AudioValve *valve = new AudioValve;
    valve->setOpen(false);  // Initially closed
    splitter->addSink(valve, true);
    AudioAmp *amp = new AudioAmp;
    amp->setGain(0);  // Normal gain initially
    valve->registerSink(amp, true);
    (*it).second.mixer->addSource(amp);
    (*it).second.valves[logic->name()] = valve;
    (*it).second.amps[logic->name()] = amp;
  }

    // Now create a connection from each existing logic source to the new sink.
  for (SourceMap::iterator it=sources.begin(); it!=sources.end(); ++it)
  {
      // Passthrough for FIRST mode (selector path)
    AudioPassthrough *connector = new AudioPassthrough;
    (*it).second.splitter->addSink(connector, true);
    selector->addSource(connector);
    sinks[logic->name()].connectors[(*it).first] = connector;

      // Valve + Amp for MIX/DUCK mode (mixer path)
    AudioValve *valve = new AudioValve;
    valve->setOpen(false);  // Initially closed
    (*it).second.splitter->addSink(valve, true);
    AudioAmp *amp = new AudioAmp;
    amp->setGain(0);  // Normal gain initially
    valve->registerSink(amp, true);
    mixer->addSource(amp);
    sinks[logic->name()].valves[(*it).first] = valve;
    sinks[logic->name()].amps[(*it).first] = amp;
  }

    // Create new object containing metadata for this logic core
  LogicInfo logic_info(logic);

    // Keep track of the newly added logics idle state so that we can start
    // and stop timeout timers.
  logic_info.idle_state_changed_con = logic->idleStateChanged.connect(
      sigc::bind(mem_fun(*this, &LinkManager::logicIdleStateChanged), logic));

    // Connect signals that convey information to other linked logics
  logic_info.received_tg_update_con = logic->receivedTgUpdated.connect(
      sigc::bind<0>(
        sigc::mem_fun(*this, &LinkManager::onReceivedTgUpdated), logic));
  logic_info.received_publish_state_event_con = logic->publishStateEvent.connect(
      sigc::bind<0>(
        sigc::mem_fun(*this, &LinkManager::onPublishStateEvent), logic));

    // Connect squelch state signal for DUCK mode
  logic_info.squelch_state_changed_con = logic->squelchStateChanged.connect(
      sigc::bind(
        sigc::mem_fun(*this, &LinkManager::onSquelchStateChanged), logic));

    // Add the logic core to the logic map
  logic_map.emplace(logic->name(), logic_info);

    // Create command objects associated with this logic
    // FIXME: We should not reference to a specific logic core type in this
    //        class
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
            std::cerr << "*** WARNING: Can not setup command "
                      << logic_props.cmd << " for the logic " << logic->name()
                      << std::endl;
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

    // Disconnect the sigc signals that was connected when the
    // logic was first registered.
  assert(logic_info.idle_state_changed_con.connected());
  logic_info.idle_state_changed_con.disconnect();
  assert(logic_info.received_tg_update_con.connected());
  logic_info.received_tg_update_con.disconnect();
  assert(logic_info.received_publish_state_event_con.connected());
  logic_info.received_publish_state_event_con.disconnect();
  if (logic_info.squelch_state_changed_con.connected())
  {
    logic_info.squelch_state_changed_con.disconnect();
  }

    // Delete the logic source splitter and all connections associated with it
  AudioSplitter *splitter = sources[logic->name()].splitter;
  for (SinkMap::iterator smit=sinks.begin(); smit!=sinks.end(); ++smit)
  {
    SinkInfo &sink_info = (*smit).second;

      // Remove passthrough from selector
    ConMap::iterator cmit = sink_info.connectors.find(logic->name());
    assert(cmit != sink_info.connectors.end());
    AudioPassthrough *connector = (*cmit).second;
    sink_info.selector->removeSource(connector);
    sink_info.connectors.erase(logic->name());
    splitter->removeSink(connector);
    //delete connector;

      // Close and remove valve and amp for mixer
    ValveMap::iterator vmit = sink_info.valves.find(logic->name());
    if (vmit != sink_info.valves.end())
    {
      vmit->second->setOpen(false);
      splitter->removeSink(vmit->second);
      delete vmit->second;
      sink_info.valves.erase(vmit);
    }

      // Remove amp for mixer
    AmpMap::iterator amit = sink_info.amps.find(logic->name());
    if (amit != sink_info.amps.end())
    {
      delete amit->second;
      sink_info.amps.erase(amit);
    }
  }
  delete splitter;
  sources.erase(logic->name());

    // Delete the logic sink and all connections associated with it
  SinkInfo &sink_to_delete = sinks[logic->name()];
  AudioSelector *selector = sink_to_delete.selector;
  AudioMixer *mixer = sink_to_delete.mixer;

    // Clean up connectors (passthroughs for selector)
  ConMap &cons = sink_to_delete.connectors;
  for (ConMap::iterator cmit = cons.begin(); cmit != cons.end(); ++cmit)
  {
    AudioPassthrough *connector = (*cmit).second;
    selector->removeSource(connector);
    const string &source_name = (*cmit).first;
    sources[source_name].splitter->removeSink(connector);
    //delete connector;
  }

    // Clean up valves (for mixer)
  ValveMap &valves = sink_to_delete.valves;
  for (ValveMap::iterator vmit = valves.begin(); vmit != valves.end(); ++vmit)
  {
    AudioValve *valve = vmit->second;
    valve->setOpen(false);
    const string &source_name = vmit->first;
    sources[source_name].splitter->removeSink(valve);
    delete valve;
  }

    // Clean up amps (for mixer DUCK mode)
  AmpMap &amps = sink_to_delete.amps;
  for (AmpMap::iterator amit = amps.begin(); amit != amps.end(); ++amit)
  {
    delete amit->second;
  }

  delete selector;
  delete mixer;
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
        std::cerr << "*** WARNING: Logic " << logic_name
                  << " has been specified in logic link " << link.name
                  << " but that logic is missing. Removing logic from link."
                  << std::endl;
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
      activateLink(link, "DEFAULT_ACTIVE");
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
  if (subcmd == "") // Disconnecting Link1 <-X-> Link2
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
  else //if (subcmd == "1") // Connecting Logic1 <---> Logic2 (two ways)
  {
    if (!link.is_activated)
    {
      activateLink(link, "DTMF command");
      ss << "activating_link \"";
      ss << logic_props.announcement_name + "\"";
    }
    //else
    //{
    //  ss << "link_already_active \"";
    //}
    //ss << logic_props.announcement_name + "\"";
    sendCmdToLogics(link, logic, subcmd);
  }
  //else
  //{
  //  ss << "unknown_command " << logic_props.cmd << subcmd;
  //}
  return ss.str();
} /* LinkManager::cmdReceived */


LogicBase *LinkManager::currentTalkerFor(const std::string& logic_name)
{
  SinkInfo& sink = sinks[logic_name];

    // In MIX mode, there's no single current talker
  if (sink.mixer->isRegistered())
  {
    return nullptr;
  }

    // FIRST mode - find the selected source
  AudioSource *selected = sink.selector->selectedSource();
  const ConMap& con_map = sink.connectors;
  for (ConMap::const_iterator it = con_map.begin(); it != con_map.end(); ++it)
  {
    if (it->second == selected)
    {
      //cout << "### Selected source: " << it->first << endl;
      return logic_map.at(it->first).logic;
    }
  }
  return nullptr;
} /* LinkManager::currentTalkerFor */


void LinkManager::setLogicMute(const LogicBase *logic, bool mute)
{
  LogicInfo &info = logic_map.at(logic->name());
  if (mute != info.is_muted)
  {
    info.is_muted = mute;
    updateConnections();
    for (const auto& link_name : getLinkNames(logic->name()))
    {
      checkTimeoutTimer(links.at(link_name));
    }
  }
} /* LinkManager::setLogicMute */


void LinkManager::playFile(LogicBase *src_logic, const std::string& path)
{
  const Async::AudioSelector *selector = sinks[src_logic->name()].selector;
  const ConMap& con_map = sinks[src_logic->name()].connectors;
  for (ConMap::const_iterator it = con_map.begin(); it != con_map.end(); ++it)
  {
    const std::string& logic_name = it->first;
    const Async::AudioSource *con = it->second;
    LogicBase *logic = logic_map.at(logic_name).logic;
    if ((logic != src_logic) && (selector->autoSelectEnabled(con)))
    {
      logic->playFile(path);
    }
  }
} /* LinkManager::playFile */


void LinkManager::playSilence(LogicBase *src_logic, int length)
{
  const Async::AudioSelector *selector = sinks[src_logic->name()].selector;
  const ConMap& con_map = sinks[src_logic->name()].connectors;
  for (ConMap::const_iterator it = con_map.begin(); it != con_map.end(); ++it)
  {
    const std::string& logic_name = it->first;
    const Async::AudioSource *con = it->second;
    LogicBase *logic = logic_map.at(logic_name).logic;
    if ((logic != src_logic) && (selector->autoSelectEnabled(con)))
    {
      logic->playSilence(length);
    }
  }
} /* LinkManager::playSilence */


void LinkManager::playTone(LogicBase *src_logic, int fq, int amp, int len)
{
  const Async::AudioSelector *selector = sinks[src_logic->name()].selector;
  const ConMap& con_map = sinks[src_logic->name()].connectors;
  for (ConMap::const_iterator it = con_map.begin(); it != con_map.end(); ++it)
  {
    const std::string& logic_name = it->first;
    const Async::AudioSource *con = it->second;
    LogicBase *logic = logic_map.at(logic_name).logic;
    if ((logic != src_logic) && (selector->autoSelectEnabled(con)))
    {
      logic->playTone(fq, amp, len);
    }
  }
} /* LinkManager::playTone */


void LinkManager::playDtmf(LogicBase *src_logic, const std::string& digits, int amp, int len)
{
  const Async::AudioSelector *selector = sinks[src_logic->name()].selector;
  const ConMap& con_map = sinks[src_logic->name()].connectors;
  for (ConMap::const_iterator it = con_map.begin(); it != con_map.end(); ++it)
  {
    const std::string& logic_name = it->first;
    const Async::AudioSource *con = it->second;
    LogicBase *logic = logic_map.at(logic_name).logic;
    if ((logic != src_logic) && (selector->autoSelectEnabled(con)))
    {
      logic->playDtmf(digits, amp, len);
    }
  }
} /* LinkManager::playDtmf */



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
          const std::string& l1_name = (*oit).first;
          const std::string& l2_name = (*iit).first;
          const LogicInfo &l1_info = logic_map.at(l1_name);
          const LogicInfo &l2_info = logic_map.at(l2_name);
          if ((l1_name != l2_name) && !l1_info.is_muted && !l2_info.is_muted)
          {
            want.insert(make_pair(l1_name, l2_name));
            want.insert(make_pair(l2_name, l1_name));
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


void LinkManager::updateConnections(void)
{
    // Get the wanted logic connections based on which links that are activated
  LogicConSet want;
  wantedConnections(want);

    // Calculate which of the current connections should be broken.
    // This is easily done with a simple difference set operation.
    // After this operation the "to_disconnect" variable will contain
    // the set of logic connections that have to be broken.
  LogicConSet to_disconnect;
  set_difference(current_cons.begin(), current_cons.end(),
                 want.begin(), want.end(),
                 inserter(to_disconnect, to_disconnect.end()));

  for (auto it = to_disconnect.begin(); it != to_disconnect.end(); ++it)
  {
    const string &src_name = it->first;
    const string &sink_name = it->second;
    SinkInfo &sink = sinks.at(sink_name);

      // Disconnect the audio path from source logic to sink logic
    sink.selector->disableAutoSelect(sink.connectors.at(src_name));

      // Close the mixer valve for this connection
    sink.valves.at(src_name)->setOpen(false);

      // Delete the link connect information
    current_cons.erase(*it);
  }

    // Calculate the difference between the wanted connection set and the
    // current connection set. This is easily done using the difference set
    // operation. After this operation the "to_connect" variable will contain
    // the set of logic connections that have to be established.
  LogicConSet to_connect;
  set_difference(want.begin(), want.end(),
                 current_cons.begin(), current_cons.end(),
                 inserter(to_connect, to_connect.end()));

    // Establish missing connections
  for (auto it = to_connect.begin(); it != to_connect.end(); ++it)
  {
    const string &src_name = it->first;
    const string &sink_name = it->second;
    SinkInfo &sink = sinks.at(sink_name);

      // Get the effective audio mode for this connection
    LinkAudioMode mode = getEffectiveMode(src_name, sink_name);

    if (mode == LinkAudioMode::FIRST)
    {
        // FIRST mode: use selector, close mixer valve
      sink.selector->enableAutoSelect(sink.connectors.at(src_name), 0);
      sink.valves.at(src_name)->setOpen(false);
    }
    else // MIX, DUCK, or PRIORITY mode - all use mixer
    {
        // MIX/DUCK/PRIORITY mode: disable selector, open mixer valve
      sink.selector->disableAutoSelect(sink.connectors.at(src_name));
      sink.valves.at(src_name)->setOpen(true);
    }

      // Store all connections in "current_cons" (current connections)
    current_cons.insert(*it);
  }

    // Update mixer routing based on active MIX/DUCK/PRIORITY connections
  updateMixerRouting();
} /* LinkManager::updateConnections */


void LinkManager::activateLink(Link &link, const std::string& reason)
{
  if (!link.is_activated)
  {
    std::cout << "Activating link '" << link.name << "'";
    if (!reason.empty())
    {
      std::cout << " due to " << reason;
    }
    std::cout << std::endl;
    link.is_activated = true;
    updateConnections();
    checkTimeoutTimer(link);
  }
} /* LinkManager::activateLink */


void LinkManager::deactivateLink(Link &link, const std::string& reason)
{
  if (link.is_activated)
  {
    std::cout << "Deactivating link " << link.name;
    if (!reason.empty())
    {
      std::cout << " due to " << reason;
    }
    std::cout << std::endl;
    link.is_activated = false;
    updateConnections();
    checkTimeoutTimer(link);
  }
} /* LinkManager::deactivateLink */


void LinkManager::sendCmdToLogics(Link& link, LogicBase* src_logic,
                                  const std::string& cmd)
{
  for (const auto& props : link.logic_props)
  {
    const string &logic_name = props.first;
    const LogicInfo &logic_info = logic_map.at(logic_name);
    if ((logic_info.logic != src_logic) && !logic_info.is_muted)
    {
      logic_info.logic->remoteCmdReceived(src_logic, cmd);
    }
  }
} /* LinkManager::sendCmdToLogics */


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
    activateLink(*link, "TIMEOUT on default activated link");
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

    // We need all linknames where the logic is included
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
        std::ostringstream ss;
        ss << "ACTIVATE_ON_ACTIVITY from logic '"
           << logic->name() << "'";
        activateLink(link, ss.str());
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
  for (const auto& prop : link.logic_props)
  {
    const std::string &logic_name = prop.first;
    const LogicInfo &logic_info = logic_map.at(logic_name);
    all_logics_idle &= logic_info.is_muted || logic_info.logic->isIdle();
  }

  link.timeout_timer->setEnable(
      all_logics_idle && (link.is_activated != link.default_active));
} /* LinkManager::checkTimeoutTimer */


void LinkManager::onReceivedTgUpdated(LogicBase *src_logic, uint32_t tg)
{
  //cout << "### LinkManager::onReceivedTgUpdated: logic=" << src_logic->name()
  //     << "  tg=" << tg << endl;


    // Loop through all links associated with the logic to see if we should
    // auto activate any links
  for (auto& link_spec : links)
  {
    Link &link = link_spec.second;

#if 1
    std::ostringstream tgss;
    tgss << tg;
    auto aait = link.auto_activate_on_tg.find(src_logic->name());
    if (!link.is_activated &&
        //(link.logic_props.find( != link.logic_props.end()) &&
        aait != link.auto_activate_on_tg.end() &&
        std::regex_match(tgss.str(), std::regex(aait->second)))
    {
      std::ostringstream ss;
      ss << "ACTIVATE_ON_TG from logic '"
         << src_logic->name() << "'"
         << " which has activated talkgroup " << tgss.str();
      activateLink(link, ss.str());
    }
#endif
  }


  const Async::AudioSelector *selector = sinks[src_logic->name()].selector;
  const ConMap& con_map = sinks[src_logic->name()].connectors;
  for (ConMap::const_iterator it = con_map.begin(); it != con_map.end(); ++it)
  {
    const std::string& logic_name = it->first;
    const Async::AudioSource *con = it->second;
    LogicBase *logic = logic_map.at(logic_name).logic;
    if ((logic != src_logic) && (selector->autoSelectEnabled(con)))
    {
      logic->remoteReceivedTgUpdated(src_logic, tg);
    }
  }
} /* LinkManager::onReceivedTgUpdated */


void LinkManager::onPublishStateEvent(LogicBase *src_logic,
    const std::string& event_name, const std::string& msg)
{
  //cout << "### LinkManager::onPublishStateEvent: logic=" << src_logic->name()
  //     << "  event_name=" << event_name
  //     << "  msg=" << msg
  //     << endl;
  const Async::AudioSelector *selector = sinks[src_logic->name()].selector;
  const ConMap& con_map = sinks[src_logic->name()].connectors;
  for (ConMap::const_iterator it = con_map.begin(); it != con_map.end(); ++it)
  {
    const std::string& logic_name = it->first;
    const Async::AudioSource *con = it->second;
    LogicBase *logic = logic_map.at(logic_name).logic;
    if ((logic != src_logic) && (selector->autoSelectEnabled(con)))
    {
      logic->remoteReceivedPublishStateEvent(src_logic, event_name, msg);
    }
  }
} /* LinkManager::onPublishStateEvent */


LinkAudioMode LinkManager::getEffectiveMode(const std::string& src_name,
                                             const std::string& sink_name)
{
    // Determine the effective audio mode for a connection.
    // DUCK takes precedence over MIX, and both use the mixer path.
  LinkAudioMode mode = LinkAudioMode::FIRST;

  for (const auto& link_pair : links)
  {
    const Link& link = link_pair.second;
    if (!link.is_activated)
    {
      continue;
    }

    bool has_src = link.logic_props.count(src_name) > 0;
    bool has_sink = link.logic_props.count(sink_name) > 0;

    if (has_src && has_sink)
    {
        // PRIORITY takes highest precedence, then DUCK, then MIX
      if (link.audio_mode == LinkAudioMode::PRIORITY)
      {
        return LinkAudioMode::PRIORITY;
      }
      else if (link.audio_mode == LinkAudioMode::DUCK &&
               mode != LinkAudioMode::PRIORITY)
      {
        mode = LinkAudioMode::DUCK;
      }
      else if (link.audio_mode == LinkAudioMode::MIX &&
               mode != LinkAudioMode::PRIORITY && mode != LinkAudioMode::DUCK)
      {
        mode = LinkAudioMode::MIX;
      }
    }
  }
  return mode;
} /* LinkManager::getEffectiveMode */


void LinkManager::updateMixerRouting(void)
{
    // Connect/disconnect mixer to logic input based on active MIX connections.
    // If a sink has any open mixer valves, route the mixer output to the logic.
    // Otherwise, route the selector output to the logic.
  for (auto& sink_pair : sinks)
  {
    SinkInfo& sink = sink_pair.second;

      // Check if this sink has any MIX-mode connections active (any valve open)
    bool has_mix_connection = false;
    for (const auto& valve_pair : sink.valves)
    {
      if (valve_pair.second->isOpen())
      {
        has_mix_connection = true;
        break;
      }
    }

      // Route mixer or selector to logic input
    if (has_mix_connection)
    {
        // Disconnect selector, connect mixer
      if (sink.selector->isRegistered())
      {
        sink.selector->unregisterSink();
      }
      if (!sink.mixer->isRegistered())
      {
        sink.mixer->registerSink(sink.sink);
      }
    }
    else
    {
        // Disconnect mixer, connect selector
      if (sink.mixer->isRegistered())
      {
        sink.mixer->unregisterSink();
      }
      if (!sink.selector->isRegistered())
      {
        sink.selector->registerSink(sink.sink);
      }
    }
  }
} /* LinkManager::updateMixerRouting */


void LinkManager::onSquelchStateChanged(bool is_open, LogicBase *logic)
{
    // Update squelch state tracking
  LogicMap::iterator it = logic_map.find(logic->name());
  if (it != logic_map.end())
  {
    it->second.squelch_open = is_open;
  }

    // Update ducking for this logic's incoming connections
  updateDuckingForSink(logic->name(), is_open);

    // Update priority preemption for all sinks
  for (auto& sink_pair : sinks)
  {
    updatePriorityForSink(sink_pair.first);
  }
} /* LinkManager::onSquelchStateChanged */


void LinkManager::updateDuckingForSink(const std::string& sink_name,
                                        bool local_squelch_open)
{
  SinkMap::iterator sink_it = sinks.find(sink_name);
  if (sink_it == sinks.end())
  {
    return;
  }

  SinkInfo& sink = sink_it->second;

  for (auto& amp_pair : sink.amps)
  {
    const std::string& src_name = amp_pair.first;
    AudioAmp* amp = amp_pair.second;

      // Check if this connection uses DUCK mode
    LinkAudioMode mode = getEffectiveMode(src_name, sink_name);

    if (mode == LinkAudioMode::DUCK)
    {
      if (local_squelch_open)
      {
          // Duck incoming audio when local squelch is open
        float duck_level = getEffectiveDuckLevel(src_name, sink_name);
        amp->setGain(duck_level);
      }
      else
      {
          // Restore normal gain when local squelch closes
        amp->setGain(0);
      }
    }
  }
} /* LinkManager::updateDuckingForSink */


float LinkManager::getEffectiveDuckLevel(const std::string& src_name,
                                          const std::string& sink_name)
{
  float duck_level = -12.0f;  // Default

  for (const auto& link_pair : links)
  {
    const Link& link = link_pair.second;
    if (!link.is_activated) continue;

    bool has_src = link.logic_props.count(src_name) > 0;
    bool has_sink = link.logic_props.count(sink_name) > 0;

    if (has_src && has_sink && link.audio_mode == LinkAudioMode::DUCK)
    {
      duck_level = link.duck_level_db;
      break;
    }
  }
  return duck_level;
} /* LinkManager::getEffectiveDuckLevel */


bool LinkManager::isSourceFromPriorityLink(const std::string& src_name,
                                            const std::string& sink_name)
{
    // Check if this source-sink pair is in a PRIORITY mode link
  for (const auto& link_pair : links)
  {
    const Link& link = link_pair.second;
    if (!link.is_activated) continue;
    if (link.audio_mode != LinkAudioMode::PRIORITY) continue;

    if (link.logic_props.count(src_name) > 0 &&
        link.logic_props.count(sink_name) > 0)
    {
      return true;
    }
  }
  return false;
} /* LinkManager::isSourceFromPriorityLink */


bool LinkManager::isPrioritySourceActive(const std::string& sink_name)
{
    // Check if any PRIORITY source is currently transmitting to this sink
  SinkMap::iterator sink_it = sinks.find(sink_name);
  if (sink_it == sinks.end()) return false;

  for (const auto& amp_pair : sink_it->second.amps)
  {
    const std::string& src_name = amp_pair.first;

      // Check if this source's squelch is open
    LogicMap::iterator it = logic_map.find(src_name);
    if (it == logic_map.end() || !it->second.squelch_open) continue;

      // Check if this source is from a PRIORITY link to this sink
    if (isSourceFromPriorityLink(src_name, sink_name))
    {
      return true;
    }
  }
  return false;
} /* LinkManager::isPrioritySourceActive */


float LinkManager::getEffectivePriorityMuteLevel(const std::string& src_name,
                                                  const std::string& sink_name)
{
  for (const auto& link_pair : links)
  {
    const Link& link = link_pair.second;
    if (!link.is_activated) continue;

      // Find any PRIORITY link that includes this sink
    if (link.audio_mode == LinkAudioMode::PRIORITY &&
        link.logic_props.count(sink_name) > 0)
    {
      return link.priority_mute_db;
    }
  }
  return -60.0f;
} /* LinkManager::getEffectivePriorityMuteLevel */


void LinkManager::updatePriorityForSink(const std::string& sink_name)
{
  SinkMap::iterator sink_it = sinks.find(sink_name);
  if (sink_it == sinks.end()) return;

  SinkInfo& sink = sink_it->second;
  bool priority_active = isPrioritySourceActive(sink_name);

    // Check if sink logic's squelch is open (for DUCK interaction)
  bool sink_squelch_open = false;
  LogicMap::iterator sink_logic_it = logic_map.find(sink_name);
  if (sink_logic_it != logic_map.end())
  {
    sink_squelch_open = sink_logic_it->second.squelch_open;
  }

    // Handle all connections (both FIRST mode via selector and mixer-based modes)
  for (auto& con_pair : sink.connectors)
  {
    const std::string& src_name = con_pair.first;
    LinkAudioMode mode = getEffectiveMode(src_name, sink_name);
    bool is_priority_src = isSourceFromPriorityLink(src_name, sink_name);

    if (mode == LinkAudioMode::FIRST)
    {
        // FIRST mode: use selector enable/disable for priority preemption
      if (priority_active && !is_priority_src)
      {
          // Disable this source in the selector when preempted by priority
        sink.selector->disableAutoSelect(con_pair.second);
      }
      else
      {
          // Re-enable in selector (normal operation)
        sink.selector->enableAutoSelect(con_pair.second, 0);
      }
    }
    else
    {
        // MIX/DUCK/PRIORITY mode: use amp gain for priority preemption
      AudioAmp* amp = sink.amps.at(src_name);

        // Determine base gain level (may be ducked if sink squelch is open)
      float base_gain = 0;
      if (sink_squelch_open && (mode == LinkAudioMode::DUCK || mode == LinkAudioMode::PRIORITY))
      {
        base_gain = getEffectiveDuckLevel(src_name, sink_name);
      }

      if (priority_active && !is_priority_src)
      {
          // A PRIORITY source is active and this is NOT a priority source - mute it
        float mute_level = getEffectivePriorityMuteLevel(src_name, sink_name);
        amp->setGain(mute_level);
      }
      else
      {
          // No priority preemption (or this IS the priority source) - use base gain
        amp->setGain(base_gain);
      }
    }
  }
} /* LinkManager::updatePriorityForSink */


/*
 * This file has not been truncated
 */
