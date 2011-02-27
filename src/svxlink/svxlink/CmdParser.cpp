/**
@file	 CmdParser.cpp
@brief   A_brief_description_for_this_file
@author  Tobias Blomberg / SM0SVX
@date	 2005-04-24

A_detailed_description_for_this_file

\verbatim
<A brief description of the program or library this file belongs to>
Copyright (C) 2003 Tobias Blomberg / SM0SVX

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
  if (cmds.count(cmd->cmdStr()) != 0)
  {
    return false;
  }
  cmds[cmd->cmdStr()] = cmd;
  return true;
} /* CmdParser::addCmd */


bool CmdParser::removeCmd(Command *cmd)
{
  bool cmd_exist = (cmds.count(cmd->cmdStr()) == 1);
  if (cmd_exist)
  {
    cmds.erase(cmd->cmdStr());
  }
  return cmd_exist;
} /* CmdParser::removeCmd */


bool CmdParser::processCmd(const string& cmd_str)
{
  int len = cmd_str.size();
  while (len > 0)
  {
    if (cmds.count(cmd_str.substr(0, len)) > 0)
    {
      Command *cmd = cmds[cmd_str.substr(0, len)];
      (*cmd)(cmd_str.substr(len, cmd_str.size()-len));
      return true;
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

