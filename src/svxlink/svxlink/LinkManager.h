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
    static LinkManager* instance()
    {
       static CGuard g;
       if (!_instance)
        return NULL; // illegal pointer, if not initialized
       return _instance;
    }

    static bool has_instance()
    {
       return _instance;
    }


    static bool initialize(const Async::Config &cfg,
                                             const std::string &cfg_name);
    typedef std::vector<std::string> StrList;
    typedef std::map<std::string, std::vector<std::string> > CommandLinkList;
    CommandLinkList    cmd_link_list;

    std::set<std::string> logiclist;

    struct linkSet
    {
      std::string  linkname;
      std::map<std::string, std::string>  logic_cmd; // logicname : command
      std::vector<std::string> auto_connect;
      int          timeout;
      bool         default_connect;
      bool         no_disconnect;
      bool         is_connected;
    };

    typedef std::map<std::string, linkSet> linkC;
    linkC    linkCfg;

    typedef std::set<std::pair<std::string, std::string> > LogicConSet;
    typedef std::map<std::string, std::set<std::string> > LogicConMap;
    LogicConMap con_map;
   // LogicConSet is;

    void addSource(const std::string& logicname, Async::AudioSelector *logic_con_out);
    void addSink(const std::string& logicname, Async::AudioSplitter *logic_con_in);
    void deleteSource(const std::string& source_name);
    void deleteSink(const std::string& sink_name);
    void logicIsUp(std::string name);
    void resetTimers(const std::string& logicname);
    void enableTimers(const std::string& logicname);
    std::string cmdReceived(const std::string& logicname, std::string cmd, std::string subcmd);
    std::vector<std::string> getCommands(std::string logicname);


    class CGuard
    {
      public:
        ~CGuard()
        {
          if(NULL != LinkManager::_instance)
          {
             delete LinkManager::_instance;
             LinkManager::_instance = NULL;
          }
        }
    };
    friend class CGuard;


  private:
    static LinkManager* _instance;
    LinkManager() {};
    LinkManager(const LinkManager&);
    ~LinkManager() {};

    std::map<std::string, Async::Timer*> timeoutTimerset;

    std::vector<std::string>getLinks(std::string logicname);
    Async::Timer    *up_timer;

    typedef struct
    {
      Async::AudioSource      *source;
      Async::AudioSplitter    *splitter;
    } SourceInfo;
    typedef struct
    {
      Async::AudioSink       	      	      	      	*sink;
      Async::AudioSelector   	      	      	      	*selector;
      std::map<std::string, Async::AudioPassthrough *>	connectors;
    } SinkInfo;

    typedef std::map<std::string, SourceInfo> SourceMap;
    typedef std::map<std::string, SinkInfo>   SinkMap;

    SourceMap sources;
    SinkMap   sinks;

    bool sourceIsAdded(const std::string &logicname);
    bool sinkIsAdded(const std::string &logicname);
    std::vector<std::string> getLinkNames(const std::string& logicname);
    bool connectLinks(const std::string& linkname);
    bool disconnectLinks(const std::string& linkname);
    bool isConnected(const std::string& source_name, const std::string& sink_name);
    void upTimeout(Async::Timer *t);
    std::set<std::pair<std::string, std::string> > getmatrix(const std::string& linkname);
    std::set<std::pair<std::string, std::string> > getdifference(LogicConSet is, LogicConSet want);
    std::set<std::pair<std::string, std::string> > gettodisconnect(const std::string& linkname);
    std::set<std::pair<std::string, std::string> > getisconnected(void);
};  /* class LinkManager */

#endif /* LINK_MANAGER_INCLUDED */

/*
 * This file has not been truncated
 */
