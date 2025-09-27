/**
@file	 CmdParser.h
@brief   A command parser for DTMF commands
@author  Tobias Blomberg / SM0SVX
@date	 2005-04-24

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2025 Tobias Blomberg / SM0SVX

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

#ifndef CMD_PARSER_INCLUDED
#define CMD_PARSER_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <sigc++/sigc++.h>

#include <map>
#include <string>
#include <cassert>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/



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

class Command;
  

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
@brief	Command parser for DTMF commands
@author Tobias Blomberg
@date   2005-04-24

This is the DTMF command parser engine implementation. Add commands based on
the Command class.
*/
class CmdParser
{
  public:
    /**
     * @brief 	Default constuctor
     */
    CmdParser(void) {}
  
    /**
     * @brief 	Destructor
     */
    ~CmdParser(void);
  
    /**
     * @brief 	Add a command to the parser
     * @param 	cmd The command to add
     * @return	Returns \em true on success or else \em false
     */
    bool addCmd(Command *cmd);
    
    /**
     * @brief	Remove a command from the parser
     * @param	cmd The command to remove
     * @return	Returns \em true on success or else \em false
     */
    bool removeCmd(Command *cmd);
    
    /**
     * @brief	Process a command string
     * @param	cmd_str The command string to process
     * @return	Returns \em true if the command was found or else \em false
     */
    bool processCmd(const std::string& cmd_str);
    
    
  protected:
    
  private:
    typedef std::map<std::string, Command *> CmdMap;
    CmdMap cmds;
    
};  /* class CmdParser */


class Command : public sigc::trackable
{
  public:
    /**
     * @brief	Constructor
     * @param	parser	The associated parser
     * @param	cmd	The command string for the base command
     */
    Command(CmdParser *parser, const std::string& cmd="")
      : parser(parser)
    {
      if (!cmd.empty())
      {
      	setCmd(cmd);
      }
    }
    
    /**
     * @brief	Destructor
     */
    virtual ~Command(void)
    {
      parser->removeCmd(this);
    }
    
    /**
     * @brief	Add this command to the associated parser
     * @return	Returns \em true if the command could be added, otherwise
     *          \em false is returned.
     */
    bool addToParser(void)
    {
      return parser->addCmd(this);
    }
    
    /**
     * @brief	Remove this command from the associated parser
     * @return	Returns \em true if the command could be removed, otherwise
     *          \em false is returned.
     */
    bool removeFromParser(void)
    {
      return parser->removeCmd(this);
    }
    
    /**
     * @brief	Get the command string
     * @return	Returns the command string
     */
    const std::string& cmdStr(void) const { return cmd; }

    /**
     * @brief   Find out if this command require an exact match
     * @return  Returns \em true if an exact match is required
     */
    bool exactMatch(void) const { return m_exact_match; }

    /**
     * @brief	Execute this command
     * @param	subcmd The sub command of the executed command
     */
    virtual void operator ()(const std::string& subcmd)
    {
      handleCmd(this, subcmd);
    }
    
    /**
     * @brief	A signal that is emitted to handle the command
     * @param	cmd The Command object
     * @param	subcmd The sub command of the executed command
     *
     * This signal is emitted unless the operator() is reimplemented.
     */
    sigc::signal<void, Command *, const std::string&> handleCmd;

  protected:
    /**
     * @brief	Set up command string
     * @param	cmd_str	The command string to use for this command
     *
     * The command to set must not be empty and a command string must not have
     * been previously setup.
     */
    void setCmd(const std::string& cmd_str)
    {
      assert(!cmd_str.empty());
      assert(cmd.empty());
      cmd = cmd_str;
    }

    void setExactMatch(bool exact_match=true)
    {
      m_exact_match = exact_match;
    }

  private:
    CmdParser*  parser        {nullptr};
    std::string cmd;
    bool        m_exact_match {false};

};  /* class Command */



//} /* namespace */

#endif /* CMD_PARSER_INCLUDED */



/*
 * This file has not been truncated
 */

