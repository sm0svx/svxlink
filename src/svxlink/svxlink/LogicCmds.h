/**
@file	 LogicCmds.h
@brief   A_brief_description_for_this_file
@author  Tobias Blomberg / SM0SVX
@date	 2005-04-24

A_detailed_description_for_this_file

\verbatim
<A brief description of the program or library this file belongs to>
Copyright (C) 2004-2005  Tobias Blomberg / SM0SVX

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

/** @example LogicCmds_demo.cpp
An example of how to use the LogicCmds class
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


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "CmdParser.h"
#include "Logic.h"
#include "Module.h"


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
@brief	A_brief_class_description
@author Tobias Blomberg
@date   2005-04-24

A_detailed_class_description
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
      if (!subcmd.empty())
      {
      	std::stringstream ss;
	ss << "command_failed " << cmdStr() << subcmd;
      	logic->processEvent(ss.str());
	return;
      }
      int module_id = atoi(cmdStr().c_str());
      Module *module = logic->findModule(module_id);
      assert(module != 0);
      logic->activateModule(module);
    }
    
  protected:
    
  private:
    Logic *logic;
    
};  /* class ModuleActivateCmd */


/**
@brief	A_brief_class_description
@author Tobias Blomberg
@date   2005-04-24

A_detailed_class_description
*/
class LinkCmd : public Command
{
  public:
    /**
     * @brief 	Default constuctor
     */
    LinkCmd(CmdParser *parser, Logic *logic)
      : Command(parser), logic(logic), timeout(0) {}
  
    /**
     * @brief 	Destructor
     */
    ~LinkCmd(void) {}
  
    /**
     * @brief 	A_brief_member_function_description
     * @param 	param1 Description_of_param1
     * @return	Return_value_of_this_member_function
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

      if (!cfg.getValue(link_name, "LOGIC1", logic1))
      {
	std::cerr << "*** ERROR: Config variable " << link_name
	     << "/LOGIC1 not set\n";
	return false;
      }

      if (!cfg.getValue(link_name, "LOGIC2", logic2))
      {
	std::cerr << "*** ERROR: Config variable " << link_name
	     << "/LOGIC2 not set\n";
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
	std::stringstream ss;
	if (!Logic::logicsAreConnected(logic1, logic2))
	{
	  ss << "link_not_active " << name;
	}
	else
	{
      	  Logic::disconnectLogics(logic1, logic2);
	  ss << "deactivating_link " << name;
	}
      	logic->processEvent(ss.str());
      }
      else if (subcmd == "1")
      {
	std::stringstream ss;
	if (Logic::logicsAreConnected(logic1, logic2))
	{
	  ss << "link_already_active " << name;
	}
	else
	{
      	  Logic::connectLogics(logic1, logic2, timeout);
	  ss << "activating_link " << name;
	}
      	logic->processEvent(ss.str());
      }
      /*
      else if (subcmd == "2")
      {
	std::stringstream ss;
	if (Logic::logicsAreConnected(logic1, logic2))
	{
	  ss << "link_already_active " << name;
	}
	else
	{
      	  Logic::connectLogics(logic1, logic2);
	  ss << "activating_link " << name;
	}
      	logic->processEvent(ss.str());
      }
      */
      else
      {
      	std::stringstream ss;
	ss << "command_failed " << cmdStr() << subcmd;
      	logic->processEvent(ss.str());
      }
    }
    
  protected:
    
  private:
    Logic     	*logic;
    std::string logic1;
    std::string logic2;
    int       	timeout;
    std::string name;
        
};  /* class LinkCmd */




//} /* namespace */

#endif /* LOGIC_CMDS_INCLUDED */



/*
 * This file has not been truncated
 */

