/**
@file	 LogicLinking.cpp
@brief   Contains a linking logic implementation as singleton
@author  Adi Bier (DL1HRC) / Christian Stussak (University of Halle/Saale)
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


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "version/SVXLINK.h"
#include "LogicLinking.h"
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

AudioSwitchMatrix LogicLinking::audio_switch_matrix;


/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/

LogicLinking* LogicLinking::_instance = 0;

bool LogicLinking::initialize(const Async::Config &cfg,
                                             const std::string &cfg_name)
{

   // check if already initialized
  if (LogicLinking::_instance->has_instance()) return false;

  bool init_ok = true;
  string value;
  string linkname;
  string temp_links;
  string temp_logics;
  StrList llist,clogics;

  LogicLinking::_instance = new LogicLinking();

   // split the line into an array of section names
  SvxLink::splitStr(llist, cfg_name, ",");
  vector<std::string>::const_iterator cit, tit;

   // cfg_name == [GLOBAL]/LINKS
   // defining the linklogic sets
  for (cit = llist.begin(); cit != llist.end(); cit++)
  {

     temp_links = (*cit);  // e.g. [GLOBAL]/LINKS=Link1 , Link2

     if (!cfg.getValue(temp_links, "LINKNAME", linkname))
     {
        cerr << "*** ERROR: each LogicLinking must have an unique name.\n" <<
              "e.g. " << temp_links << "/LINKNAME=Test\n";
        init_ok = false;
     }

      // LINKNAME=Mic
     LogicLinking::_instance->linkset[linkname].name = linkname;
      // [MicSimplex]
     LogicLinking::_instance->linkset[linkname].linksection = temp_links;

     if (!cfg.getValue(temp_links, "CONNECT_LOGICS", value))
     {
       cerr << "*** ERROR: " << temp_links << "/CONNECT_LOGICS=? You must have"
            << " at least two logics to connect.\n";
       init_ok = false;
     }

      // check if we have at least two logics to connect
      //if (SvxLink::splitStr(LogicLinking::_instance->clogics, value, ",") < 2)
     if (SvxLink::splitStr(clogics, value, ",") < 2)
     {
	    cerr << "*** ERROR: you need at least two LOGICS to connect, e.g. " <<
	            temp_links << "/CONNECT_LOGICS=RepeaterLogic,SimplexLogic\n";
	    init_ok = false;
     }

      // check if logics are defined?
     for (tit = clogics.begin();
             tit != clogics.end(); tit++)
     {
        temp_logics = (*tit);  // Logics from [GLOBAL] item by item
        if (!cfg.getValue("GLOBAL", "LOGICS", value))
        {
            cerr << "*** ERROR: Logic " << temp_logics << " defined in "
                 << temp_links << " doesn't exist!\n";
            init_ok = false;
        }

        if (value.find(temp_logics) == string::npos)
        {
            cerr << "*** ERROR: The logic \"" << temp_logics <<
                    "\" configured in [" << temp_links <<
                    "] is not defined in [GLOBAL]-section\n";
            init_ok = false;
        }
     }
     LogicLinking::_instance->linkset[linkname].linklogics = clogics;

      // set the command, a command must be unique
     if (!cfg.getValue(temp_links, "COMMAND", value))
     {
        cerr << " *** ERROR: missing parameter " << temp_links
             << "/COMMAND=xxx e.g. COMMAND=64\n";
        init_ok = false;
     }
     LogicLinking::_instance->linkset[linkname].command = value;

      // check if a  timeout is set
     int timeout = atoi(cfg.getValue(temp_links, "TIMEOUT").c_str());
     if (timeout < 0 )
     {
        cerr << "*** ERROR: TIMEOUT must be zero or a positive value.\n";
        init_ok = false;
     }

     LogicLinking::_instance->linkset[linkname].timeout = timeout;


      // checking connect mode ( on_startup? and/or persistent? )
     if (cfg.getValue(temp_links, "CONNECTMODE", value))
     {

         // transform to upper case
        transform(value.begin(), value.end(), value.begin(), ::toupper);

         // logics should be connected on startup
        if (value.find("ON_STARTUP") != string::npos)
        {
           LogicLinking::_instance->linkset[linkname].on_startup = true;
        }

         // logics should beeing connected unlimited
        if (value.find("PERSISTENT") != string::npos)
        {
           LogicLinking::_instance->linkset[linkname].persistent = true;
        }
     }
  }

  if( !init_ok )
  {
    delete LogicLinking::_instance;
    LogicLinking::_instance = NULL;
  }

  return init_ok;

} /* LogicLinking::initialize */


bool LogicLinking::checkConnectOnStartup(std::string logicname)
{
   std::vector<string>::iterator vt;
   vector<string> ln = LogicLinking::instance()->getLinkName(logicname);

   for (vt = ln.begin(); vt != ln.end(); vt++)
   {
      if (LogicLinking::instance()->linkset[(*vt)].on_startup == true)
      {
         // cout << "Checkconnectonstartup " << (*vt) << "\n";
         if (!LogicLinking::instance()->connectLogics((*vt)))
         {
            return false;
         }
      }
   }
   return true;
} /* LogicLinking::checkConnectOnStartup */


// params:  (name of the logic, received command)
bool LogicLinking::connectThisLink(std::string logic_name, std::string cmd,
                                     std::string &link_name)
{
    std::map<string, LogicSet>::iterator it;
    std::vector<string> t_links;

    for (it = linkset.begin(); it != linkset.end(); ++it)
    {
       if (((*it).second).command == cmd)
       {

          t_links =((*it).second).linklogics;

          for ( std::vector<string>::iterator it1 = t_links.begin();
              it1 != t_links.end(); ++it1)
          {
             if ( (*it1) == logic_name)
             {
                link_name = ((*it).second).name;
                return connectLogics(((*it).second).name);
             }
          }
       }
    }

    return false;
} /* LogicLinking::connectThisLink */


bool LogicLinking::disconnectThisLink(std::string logic_name, std::string cmd,
                                        std::string &link_name)
{
    std::map<string, LogicSet>::iterator it;
    std::vector<string> t_links;

    for (it = linkset.begin(); it != linkset.end(); ++it)
    {
       if (((*it).second).command == cmd)
       {

          t_links =((*it).second).linklogics;

          for ( std::vector<string>::iterator it1 = t_links.begin();
              it1 != t_links.end(); ++it1)
          {
             if ( (*it1) == logic_name)
             {
                link_name = ((*it).second).name;
                return disconnectLogics(((*it).second).name);
             }
          }
       }
    }

    return false;
} /* LogicLinking::disconnectThisLink */


//                               name = LOGICNAME
bool LogicLinking::connectLogics(std::string logicname)
{
   bool check_connect = false;
   std::vector<std::string> connectlinks =
          LogicLinking::instance()->linkset[logicname].linklogics;

   assert(connectlinks.size() > 0);

   if (!audio_switch_matrix.checkLogics(connectlinks))
   {
      return false;
   }

   for (std::vector<string>::const_iterator it = connectlinks.begin();
            it != connectlinks.end(); it++)
   {
      for (vector<string>::const_iterator it1 = it+1; it1 != connectlinks.end();
	        it1++)
      {
         assert(*it != *it1);
         if (!LogicLinking::instance()->logicsAreConnected(*it, *it1))
         {
            cout << "Activating link " << *it << " <--> " << *it1 << endl;
            audio_switch_matrix.connect(*it, *it1);
            audio_switch_matrix.connect(*it1, *it);
            check_connect = true;
         }
      }
   }

    // starting timeout timer, when expired cut the link(s)
   if (LogicLinking::instance()->linkset[logicname].timeout > 0 )
   {
     delete timeoutTimerset[logicname];
     timeoutTimerset[logicname] = 0;
     timeoutTimerset[logicname] =
     new Async::Timer(LogicLinking::instance()->linkset[logicname].timeout*1000);

     timeoutTimerset[logicname]->expired.connect(slot(*this,
                                    &LogicLinking::linkTimeoutExpired));
   }

   return check_connect;
} /* LogicLinking::connectLogics */


bool LogicLinking::disconnectLogics(std::string logicname)
{
   bool check_connect = false;
   std::vector<std::string> connectlinks =
          LogicLinking::instance()->linkset[logicname].linklogics;

   assert(connectlinks.size() > 0);

   if (LogicLinking::instance()->linkset[logicname].persistent == true)
   {
      check_connect = false;
   }
   else
   {
     for (std::vector<string>::const_iterator it = connectlinks.begin();
       it != connectlinks.end(); it++)
     {
        for (vector<string>::const_iterator it1 = it+1;
                                              it1 != connectlinks.end();
	          it1++)
        {
           assert(*it != *it1);
           if (LogicLinking::instance()->logicsAreConnected(*it, *it1))
           {
             cout << "Deactivating link " << *it << " <--> " << *it1 << endl;
             audio_switch_matrix.disconnect(*it, *it1);
             audio_switch_matrix.disconnect(*it1, *it);
             check_connect = true;
           }
        }
     }
   }
   return check_connect;
} /* LogicLinking::disconnectLogics */


vector<string> LogicLinking::getLinkName(std::string sectionname)
{
   std::map<string, LogicSet>::iterator it;
   std::vector<string> val;
   std::vector<string>::iterator tval;

   for (it = LogicLinking::instance()->linkset.begin();
       it != LogicLinking::instance()->linkset.end(); it++)
   {
      for (tval = (*it).second.linklogics.begin();
             tval != (*it).second.linklogics.end(); tval++)
      {
         if ((*tval) == sectionname)
         {
            val.push_back((*it).second.name);
         }
      }
   }

   return val;
} /* LogicLinking::getName */


// returns the specific command of a link definition
std::vector<string> LogicLinking::getCommands(std::string logicname)
{
   std::string tcommand;
   std::vector<string> commands;
   std::vector<string>::iterator vt;
                                                   // [SimplexLogic]
   vector<string> ln = LogicLinking::instance()->getLinkName(logicname);

          // e.g. ln = Sim
   for (vt = ln.begin(); vt != ln.end(); ++vt)
   {
      tcommand = LogicLinking::instance()->linkset[(*vt)].command;
      commands.push_back(tcommand);
   }

   return commands;
} /* LogicLinking::getCommand */


// reset the timers for the links, that are connected to eachother
void LogicLinking::resetTimers(string logicname)
{
   std::vector<string>::iterator vt;
   std::map<std::string, Async::Timer*>::iterator it;
   vector<string> ln = LogicLinking::instance()->getLinkName(logicname);

   for (vt = ln.begin(); vt != ln.end(); ++vt)
   {
      it = timeoutTimerset.find(*vt);
      if ( it != timeoutTimerset.end())
      {
         // reset each Timer connected with the specified logic
        (it->second)->reset();
      }
   }
} /* LogicLinking::resetTimers */


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

bool LogicLinking::logicsAreConnected(const string& l1, const string& l2)
{
   return audio_switch_matrix.isConnected(l1, l2);
} /* LogicLinking::logicsAreConnected */


// will be exceuted by an expired link timer
void LogicLinking::linkTimeoutExpired(Timer *t)
{
   for (std::map<std::string, Async::Timer*>::iterator
        it = timeoutTimerset.begin(); it != timeoutTimerset.end(); ++it)
   {
     if (t == it->second)       // compare pointers
     {
        LogicLinking::instance()->disconnectLogics(it->first);
        delete it->second;
        timeoutTimerset.erase(it);
        break;
     }
   }
} /* LogicLinking::linkTimeoutTimer */



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
