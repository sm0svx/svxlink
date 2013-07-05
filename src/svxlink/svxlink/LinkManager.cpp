/**
@file	 LinkManager.cpp
@brief   Contains the manager to link different logics together
         implemented as singleton
@author  Adi Bier (DL1HRC) / Christian Stussak (University of Halle/Saale)
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
                             const std::string &cfg_name)
{
    // Check if already initialized
  if (LinkManager::_instance->has_instance())
  {
    return false;
  }
  LinkManager::_instance = new LinkManager;

  bool init_ok = true;

    // Split the line into an array of section names
    // LINKS=Link1,Link2,Link3,...
    // cfg_name == [GLOBAL]/LINKS
  StrList linklist;
  SvxLink::splitStr(linklist, cfg_name, ",");

    // Loop to create a link object for each single linkdefinition
    // in the "LINKS=Link1, Link2, Link3" set
  for (StrList::const_iterator name = linklist.begin();
       name != linklist.end();
       name++)
  {
    LinkSet tl_cmd;

      // The link name is the same as the config section name
    tl_cmd.name = *name;

      // Logic1:70:name1,Logic2:71:name2,...
    string connect_logics;
    cfg.getValue(*name, "CONNECT_LOGICS", connect_logics);

    vector<string> logic_list;
    SvxLink::splitStr(logic_list, connect_logics, ",");
    if (logic_list.size() < 2)
    {
      cerr << "*** ERROR: " << *name << "/CONNECT_LOGICS=? You must "
           << "have at least two logics to connect.\n";
      init_ok = false;
    }

      // Configure the list of logics to connect
      // CONNECT_LOGICS=Logic1:70:Name1,Logic2:71,Logic3:72:Name2,...
    for(vector<string>::iterator it = logic_list.begin();
        it != logic_list.end();
        ++it)
    {
      vector<string> logic_spec;
      SvxLink::splitStr(logic_spec, *it, ":");
      logic_spec.resize(3);
      if (logic_spec.at(0).size() > 0)
      {
        string logic_name = logic_spec.at(0);
        LinkProperties link_props;
        link_props.logic_cmd =  logic_spec.at(1);
        link_props.announcement_name = logic_spec.at(2);
        tl_cmd.link_cname.insert(make_pair(logic_name, link_props));
      }
      else
      {
        cout << "*** ERROR: Bad configuration for " << *name
             << "/CONNECT_LOGICS=" << connect_logics << ". Legal format: "
             << "<logic name>:<command>:<announcement name>,...\n";
        init_ok = false;
      }
    }

    cfg.getValue(*name, "TIMEOUT", tl_cmd.timeout);
    tl_cmd.timeout *= 1000;

      // Automatically activate the link, if one (or more) logics
      // has activity, e.g. squelch open.
    string autoconnect_on_sql;
    if (cfg.getValue(*name, "AUTOCONNECT_ON_SQL", autoconnect_on_sql))
    {
      SvxLink::splitStr(tl_cmd.auto_connect, autoconnect_on_sql, ",");

        // An automatically connected link should be disconnected after a
        // while so the TIMEOUT configuration variable must be set.
      if (tl_cmd.timeout == 0)
      {
        cout << "*** WARNING: missing param " << *name
             << "/TIMEOUT=??, set to default (30 sec)\n";
        tl_cmd.timeout = 30000;
      }
    }

    cfg.getValue(*name, "DEFAULT_CONNECT", tl_cmd.default_connect);
    cfg.getValue(*name, "NO_DISCONNECT", tl_cmd.no_disconnect);

    LinkManager::instance()->link_cfg.insert(make_pair(tl_cmd.name, tl_cmd));
  }

  if(!init_ok)
  {
    delete LinkManager::_instance;
    LinkManager::_instance = 0;
  }

  return init_ok;

} /* LinkManager::initialize */


/**
 * @brief Register the audio source (source_name , audioname)
 */
void LinkManager::addSource(const string& source_name,
                            AudioSource *logic_con_out)
{
  if (sourceIsAdded(source_name))
  {
    return;
  }

  AudioSplitter *splitter = new AudioSplitter;
  logic_con_out->registerSink(splitter);

  sources[source_name].source = logic_con_out;
  sources[source_name].splitter = splitter;

  for (SinkMap::iterator it=sinks.begin(); it != sinks.end(); ++it)
  {
    AudioPassthrough *connector = new AudioPassthrough;
    splitter->addSink(connector);
    AudioSelector *sel = (*it).second.selector;
    sel->addSource(connector);
    (*it).second.connectors[source_name] = connector;
  }
} /* LinkManager::addSource */


void LinkManager::addSink(const string &sink_name, AudioSink *logic_con_in)
{
  if (sinkIsAdded(sink_name))
  {
    return;
  }

  AudioSelector *selector = new AudioSelector;
  selector->registerSink(logic_con_in);

  sinks[sink_name].sink = logic_con_in;
  sinks[sink_name].selector = selector;

  for (SourceMap::iterator it=sources.begin(); it!=sources.end(); ++it)
  {
    AudioPassthrough *connector = new AudioPassthrough;
    (*it).second.splitter->addSink(connector);
    selector->addSource(connector);
    sinks[sink_name].connectors[(*it).first] = connector;
  }
} /* LinkManager::addSink */


void LinkManager::deleteSource(const string& source_name)
{
  assert(sources.size() > 0);

  AudioSplitter *splitter = sources[source_name].splitter;

  for (SinkMap::iterator it=sinks.begin(); it!=sinks.end(); ++it)
  {
    assert((*it).second.connectors.size() > 0);
    AudioPassthrough *connector = (*it).second.connectors[source_name];
    (*it).second.selector->removeSource(connector);
    (*it).second.connectors.erase(source_name);
    splitter->removeSink(connector);
    delete connector;
  }

  delete splitter;
  sources.erase(source_name);
} /* LinkManager::deleteSource */


void LinkManager::deleteSink(const string& sink_name)
{
  assert(sinks.size() > 0);
  AudioSelector *selector = sinks[sink_name].selector;

  ConMap &cons = sinks[sink_name].connectors;
  for (ConMap::iterator it = cons.begin(); it != cons.end(); ++it)
  {
    string source_name = (*it).first;
    AudioPassthrough *connector = (*it).second;
    selector->removeSource(connector);
    assert(sources.size() >= 1);
    sources[source_name].splitter->removeSink(connector);
    delete connector;
  }

  delete selector;
  sinks.erase(sink_name);
} /* LinkManager::deleteSink */


/**
 * @brief Called by the logic when its operational
 *
 * Trigger from the logic, now check if DEFAULT_CONNECT is set
 * and all the logics inside the link definition can be connected
 */
void LinkManager::logicIsUp(const std::string &logicname)
{
   bool conn = false;

     // collecting names of logics that already are up
   logic_list.insert(logicname);

     // 1st we need all LINK's in that this Logic is involved
   vector<string> ln = getLinkNames(logicname);

   for (vector<string>::iterator vt = ln.begin(); vt != ln.end(); ++vt)
   {
       // 2nd we have to proof, if the Logics should beeing connected on startup
       // and if the involved logics are already up
     LinkCfg::iterator it = link_cfg.find(*vt);
     if (it != link_cfg.end())
     {
       conn = true;
       for (CName::iterator lit = (it->second).link_cname.begin();
            lit != (it->second).link_cname.end();
            ++lit)
       {
         if (find(logic_list.begin(), logic_list.end(), lit->first)
             == logic_list.end())
         {
             // one affected logic isn't already up so we skip connecting,
             // it will be called up with the next logic again
           conn = false;
         }
       }
     }

     if (conn && (it != link_cfg.end()) && (it->second.default_connect))
     {
         // if DEFAULT_CONNECT is true, connect the logics together
       connectLinks(it->second.name);
     }
   }
} /* LinkManager::logicIsUp */


/**
 * @brief Returns all commands specific for the specified logic
 */
vector<string> LinkManager::getCommands(string logicname) const
{
  vector<string> cmds;

  for (LinkCfg::const_iterator it = link_cfg.begin(); it != link_cfg.end(); ++it)
  {
    CName::const_iterator lc(it->second.link_cname.find(logicname));
    if ((lc != it->second.link_cname.end()) &&
        (atoi(lc->second.logic_cmd.c_str()) > 0))
    {
      cmds.push_back(lc->second.logic_cmd);
    }
  }
  return cmds;
} /* LinkManager::getCommands */


/**
 * @brief Reset the timeout timers for the links, that are connected
 */
void LinkManager::resetTimers(const string& logicname)
{
  /*
   * Here we have only the "logicname" but no idea in how many LINKS
   * this logic is involved so we have to check all LINKS entries
   * to reset each single timeout timer in each "linkname"
   */

    // We need all "linknames" where the "logicname" is included in
  vector<string> t_linknames = getLinkNames(logicname);

  for (vector<string>::iterator vt = t_linknames.begin();
      vt != t_linknames.end();
      ++vt)
  {
    map<string, Async::Timer*>::iterator it = timeout_timers.find(*vt);
    if (it != timeout_timers.end())
    {
        // Reset each Timer connected with the specified logic
      it->second->reset();
      it->second->setEnable(false);
    }

      // Check if a logic has sent a trigger that is defined as a
      // "auto_connect" logic, in this case all logics of a link shall
      // be connected if they are disconnected before
    vector<string>::iterator tauto_connect;
    for (tauto_connect = link_cfg[*vt].auto_connect.begin();
         tauto_connect != link_cfg[*vt].auto_connect.end();
         ++tauto_connect)
    {
      if (*tauto_connect == logicname)
      {
        cout << "connecting link " << *vt <<
          " due to AUTOCONNECT_ON_SQL from " << logicname << endl;
        connectLinks(*vt);
      }
    }
  }
} /* LinkManager::resetTimers */


/**
 * @brief Restart the to-timer
 */
void LinkManager::enableTimers(const string& logicname)
{
    // We need all "linknames" where the "logicname" is included in
  vector<string> t_linknames = getLinkNames(logicname);

  for (std::vector<string>::iterator vt = t_linknames.begin();
       vt != t_linknames.end();
       ++vt)
  {
    TimerMap::iterator it = timeout_timers.find(*vt);
    if (it != timeout_timers.end())
    {
        // Enable each Timer connected with the specified logic
      it->second->setEnable(true);
    }
  }
} /* LinkManager::enableTimers */


/**
 * @brief Called from the Logic if a command has been received
 */
string LinkManager::cmdReceived(const string &logicname, const string &cmd,
                                const string &subcmd)
{
  stringstream ss;
  int isubcmd = atoi(subcmd.c_str());

  cout << "cmdReceived " << logicname << ": " << cmd << " subcmd:"
       << subcmd << endl << flush;

  for (LinkCfg::iterator it = link_cfg.begin(); it != link_cfg.end(); ++it)
  {
    CName::iterator lcx = it->second.link_cname.find(logicname);
    if (lcx != it->second.link_cname.end())
    {
      if (lcx->second.logic_cmd == cmd)
      {
        switch(isubcmd)
        {
            // Disconnecting Link1 <-X-> Link2
          case 0:
          {
            if (it->second.no_disconnect)
            {
                // Not possible due to configuration
              ss << "deactivating_link_not_possible \""
                 << lcx->second.announcement_name << "\"";
              break;
            }
            if (!it->second.is_connected)
            {
              ss << "link_not_active \"";
              ss << lcx->second.announcement_name << "\"";
              break;
            }

            ConnectResult tconnect = disconnectLinks(it->second.name);
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
            }
            ss << lcx->second.announcement_name << "\"";
            break;
          }

            // Connecting Logic1 <---> Logic2 (two ways)
          case 1:
          {
            if ((it->second).is_connected)
            {
              ss << "link_already_active \"";
              ss << lcx->second.announcement_name << "\"";
              break;
            }

            ConnectResult tconnect = connectLinks(it->second.name);
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
            ss << lcx->second.announcement_name + "\"";
            break;
          }

          default:
          {
            ss << "unknown_command " << cmd << subcmd;
          }
        }
      }
    }
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
LinkManager::ConnectResult LinkManager::connectLinks(const string& name)
{
  ConnectResult check = ERROR;

    // Get the wanted connect matrix depending from the linkname, e.g.
    // <RepeaterLogic, MicSpkeLogic>
    // <RepeaterLogic, SimplexLogic>
    // <SimplexLogic, RepeaterLogic>
    // <SimplexLogic, MicSpkrLogic>
    // <MicSpkrLogic, RepeaterLogic>
    // <MicSpkrLogic, SimplexLogic>
  LogicConSet want = getMatrix(name);

    // Gets the difference of both, the result is the matrix to be connected
    // "is" contains the actual state of the connected logics, e.g.
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
    cout << "connecting " << it->first << " ===> " << it->second << endl;
    sinks[it->first].selector->
        enableAutoSelect(sinks[it->first].connectors[it->second], 0);

      // Store all connections in "current_cons"
    current_cons.insert(*it);
    check = OK;
  }

    // Setting up the timers if configured
  if ((link_cfg[name].timeout > 0) &&
      !link_cfg[name].no_disconnect &&
      !link_cfg[name].default_connect &&
      (timeout_timers.find(name) == timeout_timers.end()))
  {
      // The timeout_timers variable contains all running expiration timers
      // for each logic
    timeout_timers.insert(make_pair(name, new Timer(link_cfg[name].timeout)));
    timeout_timers[name]->expired.connect(
        mem_fun(*this, &LinkManager::upTimeout));
  }
  link_cfg[name].is_connected = true;

  return check;
} /* LinkManager::connectLinks */


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
  LinkCfg::iterator it = link_cfg.find(linkname);

  for (CName::iterator xi = it->second.link_cname.begin();
       xi != it->second.link_cname.end();
       ++xi)
  {
    for (CName::iterator xj = it->second.link_cname.begin();
         xj != it->second.link_cname.end();
         ++xj)
    {
      if (xj->first != xi->first)
      {
        if ((logic_list.find(xj->first) != logic_list.end()) &&
            (logic_list.find(xi->first) != logic_list.end()))
        {
          ret.insert(make_pair(xi->first, xj->first));
        }
        else
        {
          cout << "*** WARNING: One of these logics: "<< xj->first << ", "
               << xi->first << " isn't up an can not be connected.\n"
               << "**** Missing entry in your configuration " <<
               "[GLOBAL]/LINKS=???\n";
        }
      }
    }
  }
  return ret;
} /* LinkManager::getLogics */


LinkManager::LogicConSet LinkManager::getMatrix(const string& linkname)
{
  LinkCfg::iterator it = link_cfg.find(linkname);
  LogicConSet ret = getLogics(linkname);

  for (LinkCfg::iterator iu = link_cfg.begin(); iu != link_cfg.end(); ++iu)
  {
    if (iu->second.is_connected)
    {
      for (CName::iterator xj = it->second.link_cname.begin();
           xj != it->second.link_cname.end();
           ++xj)
      {
          // look in the actual connected linkset if there is a logicname
          // that exists in another already connected link definition
        CName::iterator cn = iu->second.link_cname.find(xj->first);
        if (cn != iu->second.link_cname.end())
        {
          for (CName::iterator dn = iu->second.link_cname.begin();
               dn != iu->second.link_cname.end();
               ++dn)
          {
            if (dn->first != cn->first)
            {
              for (CName::iterator xi = it->second.link_cname.begin();
                   xi != it->second.link_cname.end(); xi++)
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
  for (LinkCfg::iterator it = link_cfg.begin(); it != link_cfg.end(); ++it)
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


LinkManager::ConnectResult LinkManager::disconnectLinks(const string& name)
{
   ConnectResult check = ERROR;

   if (link_cfg.find(name) == link_cfg.end())
   {
     return check;
   }

   assert(sources.size() > 0);
   assert(sinks.size() > 0);

   LogicConSet diff = getToDisconnect(name);

   if (diff.size() == 0)
   {
     check = ALREADY_CONNECTED;
   }

   for (LogicConSet::iterator it = diff.begin(); it != diff.end(); ++it)
   {
     if ((logic_list.find(it->first) != logic_list.end()) &&
         (logic_list.find(it->second) != logic_list.end()))
     {
       cout << "disconnect " << it->first << " -X-> " << it->second << endl;
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
   link_cfg[name].is_connected = false;

     // Clear the timer
   TimerMap::iterator ti = timeout_timers.find(name);
   if (ti != timeout_timers.end())
   {
      delete ti->second;  // delete the timer
      ti->second = 0;
      timeout_timers.erase(ti); // delete the entry
   }

     // Now check if default_connect && timeout are set
     // and start a reconnect timer in this case
   if ((link_cfg[name].default_connect) && (link_cfg[name].timeout > 0))
   {
       // timeout_timers contains all running expiration timers for each
       // logics
     timeout_timers.insert(make_pair(name, new Timer(link_cfg[name].timeout)));
     timeout_timers[name]->expired.connect(
         mem_fun(*this, &LinkManager::upTimeout));
   }

   return check;
} /* LinkManager::disconnectLinks */


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
  LinkCfg::iterator it;

  for (LinkCfg::iterator it=link_cfg.begin(); it!=link_cfg.end(); ++it)
  {
    if (it->second.link_cname.find(logicname) != it->second.link_cname.end())
    {
      t_linknames.push_back(it->first);
    }
  }
  return t_linknames;
} /* LinkManager::getLinkNames */


void LinkManager::upTimeout(Async::Timer *t)
{
  TimerMap::iterator it_ts;

  for (TimerMap::iterator it_ts = timeout_timers.begin();
       it_ts != timeout_timers.end();
       ++it_ts)
  {
    if (it_ts->second == t)
    {
      LinkCfg::iterator it = link_cfg.find(it_ts->first);

      if (it->second.is_connected && !it->second.default_connect
          && !it->second.no_disconnect)
      {
        disconnectLinks(it_ts->first);
      }

      if (!it->second.is_connected && it->second.default_connect)
      {
        connectLinks(it_ts->first);
      }
    }
  }
} /* LinkManager::upTimeout */



/****************************************************************************
 *
 * Private local functions
 *
 ****************************************************************************/



/*
 * This file has not been truncated
 */
