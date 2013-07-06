/**
@file	 LinkManager.h
@brief   Contains the manager to link different logics together
         implemented as singleton
@author  Adi Bier (DL1HRC) / Christian Stussak (University of Halle)
         Tobias Blomberg / SM0SVX
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

namespace Async
{
  class AudioPassthrough;
};
class Logic;


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
    /**
     * @brief Initialize the link manager
     * @param cfg An initialized config object
     * @param link_names A comma separated list of link config section names
     *
     * This function will initialize the link manager. It must be called
     * before calling the instace function.
     */
    static bool initialize(const Async::Config &cfg,
                           const std::string &link_names);

    /**
     * @brief   Check if a link manager instance have been created
     * @return  Returns \em true if an instance has been created
     */
    static bool hasInstance(void)
    {
      return (_instance != 0);
    }

    /**
     * @brief   Retrieve the link manager object
     * @return  Returns the link manager object
     *
     * Before calling this function, the initialize function must be called.
     * Otherwise an assertion will be thrown.
     */
    static LinkManager* instance(void)
    {
      assert(_instance != 0);
      return _instance;
    }

    /**
     * @brief Delete singleton instance
     *
     * Call this function when you want to deallocate all memory that the
     * link manager has allocated, typically before program exit.
     */
    static void deleteInstance(void)
    {
      delete _instance;
      _instance = 0;
    }


    /**
     * @brief Add a logic core to the link manager
     * @param logic The logic core to add
     *
     * This function should be called by each logic core upon creation.
     */
    void addLogic(Logic *logic);

    /**
     * @brief Delete a logic core from the link manager
     * @param logic The logic to delete
     */
    void deleteLogic(Logic *logic);

    /**
     * @brief Should be called after all logics have been initialized
     *
     * Check if DEFAULT_CONNECT is set for a link and if so, the logics
     * configured for that link is connected.
     */
    void allLogicsStarted(void);

    std::string cmdReceived(const std::string &logicname,
                            const std::string &cmd,
                            const std::string &subcmd);
    std::vector<std::string> getCommands(std::string logicname) const;

    sigc::signal<void, int, bool> logicStateChanged;

  private:
    struct LinkProperties
    {
      std::string logic_cmd;
      std::string announcement_name;
    };
    typedef std::map<std::string, LinkProperties> LinkPropMap;
    typedef std::vector<std::string> StrList;
    struct LinkSet
    {
      LinkSet(void)
        : timeout(0), default_connect(false), no_disconnect(false),
          is_connected(false), timeout_timer(0)
      {}
      ~LinkSet(void) { delete timeout_timer; }

      std::string  name;
      LinkPropMap  properties;
      StrList      auto_connect;
      unsigned     timeout;
      bool         default_connect;
      bool         no_disconnect;
      bool         is_connected;
      Async::Timer *timeout_timer;
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
    struct LogicInfo
    {
      Logic             *logic;
      sigc::connection  idle_state_changed_con;
    };
    typedef std::map<std::string, LogicInfo>        LogicMap;

    static LinkManager *_instance;

    LinkCfg               link_cfg;
    LogicMap              logic_map;
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
    void linkTimeout(Async::Timer *t, LinkSet *link);
    LogicConSet getDifference(LogicConSet is, LogicConSet want);
    LogicConSet getLogics(const std::string& linkname);
    LogicConSet getMatrix(const std::string& name);
    LogicConSet getToDisconnect(const std::string& name);
    void logicIdleStateChanged(bool is_idle, const Logic *logic);
    void checkTimeoutTimer(LinkSet &link);

};  /* class LinkManager */


#endif /* LINK_MANAGER_INCLUDED */

/*
 * This file has not been truncated
 */
