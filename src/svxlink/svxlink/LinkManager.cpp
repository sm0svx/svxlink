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

#include "version/SVXLINK.h"
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

void print_error(const string &name, const string &variable,
                 const string &value, const string &example = "");


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



/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/

LinkManager* LinkManager::_instance = 0;

bool LinkManager::initialize(const Async::Config &cfg,
                             const std::string &cfg_name)
{
    // check if already initialized
  if (LinkManager::_instance->has_instance()) return false;
  LinkManager::_instance = new LinkManager();

  bool init_ok = true;

    // split the line into an array of section names
    // LINKS=Link1,Link2,Link3,...
    // cfg_name == [GLOBAL]/LINKS
  StrList linklist;
  SvxLink::splitStr(linklist, cfg_name, ",");

  vector<string>::const_iterator name;
  string tname;  // temp linkname
  string tvalue;
  linkSet tlCmd;
  vector<string> tlogiclist;
  vector<string> tnamelist;
  size_t found;
  size_t found2;
  vector<string> tauto_connect;
  t_cname tlc;         // -> ("Link1", <"73","Name2">)
  link_properties tln; // -> ("Link1", "NameOfConnection")
  string tlogic;

    // loop to create a link object for each single linkdefinition
    // in the "LINKS=Link1, Link2, Link3" set
  for (name = linklist.begin(); name != linklist.end(); name++)
  {
      // each link set must have a name
    if (!cfg.getValue((*name), "NAME", tname))
    {
      cerr << "*** ERROR: each Linkdefinition must have an unique name.\n"
           << "           e.g. " << (*name) << "/NAME=Test" << endl;
      init_ok = false;
    }
    tlCmd.name = tname;

      // check the connect_logics
    if (!cfg.getValue((*name), "CONNECT_LOGICS", tvalue))
    {
      cerr << "*** ERROR: " << (*name) << "/CONNECT_LOGICS=? You must "
           << "have at least two logics to connect." << endl;
      init_ok = false;
    }

      // Logic1:70:name1  Logic2:71:name2 ...
    SvxLink::splitStr(tlogiclist, tvalue, ",");

      // configure the list of logics to connect
      // CONNECT_LOGICS=Logic1:70:Name1,Logic2:71,Logic3:72:Name2
    for(vector<string>::iterator it = tlogiclist.begin();
                                 it != tlogiclist.end(); it++)
    {
      found = it->find(":", 0);    // (Logic -> <cmd, Name>)
      found2 = it->find(":", found + 1);
      if (found2 != string::npos)
      {
        tlogic = (*it).substr(0, found);
        tln.logic_cmd =  (*it).substr(found + 1, found2-found-1);
        tln.logic_name = (*it).substr(found2 + 1, (*it).length()-found2);
        tlc.insert(pair<string, link_properties>(tlogic, tln));
      }
      else
      {
        cout << "*** ERROR: wrong configuration in section CONNECT_LOGICS="
             << tvalue << endl
             << "           e.g. CONNECT_LOGICS=Logicname:Command:Linkname\n";
        init_ok = false;
      }
    }

    tlCmd.link_cname = tlc;
    tlCmd.timeout = 0;

      // check timeout settings
    if (cfg.getValue((*name), "TIMEOUT", tvalue))
    {
      tlCmd.timeout = atoi(tvalue.c_str()) * 1000;
    }

    tlCmd.auto_connect.clear();

      // connects automatically the links, if the one (or more) links
      // has/have activity for timeout
    if (cfg.getValue((*name), "AUTOCONNECT_ON_SQL", tvalue))
    {
      SvxLink::splitStr(tauto_connect, tvalue, ",");
      tlCmd.auto_connect = tauto_connect;

        // Autoconnect should be disconnected after a while
      if (tlCmd.timeout == 0)
      {
        cout << "*** WARNING: missing param " << (*name)
             << "/TIMEOUT=??, set to default (30 sec)" << endl;
        tlCmd.timeout = 30000;
      }
    }

    tlCmd.is_connected = false;
    tlCmd.default_connect = false;
    tlCmd.no_disconnect = false;


      // checking OPTIONS ( on_startup?, ... )
    cfg.getValue( *name, "DEFAULT_CONNECT", tlCmd.default_connect);

      // persistent connected?
    cfg.getValue( *name, "NO_DISCONNECT", tlCmd.no_disconnect);

    LinkManager::instance()->linkCfg.insert(pair<string,linkSet>(tname,tlCmd));
    tlc.clear();
  }

  if( !init_ok )
  {
    delete LinkManager::_instance;
    LinkManager::_instance = NULL;
  }

  return init_ok;

} /* LinkManager::initialize */


// register the audio source (logicname , audioname)
void LinkManager::addSource(const string& logicname,
                               AudioSelector *logic_con_out)
{
    // adding the audio source
  if (sourceIsAdded(logicname))
  {
    return;
  }

  AudioSplitter *splitter = new AudioSplitter;
  logic_con_out->registerSink(splitter);

  sources[logicname].source = logic_con_out;
  sources[logicname].splitter = splitter;

  SinkMap::iterator it;

  for (it=sinks.begin(); it != sinks.end(); it++)
  {
    AudioPassthrough *connector = new AudioPassthrough;
    splitter->addSink(connector);
    AudioSelector *sel = (*it).second.selector;
    sel->addSource(connector);
    (*it).second.connectors[logicname] = connector;
  }
} /* LinkManager::addSource */


void LinkManager::addSink(const string& logicname, AudioSplitter *logic_con_in)
{

  if (sinkIsAdded(logicname))
  {
    return;
  }

  AudioSelector *selector = new AudioSelector;
  selector->registerSink(logic_con_in);

  sinks[logicname].sink = logic_con_in;
  sinks[logicname].selector = selector;

  SourceMap::iterator it;
  for (it=sources.begin(); it!=sources.end(); it++)
  {
    AudioPassthrough *connector = new AudioPassthrough;
    (*it).second.splitter->addSink(connector);
    selector->addSource(connector);
    sinks[logicname].connectors[(*it).first] = connector;
  }
} /* LinkManager::addSink */


void LinkManager::deleteSource(const string& logicname)
{
   assert(LinkManager::instance()->sources.size() > 0);

   SinkMap::iterator it;
   AudioSplitter *splitter=LinkManager::instance()->sources[logicname].splitter;

   for (it=sinks.begin(); it!=sinks.end(); ++it)
   {
     assert((*it).second.connectors.size() > 0);
     AudioPassthrough *connector = (*it).second.connectors[logicname];
     (*it).second.selector->removeSource(connector);
     (*it).second.connectors.erase(logicname);
     splitter->removeSink(connector);
     delete connector;
   }

   delete splitter;
   sources.erase(logicname);
} /* LinkManager::deleteSource */


void LinkManager::deleteSink(const string& logicname)
{
  assert(LinkManager::instance()->sinks.size() > 0);
  AudioSelector *selector = sinks[logicname].selector;

  map<string, AudioPassthrough *>::iterator it;
  map<string, AudioPassthrough *> &cons = sinks[logicname].connectors;
  for (it=cons.begin(); it!=cons.end(); ++it)
  {
    AudioPassthrough *connector = (*it).second;
    selector->removeSource(connector);
    assert(sources.size() >= 1);
    sources[logicname].splitter->removeSink(connector);
    delete connector;
  }

  delete selector;
  sinks.erase(logicname);
} /* LinkManager::deleteSink */


 // trigger from the logic, now check if DEFAULT_CONNECT is set
 // and the all logics inside the link definition can be connected
void LinkManager::logicIsUp(std::string logicname)
{
   map<std::string, linkSet>::iterator it;
   map<std::string, link_properties>::iterator lit;
   bool conn = false;

     // collecting names of logics that already are up
   logiclist.insert(logicname);

     // 1st we need all LINK's in that this Logic is involved
   vector<string> ln = LinkManager::instance()->getLinkNames(logicname);

   for (vector<string>::iterator vt = ln.begin(); vt != ln.end(); vt++)
   {
       // 2nd we have to proof, if the Logics should beeing connected on startup
       // and if the involved logics are already up
     if ((it = LinkManager::instance()->linkCfg.find(*vt)) !=
               LinkManager::instance()->linkCfg.end())
     {
       conn = true;
       for (lit = (it->second).link_cname.begin();
                           lit != (it->second).link_cname.end(); lit++)
       {
         if (find(logiclist.begin(), logiclist.end(), lit->first) ==
                                              logiclist.end())
         {
             // one affected logic isn't already up so we skip connecting,
             // it will be called up with the next logic again
           conn = false;
         }
       }
     }

     if (conn && it != linkCfg.end() && (it->second).default_connect)
     {
         // if DEFAULT_CONNECT is true, connect the logics together
       connectLinks((it->second).name);
     }
   }
} /* LinkManager::logicIsUp */


// returns all commands specific for the logic that called up this function
vector<string> LinkManager::getCommands(string logicname)
{
   vector<string> tcmds;
   map<std::string, linkSet>::iterator it;
   map<string, link_properties>::iterator lc;

   for (it = linkCfg.begin(); it != linkCfg.end(); it++)
   {
     lc = (it->second).link_cname.find(logicname);

     if (lc != (it->second).link_cname.end() &&
                           atoi((lc->second).logic_cmd.c_str()) > 0 )
     {
       tcmds.push_back((lc->second).logic_cmd);
     }
   }
   return tcmds;
} /* LinkManager::getCommands */


// reset the timeout timers for the links, that are connected
void LinkManager::resetTimers(const string& logicname)
{
     // here we have only the "logicname" but no idea in how many LINKS
     // this logic is involved so we have to check all LINKS entries
     // to reset each single timeout timer in each "linkname"
   vector<string>::iterator vt;
   map<string, Async::Timer*>::iterator it;
   vector<string>::iterator tauto_connect;

     // we need all "linknames" where the "logicname" is included in
   vector<string> t_linknames=LinkManager::instance()->getLinkNames(logicname);

   for (vt = t_linknames.begin(); vt != t_linknames.end(); ++vt)
   {
     it = timeoutTimerset.find(*vt);
     if ( it != timeoutTimerset.end() )
     {
         // reset each Timer connected with the specified logic
       (it->second)->reset();
       (it->second)->setEnable(false);
     }

       // check if a logic has sent a trigger that is defined as a
       // "auto_connect" logic, in this case all logics of a link shall
       // beeing connected if they are disconnected before
     for (tauto_connect=((linkCfg[*vt]).auto_connect).begin();
           tauto_connect!=((linkCfg[*vt]).auto_connect).end(); tauto_connect++)
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


// restart the to-timer
void LinkManager::enableTimers(const string& logicname)
{
   std::vector<string>::iterator vt;
   std::map<std::string, Async::Timer*>::iterator it;

     // we need all "linknames" where the "logicname" is included in
   vector<string> t_linknames =
     LinkManager::instance()->getLinkNames(logicname);

   for (vt = t_linknames.begin(); vt != t_linknames.end(); ++vt)
   {
      it = timeoutTimerset.find(*vt);
      if ( it != timeoutTimerset.end() )
      {
          // enable each Timer connected with the specified logic
        (it->second)->setEnable(true);
      }
   }
} /* LinkManager::enableTimers */


// called from the Logic if a command has been received
string LinkManager::cmdReceived(const string& logicname, string cmd, string subcmd)
{
  string ss;
  map<std::string, linkSet>::iterator it;
  map<string, link_properties>::iterator lcx;
  int tconnect;
  int isubcmd = atoi(subcmd.c_str());

  cout << "cmdReceived " << logicname << ": " << cmd << " subcmd:"
       << subcmd << endl << flush;

  for (it = linkCfg.begin(); it != linkCfg.end(); it++)
  {
    lcx = ((it->second).link_cname).find(logicname);
    if (lcx != ((it->second).link_cname).end())
    {
      if ((lcx->second).logic_cmd == cmd)
      {
        switch(isubcmd)
        {
            // disconnecting Link1 <-X-> Link2
          case ERROR:
            if ((it->second).no_disconnect)
            {
                // not possible due to configuration
              ss = "deactivating_link_not_possible ";
              ss += (lcx->second).logic_name;
              break;
            }
            if (!(it->second).is_connected)
            {
              ss = "link_not_active ";
              break;
            }

            tconnect = disconnectLinks((it->second).name);
            switch (tconnect)
            {
              case ERROR:
                ss = "deactivating_link_failed ";
                break;
              case OK:
                ss = "deactivating_link ";
                break;
              case ALREADY_CONNECTED:
                ss = "deactivating_link_failed_other_connection ";
            }
            ss += (lcx->second).logic_name;
            break;

            // connecting Logic1 <---> Logic2 (two ways)
          case OK:
            if ((it->second).is_connected)
            {
              ss = "link_already_active ";
              ss += (lcx->second).logic_name;
              break;
            }

            tconnect = connectLinks((it->second).name);
            switch (tconnect)
            {
              case ERROR:
                ss = "activating_link_failed ";
                break;
              case OK:
                ss = "activating_link ";
                break;
              case ALREADY_CONNECTED:
                ss = "activating_link_failed_other_connection ";
                break;
            }
            ss += (lcx->second).logic_name;
            break;

          default:
            ss = "unknown_command " + cmd;
            ss += isubcmd;
        }
      }
    }
  }
  return ss;
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


// connects the logics by checking the connect state before
int LinkManager::connectLinks(const string& name)
{
  int check = ERROR;

    // get the wanted connect matrix depending from the linkname, e.g.
    // <RepeaterLogic, MicSpkeLogic>
    // <RepeaterLogic, SimplexLogic>
    // <SimplexLogic, RepeaterLogic>
    // <SimplexLogic, MicSpkrLogic>
    // <MicSpkrLogic, RepeaterLogic>
    // <MicSpkrLogic, SimplexLogic>
  LogicConSet want = LinkManager::instance()->getMatrix(name);

    // gets the difference of both, the result is the matrix to be connected
    // "is" contains the actual state of the connected logics, e.g.
    // <RepeaterLogic, SimplexLogic>
    // <SimplexLogic, RepeaterLogic>
  LogicConSet diff = LinkManager::instance()->getDifference(is, want);

    // if the logics are already connected by other linkdefinitions, we
    // have to tell the others about this situation
    // check = 0 -> problems when connecting the links
    // check = 1 -> the connect is established
    // check = 2 -> the request could not made because the logics are already
    //              connected by other linkdefinitions
  if (diff.size() == 0)
  {
    return ALREADY_CONNECTED;
  }

    // diff contains the resulting logics that must beeing connected e.g.
    // <RepeaterLogic, MicSpkeLogic>
    // <SimplexLogic, MicSpkrLogic>
    // <MicSpkrLogic, RepeaterLogic>
    // <MicSpkrLogic, SimplexLogic>
  LogicConSet::iterator it;
  for (it = diff.begin(); it != diff.end(); it++)
  {
    cout << "connecting " << it->first << " ===> "
      << it->second << endl;
    sinks[it->first].selector->
      enableAutoSelect(sinks[it->first].connectors[it->second], 0);

      // store all connections in "is"
    is.insert(pair<string, string>(it->first, it->second));
    check = OK;
  }

    // setting up the timers if configured
  if ((linkCfg[name]).timeout > 0 &&
      (linkCfg[name]).no_disconnect == false &&
      (linkCfg[name]).default_connect == false &&
      !(timeoutTimerset.find(name) != timeoutTimerset.end()) )
  {
      // timeouttimerset contains all running expiration timers for each
      // logic
    timeoutTimerset.insert(pair<string, Async::Timer*>
        (name, new Timer((linkCfg[name]).timeout)));
    (timeoutTimerset[name])->
      expired.connect(mem_fun(*this, &LinkManager::upTimeout));
  }
  linkCfg[name].is_connected = true;

  return check;
} /* LinkManager::connectLinks */


set<pair<string, string> > LinkManager::getDifference(LogicConSet is,
                                                      LogicConSet want)
{
  set<pair<string, string> > ret = want;
  set<pair<string, string> >::iterator it;

  for (it = want.begin(); it != want.end(); it++)
  {
    if (is.find(*it) != is.end())
    {
      ret.erase(*it);
    }
  }

  return ret;
} /* Linkmanager::getDifference */


// returns a set of logics to be connected
set<pair<string, string> > LinkManager::getLogics(const string& linkname)
{
  set<pair<string, string> > ret;
  linkC::iterator it = linkCfg.find(linkname);
  map<string, link_properties>::iterator xi, xj;

  for (xi = (it->second).link_cname.begin();
       xi != (it->second).link_cname.end();
       xi++)
  {
    for (xj = (it->second).link_cname.begin(); xj !=
                                 (it->second).link_cname.end(); xj++)
    {
      if (xj->first != xi->first)
      {
        if (logiclist.find(xj->first) != logiclist.end() &&
                   logiclist.find(xi->first) != logiclist.end())
        {
          ret.insert(pair<string, string>(xi->first, xj->first));
        }
        else
        {
          cout << "*** WARNING: One of this logics: "<< xj->first << ", "
               << xi->first << " isn't up an can not beeing connected.\n"
               << "**** Missing entry in your configuration " <<
               "[GLOBAL]/LINKS=???" << endl;
        }
      }
    }
  }
  return ret;
} /* LinkManager::getLogics */


set<pair<string, string> > LinkManager::getMatrix(const string& linkname)
{
  linkC::iterator it = linkCfg.find(linkname);
  set<pair<string, string> > ret = LinkManager::instance()->getLogics(linkname);
  map<string, link_properties>::iterator xi, xj;
  linkC::iterator iu;
  t_cname::iterator cn, dn;

  for (iu = linkCfg.begin(); iu != linkCfg.end(); iu++)
  {
    if ((iu->second).is_connected)
    {
      for (xj = (it->second).link_cname.begin();
                     xj != (it->second).link_cname.end(); xj++)
      {
          // look in the actual connected linkset if there is a logicname
          // that exists in another already connected link definition
        cn = (iu->second).link_cname.find(xj->first);
        if (cn != (iu->second).link_cname.end())
        {
          for (dn=(iu->second).link_cname.begin();
                        dn!=(iu->second).link_cname.end(); dn++)
          {
            if (dn->first != cn->first)
            {
              for (xi = (it->second).link_cname.begin();
                   xi != (it->second).link_cname.end(); xi++)
              {
                if (dn->first != xi->first)
                {
                  ret.insert(pair<string, string>(dn->first, xi->first));
                  ret.insert(pair<string, string>(xi->first, dn->first));
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


  // returns the logics that must be disconnected
set<pair<string, string> > LinkManager::getToDisconnect(const string& name)
{
  map<string, linkSet>::iterator it;
  set<pair<string, string> > ret = is;  // get all actual connections
  set<pair<string, string> > t_ret;
  set<pair<string, string> >::iterator s_it;

    // go through all linkdefinitions and check if these are connected
  for (it = linkCfg.begin(); it != linkCfg.end(); it++)
  {
    if ((it->second).is_connected && name != (it->second).name)
    {
      t_ret = LinkManager::instance()->getLogics((it->second).name);
      for (s_it = t_ret.begin(); s_it != t_ret.end(); s_it++)
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


int LinkManager::disconnectLinks(const string& name)
{
   int check = ERROR;

   if (linkCfg.find(name) == linkCfg.end())
   {
     return check;
   }

   assert(LinkManager::instance()->sources.size() > 0);
   assert(LinkManager::instance()->sinks.size() > 0);

   LogicConSet diff = getToDisconnect(name);

   if (diff.size() == 0)
   {
     check = ALREADY_CONNECTED;
   }

   LogicConSet::iterator it;

   for (it = diff.begin(); it != diff.end(); it++)
   {
     if (logiclist.find(it->first) != logiclist.end() &&
           logiclist.find(it->second) != logiclist.end())
     {
       cout << "disconnect " << it->first << " -X-> " << it->second << endl;
       sinks[it->first].selector->
              disableAutoSelect(sinks[it->first].connectors[it->second]);

         // delete the link-connect information
       is.erase(pair<string, string>(it->first, it->second));
       check = OK;
     }
     else
     {
       cout << "*** WARNING: Missing logics entry in [GLOBAL]/LINKS=???, "
            << "             check your configuration." << endl;
     }
   }

     // reset the connected flag
   (linkCfg[name]).is_connected = false;

     // clear the timer
   map<string, Async::Timer*>::iterator ti;
   if ((ti = timeoutTimerset.find(name)) != timeoutTimerset.end())
   {
      ti->second = 0;
      delete ti->second;  // delete the timer
      timeoutTimerset.erase(ti); // delete the entry
   }

     // now check if default_connect && timeout are set
     // and start a reconnect timer in this case
   if ((linkCfg[name]).default_connect && (linkCfg[name]).timeout > 0 )
   {
       // timeoutTimerset contains all running expiration timers for each
       // logics
     timeoutTimerset.insert(pair<string, Async::Timer*>
                           (name, new Timer((linkCfg[name]).timeout)));
     (timeoutTimerset[name])->
                     expired.connect(mem_fun(*this, &LinkManager::upTimeout));
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


// returns all linknames in that the specific logic is involved
vector<string> LinkManager::getLinkNames(const string& logicname)
{
  vector<string> t_linknames;
  map<string, linkSet>::iterator it;

  for (it=linkCfg.begin(); it!=linkCfg.end(); it++)
  {
    if ((it->second).link_cname.find(logicname)
        != (it->second).link_cname.end())
    {
      t_linknames.push_back(it->first);
    }
  }
  return t_linknames;
} /* LinkManager::getLinkNames */


void LinkManager::upTimeout(Async::Timer *t)
{
  map<string, Async::Timer*>::iterator it_ts;
  map<string, linkSet>::iterator it;

  for (it_ts=timeoutTimerset.begin(); it_ts!=timeoutTimerset.end(); it_ts++)
  {
    if ((it_ts->second) == t)
    {
      it = linkCfg.find(it_ts->first);

      if ((it->second).is_connected && !(it->second).default_connect
          && !(it->second).no_disconnect)
      {
        disconnectLinks(it_ts->first);
      }

      if (!(it->second).is_connected && (it->second).default_connect)
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

  // Put all locally defined functions in an anonymous namespace to make them
  // file local. The "static" keyword has been deprecated in C++ so it
  // should not be used.
namespace
{

void print_error(const string &name, const string &variable,
                 const string &value, const string &example)
{
  cerr << "*** ERROR: Config variable [" << name << "]/" << variable << "="
       << value << " wrong or not set.";

  if (!example.empty())
  {
    cerr << "\n*** Example: " <<  example;
  }
  cerr << endl;
} /* print_error */

} // End of anonymous namespace



/*
 * This file has not been truncated
 */
