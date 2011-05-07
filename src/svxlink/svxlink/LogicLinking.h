/**
@file	 LogicLinking.h
@brief   Contains a linking logic implementation as singleton
@author  Adi Bier (DL1HRC) / Christian Stussak (University of Halle)
@date	 2011-04-24

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


#ifndef LOGIC_LINKING_INCLUDED
#define LOGIC_LINKING_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <string>
#include <vector>
#include <list>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncConfig.h>
#include <AsyncTimer.h>
#include <AsyncAudioSelector.h>
#include <AsyncAudioSplitter.h>
//#include <common.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "AudioSwitchMatrix.h"


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

class LogicLinking : public SigC::Object
{
  public:
    static LogicLinking* instance()
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

    std::vector<std::string> getLinkName(std::string linkname);
    std::vector<std::string> getCommands(std::string name);
    bool connectLogics(std::string logicname);
    bool disconnectLogics(std::string logicname);
    void resetTimers(std::string logicname);
    bool connectThisLink(std::string logicname, std::string cmd,
                                 std::string &link_name);
    bool disconnectThisLink(std::string logicname, std::string cmd,
                                 std::string &link_name);
    void addSource(const std::string name, Async::AudioSelector *logic_con_out)
    {
       audio_switch_matrix.addSource(name, logic_con_out);
    }
    void addSink(const std::string name, Async::AudioSplitter *logic_con_in)
    {
       audio_switch_matrix.addSink(name, logic_con_in);
    }
    void disconnectSource(const std::string& source_name)
    {
        audio_switch_matrix.disconnectSource(source_name);
    }
    void disconnectSink(const std::string& source_name)
    {
        audio_switch_matrix.disconnectSink(source_name);
    }
    bool sourceIsAdded(const std::string& source_name)
    {
        return audio_switch_matrix.sourceIsAdded(source_name);
    }
    void removeSource(const std::string& source_name)
    {
        audio_switch_matrix.removeSource(source_name);
    }
    bool sinkIsAdded(const std::string& sink_name)
    {
        return audio_switch_matrix.sinkIsAdded(sink_name);
    }
    void removeSink(const std::string& sink_name)
    {
        audio_switch_matrix.removeSink(sink_name);
    }

    bool checkConnectOnStartup(std::string name);

    struct Cfg
    {
      Cfg() : timeout(0) {};
      int          timeout;
      StrList      conn_logics;
      std::string  name;
      std::string  command;
    };

    class CGuard
    {
      public:
        ~CGuard()
        {
          if(NULL != LogicLinking::_instance)
          {
             delete LogicLinking::_instance;
             LogicLinking::_instance = NULL;
          }
        }
    };
    friend class CGuard;


  private:
    static LogicLinking* _instance;
    LogicLinking() {};
    LogicLinking(const LogicLinking&);
    ~LogicLinking() {};


    static AudioSwitchMatrix  	audio_switch_matrix;

    struct LogicSet
    {
      std::string   linksection;     // the section in the svxlink.conf
      std::string   name;            // the name of the defined link
      std::vector<std::string> linklogics; // the logics to be dis-/connected
      int           timeout;         // timoeout in seconds
      std::string   command;         // the command,e.g.64 (without subcommand)
      bool          on_startup;      // TRUE=connect during startup of svxlink
      bool          persistent;      // TRUE=no disconnect possible
    };

    //        <link name, logic definitions>
    std::map<std::string, LogicSet> linkset;

    struct SourceSinkSet
    {
       std::string logicname;
       Async::AudioSelector *logic_con_out;
       Async::AudioSplitter *logic_con_in;
    } sourcesinkset;

    StrList logicset;
    std::map<std::string, Async::Timer*> timeoutTimerset;

    bool logicsAreConnected(const std::string& l1, const std::string& l2);
    void linkTimeoutExpired(Async::Timer *t);

};  /* class LogicLinking */

#endif /* LOGIC_LINKING_INCLUDED */

/*
 * This file has not been truncated
 */
