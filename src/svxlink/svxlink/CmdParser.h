/**
@file	 CmdParser.h
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

/** @example CmdParser_demo.cpp
An example of how to use the CmdParser class
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
@brief	A_brief_class_description
@author Tobias Blomberg
@date   2005-04-24

A_detailed_class_description

\include CmdParser_demo.cpp
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


class Command : public SigC::Object
{
  public:
    Command(CmdParser *parser, const std::string& cmd="")
      : parser(parser)
    {
      if (!cmd.empty())
      {
      	setCmd(cmd);
      }
    }
    
    virtual ~Command(void)
    {
      parser->removeCmd(this);
    }
    
    bool addToParser(void)
    {
      return parser->addCmd(this);
    }
    
    bool removeFromParser(void)
    {
      return parser->addCmd(this);
    }
    
    const std::string& cmdStr(void) const { return cmd; }
    
    virtual void operator ()(const std::string& subcmd)
    {
      handleCmd(this, subcmd);
    }
    
    SigC::Signal2<void, Command *, const std::string&> handleCmd;

  protected:
    void setCmd(const std::string& cmd_str)
    {
      assert(!cmd_str.empty());
      assert(cmd.empty());
      cmd = cmd_str;
    }
    
  private:
    CmdParser 	*parser;
    std::string cmd;
    
};  /* class Command */







//} /* namespace */

#endif /* CMD_PARSER_INCLUDED */



/*
 * This file has not been truncated
 */

