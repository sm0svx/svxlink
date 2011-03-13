/**
@file	 LogicCmds.h
@brief   This file contains the implemented core logic commands
@author  Tobias Blomberg / SM0SVX
@date	 2005-04-24

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2004-2010  Tobias Blomberg / SM0SVX

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

#ifndef LOGIC_CMDS_INCLUDED
#define LOGIC_CMDS_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <iostream>
#include <sstream>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncConfig.h>
#include <common.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "CmdParser.h"
#include "Logic.h"
#include "Module.h"
#include "QsoRecorder.h"


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

//namespace MyNameSpace
//{


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
 * Exported Global Variables
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Class definitions
 *
 ****************************************************************************/

/**
@brief	The module activation command
@author Tobias Blomberg
@date   2005-04-24

This class implements the core command used to activate a module.
The subcommand represents the ID of the module to activate.
*/
class ModuleActivateCmd : public Command
{
  public:
    /**
     * @brief 	Default constuctor
     */
    ModuleActivateCmd(CmdParser *parser, const std::string& cmd, Logic *logic)
      : Command(parser, cmd), logic(logic) {}

    /**
     * @brief 	Destructor
     */
    ~ModuleActivateCmd(void) {}

    /**
     * @brief 	A_brief_member_function_description
     * @param 	param1 Description_of_param1
     * @return	Return_value_of_this_member_function
     */
    void operator ()(const std::string& subcmd)
    {
      //std::cout << "cmd=" << cmdStr() << " subcmd=" << subcmd << std::endl;
      int module_id = atoi(cmdStr().c_str());
      Module *module = logic->findModule(module_id);
      assert(module != 0);
      if (!subcmd.empty())
      {
	module->dtmfCmdReceivedWhenIdle(subcmd);
      }
      else
      {
        if (!logic->activateModule(module))
	{
	  std::stringstream ss;
	  ss << "command_failed " << cmdStr() << subcmd;
	  logic->processEvent(ss.str());
	}
      }
    }

  protected:

  private:
    Logic *logic;

};  /* class ModuleActivateCmd */


/**
@brief	The logic linking activation command
@author Tobias Blomberg
@date   2005-04-24

This class implements the logic linking activation command.
Each instance of the class represents a group of logics that can be linked
together.
Subcommand 0 means disconnect the links and subcommand 1 means connect the
links.
*/
class LinkCmd : public Command
{
  public:
    /**
     * @brief 	Constuctor
     * @param  parser The command parser to associate this command with
     * @param  logic  The logic core instance the command should operate on
     */
    LinkCmd(CmdParser *parser, Logic *logic)
      : Command(parser), logic(logic), timeout(0) {}

    /**
     * @brief 	Destructor
     */
    ~LinkCmd(void) {}

    /**
     * @brief 	Initialize the command object
     * @param 	cfg A previously initialized configuration object
     * @param  link_name The name (configuration section) of this link
     * @return	Returns \em true on success or else \em false.
     */
    bool initialize(Async::Config& cfg, const std::string& link_name)
    {
      if (!cfg.getValue(link_name, "NAME", name))
      {
      	name = link_name;
      }

      std::string cmd_str;
      if (!cfg.getValue(link_name, "COMMAND", cmd_str))
      {
	std::cerr << "*** ERROR: Config variable " << link_name
	     << "/COMMAND not set\n";
	return false;
      }
      setCmd(cmd_str);
      if (!addToParser())
      {
	std::cerr << "*** ERROR: Could not set up command \"" << cmd_str
		  << "\" for logic link \"" << name << "\". You probably have "
		  << "the same command set up in more than one place\n";
	return false;
      }

      if (!cfg.getValue(link_name, "CONNECT_LOGICS", logics))
      {
	std::cerr << "*** ERROR: Config variable " << link_name
	     << "/CONNECT_LOGICS not set\n";
	return false;
      }

      if (SvxLink::splitStr(logic_list, logics, ",") < 2)
      {
	std::cerr << "*** ERROR: you need at least two LOGICS to connect,"
	    << "e.g. CONNECT_LOGICS=RepeaterLogic,SimplexLogic" << std::endl;
	return false;
      }

      std::string value;
      if (cfg.getValue(link_name, "TIMEOUT", value))
      {
      	timeout = atoi(value.c_str());
      }

      return true;
    }

    void operator ()(const std::string& subcmd)
    {
      //std::cout << "cmd=" << cmdStr() << " subcmd=" << subcmd << std::endl;
      if (subcmd == "0")
      {
        disconnectLinks();
      }
      else if (subcmd == "1")
      {
	connectLinks();
      }
      else
      {
      	std::stringstream ss;
	ss << "command_failed " << cmdStr() << subcmd;
      	logic->processEvent(ss.str());
      }
    }

    void connectLinks(void)
    {
      std::stringstream ss;
      if (Logic::connectLogics(logic_list, timeout))
      {
	ss << "activating_link " << name;
      }
      else
      {
	ss << "link_already_active " << name;
      }
      logic->processEvent(ss.str());
    } /* connectLinks */


    void disconnectLinks(void)
    {
      std::stringstream ss;
      if (Logic::disconnectLogics(logic_list))
      {
	ss << "deactivating_link " << name;
      }
      else
      {
	ss << "link_not_active " << name;
      }
      logic->processEvent(ss.str());
    } /* disconnectLinks */


  protected:

  private:
    typedef std::vector<std::string> StrList;

    Logic     	*logic;
    std::string logics;
    int       	timeout;
    std::string name;
    StrList 	logic_list;


};  /* class LinkCmd */


class QsoRecorderCmd : public Command
{
  public:
    /**
     * @brief 	Constuctor
     * @param  	parser	  The command parser to associate this command with
     * @param  	logic	  The logic core instance the command should operate on
     * @param	recorder  A previously created QsoRecorder object
     */
    QsoRecorderCmd(CmdParser *parser, Logic *logic, QsoRecorder *recorder)
      : Command(parser), logic(logic), recorder(recorder) {}

    /**
     * @brief 	Destructor
     */
    ~QsoRecorderCmd(void) {}

    /**
     * @brief 	Initialize the command object
     * @param 	cmd_str The command base
     * @return	Returns \em true on success or \em false on failure
     */
    bool initialize(const std::string &cmd_str)
    {
      setCmd(cmd_str);
      return addToParser();
    }

    void operator ()(const std::string& subcmd)
    {
      if (subcmd == "0")
      {
        if (recorder->isEnabled())
        {
          recorder->setEnabled(false);
          logic->processEvent("deactivating_qso_recorder");
        }
        else
        {
          logic->processEvent("qso_recorder_not_active");
        }
      }
      else if (subcmd == "1")
      {
        if (!recorder->isEnabled())
        {
          recorder->setEnabled(true);
          logic->processEvent("activating_qso_recorder");
        }
        else
        {
          logic->processEvent("qso_recorder_already_active");
        }
      }
      else
      {
      	std::stringstream ss;
	ss << "command_failed " << cmdStr() << subcmd;
      	logic->processEvent(ss.str());
      }
    }

  private:
    Logic       *logic;
    QsoRecorder *recorder;

};  /* class QsoRecorderCmd */


class ChangeLangCmd : public Command
{
  public:
    ChangeLangCmd(CmdParser *parser, Logic *logic)
      : Command(parser, "00"), logic(logic)
    {
    }

    void operator ()(const std::string& subcmd)
    {
      std::stringstream ss;
      if (subcmd.empty())
      {
	ss << "list_languages";
      }
      else
      {
	ss << "set_language ";
	ss << subcmd;
      }
      logic->processEvent(ss.str());
    }

  private:
    Logic       *logic;

};


//} /* namespace */

#endif /* LOGIC_CMDS_INCLUDED */



/*
 * This file has not been truncated
 */

