/**
@file	 LogicCmds.h
@brief   This file contains the implemented core logic commands
@author  Tobias Blomberg / SM0SVX
@date	 2005-04-24

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2004-2025  Tobias Blomberg / SM0SVX

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
#include "LinkManager.h"


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
     * @brief   Constuctor
     * @param   parser  The command parser to associate this command with
     * @param   logic   The logic core instance the command should operate on
     */
    ModuleActivateCmd(CmdParser *parser, const std::string& cmd, Logic *logic)
      : Command(parser, cmd), logic(logic) {}

    /**
     * @brief 	Destructor
     */
    virtual ~ModuleActivateCmd(void) {}

    /**
     * @brief   Handle the command
     * @param   subcmd The part of the command not matching the command base
     */
    virtual void operator ()(const std::string& subcmd) override
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

The LinkCmd objects are created from the LinkManager class when a logic
core register itself.
*/
class LinkCmd : public Command
{
  public:
    /**
     * @brief   Constuctor
     * @param   logic The logic core instance the command should operate on
     * @param   link  The link instance the command should operate on
     */
    LinkCmd(Logic *logic, LinkManager::LinkRef link)
      : Command(logic->cmdParser()), logic(logic), link(link) {}

    /**
     * @brief 	Destructor
     */
    virtual ~LinkCmd(void) {}

    /**
     * @brief   Initialize the command object
     * @param   command The base command to use to control the link
     * @return  Returns \em true on success or else \em false.
     */
    bool initialize(const std::string& command)
    {
      setCmd(command);
      if (!addToParser())
      {
        std::cerr << "*** ERROR: Could not set up logic linking command \""
           << command << "\" for logic \"" << logic->name()
           << "\". You probably have the same command set up in more than "
           << "one places\n";
        return false;
      }
      return true;
    }

    /**
     * @brief   Handle the command
     * @param   subcmd The part of the command not matching the command base
     */
    virtual void operator ()(const std::string& subcmd) override
    {
      std::string event =
          LinkManager::instance()->cmdReceived(link, logic, subcmd);
      if (!event.empty())
      {
        logic->processEvent(event);
      }
    }

  protected:

  private:
    Logic                 *logic;
    LinkManager::LinkRef  link;

};  /* class LinkCmd */


/**
@brief	The QSO recorder command
@author Tobias Blomberg
@date   2013-08-23

This command is used to activate and deactivate the QSO recorder feature.
The QSO recorder will record all traffic on the node to a file.

Subcommand 1 activates the QSO recorder and subcommand 0 will deactivate it.
*/
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
    virtual ~QsoRecorderCmd(void) {}

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

    /**
     * @brief   Handle the command
     * @param   subcmd The part of the command not matching the command base
     */
    virtual void operator ()(const std::string& subcmd) override
    {
      if (subcmd == "0")
      {
        if (recorder->isEnabled())
        {
          //std::cout << logic->name() << ": Deactivating QSO recorder\n";
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
          //std::cout << logic->name() << ": Activating QSO recorder\n";
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


/**
@brief	The change language command
@author Tobias Blomberg
@date   2013-08-23

This command is used to switch the language that is used on the node for
announcements. The base command is always "00" followed by the country
prefix. The country prefix is the same as in the international phone system
so US english will have prefix 1 and Swedish will have prefix 46.
*/
class ChangeLangCmd : public Command
{
  public:
    ChangeLangCmd(CmdParser *parser, Logic *logic)
      : Command(parser, "00"), logic(logic)
    {
    }

    /**
     * @brief   Handle the command
     * @param   subcmd The part of the command not matching the command base
     */
    virtual void operator ()(const std::string& subcmd) override
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


/**
@brief	The online command
@author Tobias Blomberg
@date   2013-08-23

This command is used to set the logic offline. In offline mode the logic will
turn the transmitter off and stop accepting any other commands than the
logic online command. Any active module will be deactivated when the logic is
set to offline and module activation while the logic is offline will be
prohibited.

The logic online command is not handled here but in Logic.cpp in the
checkIfOnlineCmd function.

The name of this class may seem awkward (OnlineCmd) since it's used to bring
the logic offline but it's more consistent with what the actual config
variable is named, ONLINE_CMD.
*/
class OnlineCmd : public Command
{
  public:
    /**
     * @brief   Constuctor
     * @param   parser  The command parser to associate this command with
     * @param   logic   The logic core instance the command should operate on
     * @param   cmd     The base command string
     */
    OnlineCmd(CmdParser *parser, Logic *logic, const std::string &cmd)
      : Command(parser, cmd), logic(logic)
    {
    }

    /**
     * @brief   Handle the command
     * @param   subcmd The part of the command not matching the command base
     */
    virtual void operator ()(const std::string& subcmd) override
    {
      if (subcmd == "0")
      {
        logic->setOnline(false);
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

};


/**
@brief	The macro command handler
@author Tobias Blomberg
@date   2025-07-02

This class handle macro commands.
*/
class MacroCmd : public Command
{
  public:
    /**
     * @brief   Constuctor
     * @param   parser  The command parser to associate this command with
     * @param   cmd     The base command string
     * @param   logic   The logic core instance the command should operate on
     */
    MacroCmd(CmdParser *parser, const std::string& cmd, Logic *logic)
      : Command(parser, cmd), m_logic(logic)
    {
      setExactMatch();
    }

    /**
     * @brief 	Destructor
     */
    virtual ~MacroCmd(void) override {}

    /**
     * @brief   Handle the command
     * @param   subcmd The part of the command not matching the command base
     */
    virtual void operator ()(const std::string& subcmd) override
    {
      //std::cout << "### MacroCmd[" << m_logic->name() << "]: cmd=" << cmdStr()
      //          << " subcmd=" << subcmd << std::endl;
      assert(subcmd.empty());
      m_logic->processMacroCmd(cmdStr());
    }

  protected:

  private:
    Logic*  m_logic;

};  /* class MacroCmd */


//} /* namespace */

#endif /* LOGIC_CMDS_INCLUDED */



/*
 * This file has not been truncated
 */

