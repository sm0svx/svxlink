/**
@file	 LinkManager.h
@brief   Contains the manager to link different logics together
         implemented as singleton
@author  Adi Bier (DL1HRC) / Christian Stussak (University of Halle)
@date	 2011-08-24

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2009 Tobias Blomberg / SM0SVX

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


#ifndef LINK_MANAGER_INCLUDED
#define LINK_MANAGER_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <string>
#include <vector>
#include <list>
#include <set>
#include <stdint.h>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncConfig.h>
#include <AsyncTimer.h>
#include <AsyncAudioSelector.h>
#include <AsyncAudioSplitter.h>
#include <AsyncAudioPassthrough.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "CmdParser.h"


/****************************************************************************
 *
 * Forward declarations
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Namespace
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Forward declarations of classes inside of the declared namespace
 *
 ****************************************************************************/

class Command;



/****************************************************************************
 *
 * Defines & typedefs
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Exported Global Types
 *
 ****************************************************************************/


/****************************************************************************
 *
 * Class definitions
 *
 ****************************************************************************/

class LinkManager : public sigc::trackable
{
  public:
    static LinkManager* instance(void)
    {
      static CGuard g;
      return _instance;
    }

    static bool has_instance(void)
    {
      return (_instance != 0);
    }

    static bool initialize(const Async::Config &cfg,
                           const std::string &cfg_name);


    void addSource(const std::string &source_name,
                   Async::AudioSource *logic_con_out);
    void addSink(const std::string &sink_name,
                 Async::AudioSink *logic_con_in);
    void deleteSource(const std::string &source_name);
    void deleteSink(const std::string &sink_name);
    void logicIsUp(const std::string &name);
    void resetTimers(const std::string &logicname);
    void enableTimers(const std::string &logicname);
    std::string cmdReceived(const std::string &logicname,
                            const std::string &cmd,
                            const std::string &subcmd);
    std::vector<std::string> getCommands(std::string logicname) const;

    sigc::signal<void, uint8_t, bool> logicStateChanged;

  private:
    class CGuard
    {
      public:
        ~CGuard(void)
        {
          if (LinkManager::_instance != 0)
          {
            delete LinkManager::_instance;
            LinkManager::_instance = 0;
          }
        }
    };
    struct LinkProperties
    {
      std::string logic_name;
      std::string logic_cmd;
    };
    typedef std::map<std::string, LinkProperties> CName;
    typedef std::vector<std::string> StrList;
    struct LinkSet
    {
      LinkSet(void)
        : timeout(0), default_connect(false), no_disconnect(false),
          is_connected(false)
      {}

      std::string  name;
      CName        link_cname;
      StrList      auto_connect;
      unsigned     timeout;
      bool         default_connect;
      bool         no_disconnect;
      bool         is_connected;
    };
    typedef std::map<std::string, LinkSet> LinkCfg;
    typedef std::set<std::pair<std::string, std::string> > LogicConSet;
    struct SourceInfo
    {
      Async::AudioSource      *source;
      Async::AudioSplitter    *splitter;
    };
    typedef std::map<std::string, Async::AudioPassthrough *> ConMap;
    struct SinkInfo
    {
      Async::AudioSink      *sink;
      Async::AudioSelector  *selector;
      ConMap                connectors;
    };
    typedef std::map<std::string, SourceInfo> SourceMap;
    typedef std::map<std::string, SinkInfo>   SinkMap;
    typedef enum
    {
      ERROR = 0, OK = 1, ALREADY_CONNECTED = 3
    } ConnectResult;
    typedef std::map<std::string, Async::Timer*> TimerMap;

    static LinkManager *_instance;

    LinkCfg               link_cfg;
    std::set<std::string> logic_list;
    TimerMap              timeout_timers;
    LogicConSet           current_cons;
    SourceMap             sources;
    SinkMap               sinks;

    LinkManager(void) {};
    LinkManager(const LinkManager&);
    ~LinkManager(void) {};

    bool sourceIsAdded(const std::string &logicname);
    bool sinkIsAdded(const std::string &logicname);
    std::vector<std::string> getLinkNames(const std::string& logicname);
    ConnectResult connectLinks(const std::string& name);
    ConnectResult disconnectLinks(const std::string& name);
    bool isConnected(const std::string& source_name,
                     const std::string& sink_name);
    void upTimeout(Async::Timer *t);
    LogicConSet getDifference(LogicConSet is, LogicConSet want);
    LogicConSet getLogics(const std::string& linkname);
    LogicConSet getMatrix(const std::string& name);
    LogicConSet getToDisconnect(const std::string& name);
};  /* class LinkManager */


#endif /* LINK_MANAGER_INCLUDED */

/*
 * This file has not been truncated
 */
