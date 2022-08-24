/**
@file	 ModuleHelp.cpp
@brief   A module that implements a help system for the user
@author  Tobias Blomberg / SM0SVX
@date	 2004-03-07

\verbatim
A module (plugin) for the multi purpose tranciever frontend system.
Copyright (C) 2004  Tobias Blomberg

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

#include <cstdio>
#include <cstdlib>

#include <iostream>
#include <sstream>


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

#include "version/MODULE_HELP.h"
#include "ModuleHelp.h"



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
 * Pure C-functions
 *
 ****************************************************************************/


extern "C" {
  Module *module_init(void *dl_handle, Logic *logic, const char *cfg_name)
  {
    return new ModuleHelp(dl_handle, logic, cfg_name);
  }
} /* extern "C" */



/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/


ModuleHelp::ModuleHelp(void *dl_handle, Logic *logic, const string& cfg_name)
  : Module(dl_handle, logic, cfg_name)
{
  cout << "\tModule Help v" MODULE_HELP_VERSION " starting...\n";

} /* ModuleHelp */


ModuleHelp::~ModuleHelp(void)
{

} /* ~ModuleHelp */





/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/

void ModuleHelp::resumeOutput(void)
{
  
} /* ModuleHelp::resumeOutput */


void ModuleHelp::allSamplesFlushed(void)
{
  
} /* ModuleHelp::allSamplesFlushed */


int ModuleHelp::writeSamples(const float *samples, int count)
{
  return count;
} /* ModuleHelp::writeSamples */


void ModuleHelp::flushSamples(void)
{
  sourceAllSamplesFlushed();
} /* ModuleHelp::flushSamples */



/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/


/*
 *----------------------------------------------------------------------------
 * Method:    activateInit
 * Purpose:   Called by the core system when this module is activated.
 * Input:     None
 * Output:    None
 * Author:    Tobias Blomberg / SM0SVX
 * Created:   2004-03-07
 * Remarks:   
 * Bugs:      
 *----------------------------------------------------------------------------
 */
void ModuleHelp::activateInit(void)
{
  playChooseModuleMsg();
} /* activateInit */


/*
 *----------------------------------------------------------------------------
 * Method:    deactivateCleanup
 * Purpose:   Called by the core system when this module is deactivated.
 * Input:     None
 * Output:    None
 * Author:    Tobias Blomberg / SM0SVX
 * Created:   2004-03-07
 * Remarks:   Do NOT call this function directly unless you really know what
 *    	      you are doing. Use Module::deactivate() instead.
 * Bugs:      
 *----------------------------------------------------------------------------
 */
void ModuleHelp::deactivateCleanup(void)
{
  
} /* deactivateCleanup */


/*
 *----------------------------------------------------------------------------
 * Method:    dtmfDigitReceived
 * Purpose:   Called by the core system when a DTMF digit has been
 *    	      received.
 * Input:     digit   	- The DTMF digit received (0-9, A-D, *, #)
 *    	      duration	- The duration in milliseconds
 * Output:    Return true if the digit is handled or false if not
 * Author:    Tobias Blomberg / SM0SVX
 * Created:   2004-03-07
 * Remarks:   
 * Bugs:      
 *----------------------------------------------------------------------------
 */
bool ModuleHelp::dtmfDigitReceived(char digit, int duration)
{
  //cout << "DTMF digit received in module " << name() << ": " << digit << endl;
  
  return false;
  
} /* dtmfDigitReceived */


/*
 *----------------------------------------------------------------------------
 * Method:    dtmfCmdReceived
 * Purpose:   Called by the core system when a DTMF command has been
 *    	      received. A DTMF command consists of a string of digits ended
 *    	      with a number sign (#). The number sign is not included in the
 *    	      command string.
 * Input:     cmd - The received command.
 * Output:    None
 * Author:    Tobias Blomberg / SM0SVX
 * Created:   2004-03-07
 * Remarks:   
 * Bugs:      
 *----------------------------------------------------------------------------
 */
void ModuleHelp::dtmfCmdReceived(const string& cmd)
{
  cout << "DTMF command received in module " << name() << ": " << cmd << endl;
  
  if (cmd == "")
  {
    deactivateMe();
  }
  else
  {
    //setIdle(false); /* Reset the module timeout timer */
    //setIdle(true);
    
    int module_id = atoi(cmd.c_str());
    Module *module = findModule(module_id);
    if (module != 0)
    {
      module->playHelpMsg();
      playChooseModuleMsg();
    }
    else
    {
      stringstream ss;
      ss << "no_such_module " << module_id;
      processEvent(ss.str());
    }
  }
} /* dtmfCmdReceived */


void ModuleHelp::dtmfCmdReceivedWhenIdle(const std::string &cmd)
{
  stringstream ss(cmd);
  int module_id;
  ss >> module_id;
  Module *module = findModule(module_id);
  if (module != 0)
  {
    module->playHelpMsg();
  }
  else
  {
    stringstream ss;
    ss << "no_such_module " << module_id;
    processEvent(ss.str());
  }
} /* dtmfCmdReceivedWhenIdle */


void ModuleHelp::playChooseModuleMsg(void)
{
  stringstream ss;
  ss << "choose_module [list";

  list<Module*> modules = moduleList();
  list<Module*>::const_iterator it;
  for (it=modules.begin(); it!=modules.end(); ++it)
  {
    ss << " " << (*it)->id() << " " << (*it)->name();
  }
  ss << "]";
  processEvent(ss.str());

} /* ModuleHelp::playChooseModuleMsg */



/*
 * This file has not been truncated
 */
