/**
@file	 PhoneCmds.h
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


#ifndef PHONE_CMDS_INCLUDED
#define PHONE_CMDS_INCLUDED


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
#include "svxlink.h"


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
class PhoneCmd : public Command
{
  public:
    /**
     * @brief 	Default constuctor
     */
    PhoneCmd(CmdParser *parser, Logic *logic)
      : Command(parser), logic(logic), timeout(0) {}

    /**
     * @brief 	Destructor
     */
    ~PhoneCmd(void) {}

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

      if (!cfg.getValue(link_name, "LOGIC2", phonelogic))
      {
	    std::cerr << "*** ERROR: Config variable " << link_name
	     << "/LOGIC1 not set\n";
	    return false;
      }

      if (!cfg.getValue(link_name, "LOGIC1", rflogic))
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

    void connectPhoneline(void)
    {
      if (Logic::logicsAreConnected(phonelogic, rflogic))
	  {
	     logic->processEvent("phoneline_already_active");
	  }
      else
	  {
         std::vector< AnalogPhoneLogic * > phones = get_analogphone_logics();

         // pickup phonelogic
         for( std::vector< AnalogPhoneLogic * >::iterator phone_it = phones.begin();
            phone_it != phones.end(); phone_it++ ) (*phone_it)->pickupRemote("by remote");
	 std::vector<std::string> logics;
	 logics.push_back(phonelogic);
	 logics.push_back(rflogic);
         Logic::connectLogics(logics, timeout);
         //logic->processEvent("incoming_phonecall");
	  }
    } /* connectPhoneline */


    void disconnectPhoneline(void)
    {
      if (!Logic::logicsAreConnected(phonelogic, rflogic))
      {
          logic->processEvent("phoneline_not_active");
	  }
	  else
      {
      //   std::vector< AnalogPhoneLogic * > phones = get_analogphone_logics();

         // pickup phonelogic
       //  for( std::vector< AnalogPhoneLogic * >::iterator phone_it = phones.begin();
        //    phone_it != phones.end(); phone_it++ ) (*phone_it)->hangupRemote("by_remote");
	 std::vector<std::string> logics;
	 logics.push_back(phonelogic);
	 logics.push_back(rflogic);
         Logic::disconnectLogics(logics);
      }
    } /* disconnectPhoneline */


    void operator ()(const std::string& subcmd)
    {
      std::stringstream ss;
      //  std::cout << "cmd=" << cmdStr() << " subcmd=" << subcmd << std::endl;
      std::vector< AnalogPhoneLogic * > phones = get_analogphone_logics();

      // disconnect from repeater

      if (subcmd == "8" && Logic::logicsAreConnected(phonelogic, rflogic))
      {
         for( std::vector< AnalogPhoneLogic * >::iterator phone_it = phones.begin();
            phone_it != phones.end(); phone_it++ ) (*phone_it)->discPhone("by_remote");
      }

      // connect to repeater to authenticate with e.g. 959#
      else if (subcmd == "9" && !Logic::logicsAreConnected(phonelogic, rflogic))
      {
         for( std::vector< AnalogPhoneLogic * >::iterator phone_it = phones.begin();
            phone_it != phones.end(); phone_it++ ) (*phone_it)->requestAuth("by_remote");
      }

      else
      {
        ss << "command_failed " << cmdStr() << subcmd;
        logic->processEvent(ss.str());
      }
    }

  protected:

  private:
    Logic     	*logic;
    std::string phonelogic;
    std::string rflogic;
    int       	timeout;
    std::string name;

};  /* class LinkCmd */


//} /* namespace */

#endif /* PHONE_CMDS_INCLUDED */


/*
 * This file has not been truncated
 */

