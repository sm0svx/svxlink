/**
@file	 CmdParser.cpp
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



/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

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

#include "CmdParser.h"



/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/

using namespace std;



/****************************************************************************
 *
 * Defines & typedefs
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Local class definitions
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Prototypes
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Exported Global Variables
 *
 ****************************************************************************/




/****************************************************************************
 *
 * Local Global Variables
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/

CmdParser::~CmdParser(void)
{
  CmdMap cmds_copy = cmds;
  CmdMap::iterator it;
  for (it = cmds_copy.begin(); it != cmds_copy.end(); ++it)
  {
    delete (*it).second;
  }
} /* CmdParser::~CmdParser */


bool CmdParser::addCmd(Command *cmd)
{
  bool cmd_undefined = (cmds.count(cmd->cmdStr()) == 0);
  if (cmd_undefined)
  {
    cmds[cmd->cmdStr()] = cmd;
  }
  return cmd_undefined;
} /* CmdParser::addCmd */


bool CmdParser::removeCmd(Command *cmd)
{
  CmdMap::iterator cmd_it = cmds.find(cmd->cmdStr());
  bool cmd_exist = (cmd_it != cmds.end());
  if (cmd_exist)
  {
    cmds.erase(cmd_it);
  }
  return cmd_exist;
} /* CmdParser::removeCmd */


bool CmdParser::processCmd(const string& cmd_str)
{
  int len = cmd_str.size();
  while (len > 0)
  {
    const std::string base_cmd = cmd_str.substr(0, len);
    if (cmds.count(base_cmd) == 1)
    {
      Command *cmd = cmds[base_cmd];
      if (!cmd->exactMatch() || (cmd_str.size() == cmd->cmdStr().size()))
      {
        (*cmd)(cmd_str.substr(len, cmd_str.size()-len));
        return true;
      }
    }
    --len;
  }

  return false;
  
} /* CmdParser::processCmd */
    



/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/


/*
 *------------------------------------------------------------------------
 * Method:    
 * Purpose:   
 * Input:     
 * Output:    
 * Author:    
 * Created:   
 * Remarks:   
 * Bugs:      
 *------------------------------------------------------------------------
 */






/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/


/*
 *----------------------------------------------------------------------------
 * Method:    
 * Purpose:   
 * Input:     
 * Output:    
 * Author:    
 * Created:   
 * Remarks:   
 * Bugs:      
 *----------------------------------------------------------------------------
 */







/*
 * This file has not been truncated
 */

