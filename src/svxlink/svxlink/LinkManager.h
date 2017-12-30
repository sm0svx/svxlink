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
class LogicBase;


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

/**
 * @brief A class that handle logic linking
 *
 * This class handle connections between two or more logic cores. A group of
 * connections between two or more logic cores is called a \em link. A link
 * can be controlled (activated/deactivated) by DTMF commands. It can also be
 * setup to be connected by default.
 *
 * A link configuration section may look like this:
 *
 *   [LinkToR4]
 *   CONNECT_LOGICS=RepeaterLogic:94:SK3AB,SimplexLogic
 *   DEFAULT_CONNECT=1
 *   TIMEOUT=300
 *
 * The setup above will define a link called "LinkToR4". When activated, it
 * will connect RepeaterLogic and SimplexLogic. The link will be activated by
 * default. To disconnect the link, a user may send DTMF command 940 from the
 * RepeaterLogic side. When there have been no activity for 300 seconds
 * (5 minutes), the link will be automatically connected again. It can also be
 * manually connected again using DTMF command 941 from the RepeaterLogic side.
 * It is not possible to control the link from the SimplexLogic side since no
 * command has been specified.
 */
class LinkManager : public sigc::trackable
{
  private:
    struct Link;

  public:
    typedef Link& LinkRef;

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
    void addLogic(LogicBase *logic);

    /**
     * @brief Delete a logic core from the link manager
     * @param logic The logic to delete
     */
    void deleteLogic(LogicBase *logic);

    /**
     * @brief Should be called after all logics have been initialized
     *
     * Check if DEFAULT_CONNECT is set for a link and if so, the logics
     * configured for that link is connected.
     * This function will also go through all links and check if all specified
     * logics are present. If not, a warning will be printed and the missing
     * logic will be removed from the link.
     */
    void allLogicsStarted(void);

    /**
     * @brief   Called by the DTMF command handler upon command reception
     * @param   link The link object associated with this command
     * @param   logic The logic core associated with this command
     * @param   subcmd The subcommand
     * @return  Returns an event handler command to be executed by the logic
     */
    std::string cmdReceived(LinkRef link, LogicBase *logic,
                            const std::string &subcmd);

  private:
    struct LogicProperties
    {
      std::string cmd;
      std::string announcement_name;
    };
    typedef std::map<std::string, LogicProperties> LogicPropMap;
    typedef std::set<std::string> StrSet;
    struct Link
    {
      Link(void)
        : timeout(0), default_active(false), is_activated(false),
          timeout_timer(0)
      {}
      ~Link(void) { delete timeout_timer; }

      std::string  name;
      LogicPropMap logic_props;
      StrSet       auto_activate;
      unsigned     timeout;
      bool         default_active;
      bool         is_activated;
      Async::Timer *timeout_timer;
    };
    typedef std::map<std::string, Link> LinkMap;
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
    struct LogicInfo
    {
      LogicBase         *logic;
      sigc::connection  idle_state_changed_con;
    };
    typedef std::map<std::string, LogicInfo> LogicMap;

    static LinkManager *_instance;

    LinkMap     links;
    LogicMap    logic_map;
    LogicConSet current_cons;
    SourceMap   sources;
    SinkMap     sinks;
    bool        all_logics_started;

    LinkManager(void) : all_logics_started(false) {};
    LinkManager(const LinkManager&);
    ~LinkManager(void);

    std::vector<std::string> getLinkNames(const std::string& logicname);
    void wantedConnections(LogicConSet &want);
    void activateLink(Link &link);
    void deactivateLink(Link &link);
    /*
    bool isConnected(const std::string& source_name,
                     const std::string& sink_name);
    */
    void linkTimeout(Async::Timer *t, Link *link);
    void logicIdleStateChanged(bool is_idle, const LogicBase *logic);
    void checkTimeoutTimer(Link &link);

};  /* class LinkManager */


#endif /* LINK_MANAGER_INCLUDED */

/*
 * This file has not been truncated
 */
