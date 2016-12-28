/**
@file	 ModuleTcl.cpp
@brief   A module implementing an API so that modules can be written in TCL
@author  Tobias Blomberg / SM0SVX
@date	 2005-08-28

\verbatim
A module (plugin) for the multi purpose tranciever frontend system.
Copyright (C) 2004-2010 Tobias Blomberg

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

#include <stdio.h>

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

#include "version/MODULE_TCL.h"
#include "ModuleTcl.h"



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
    return new ModuleTcl(dl_handle, logic, cfg_name);
  }
} /* extern "C" */



/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/


ModuleTcl::ModuleTcl(void *dl_handle, Logic *logic, const string& cfg_name)
  : Module(dl_handle, logic, cfg_name)
{
  cout << "\tModule Tcl v" MODULE_TCL_VERSION " starting...\n";

} /* ModuleTcl */


ModuleTcl::~ModuleTcl(void)
{

} /* ~ModuleTcl */





/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/


void ModuleTcl::resumeOutput(void)
{

} /* ModuleTcl::resumeOutput */


void ModuleTcl::allSamplesFlushed(void)
{

} /* ModuleTcl::allSamplesFlushed */


int ModuleTcl::writeSamples(const float *samples, int count)
{
  return count;
} /* ModuleTcl::writeSamples */


void ModuleTcl::flushSamples(void)
{
  sourceAllSamplesFlushed();
} /* ModuleTcl::flushSamples */






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
void ModuleTcl::activateInit(void)
{
  processEvent("activateInit");
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
void ModuleTcl::deactivateCleanup(void)
{
  processEvent("deactivateCleanup");
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
bool ModuleTcl::dtmfDigitReceived(char digit, int duration)
{
  stringstream ss;
  ss << "dtmfDigitReceived " << digit << " " << duration;
  processEvent(ss.str());
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
void ModuleTcl::dtmfCmdReceived(const string& cmd)
{
  stringstream ss;
  ss << "dtmfCmdReceived \"" << cmd << "\"";
  processEvent(ss.str());
} /* dtmfCmdReceived */


/*
 *----------------------------------------------------------------------------
 * Method:    dtmfCmdReceivedWhenIdle
 * Purpose:   This function is called by the logic core when a DTMF command
 *            has been detected on the receiver when the module is idle, that
 *            is it has not been activated. A command is sent to a non-active
 *            module if a command is received that start with the module ID
 *            but have more digits than just the module ID. The digits
 *            following the module ID is the actual command sent to this
 *            function.
 *            A DTMF command is just a sequence of digits. A-D, *, # is
 *            filtered out and has special meanings to the logic core.
 * Input:     cmd - The received command.
 * Output:    None
 * Author:    Tobias Blomberg / SM0SVX
 * Created:   2010-01-10
 * Remarks:   
 * Bugs:      
 *----------------------------------------------------------------------------
 */
void ModuleTcl::dtmfCmdReceivedWhenIdle(const std::string &cmd)
{
  stringstream ss;
  ss << "dtmfCmdReceivedWhenIdle \"" << cmd << "\"";
  processEvent(ss.str());
} /* dtmfCmdReceivedWhenIdle  */


/*
 *----------------------------------------------------------------------------
 * Method:    squelchOpen
 * Purpose:   Called by the core system when the squelch open or close.
 * Input:     is_open - Set to \em true if the squelch is open or \em false
 *    	      	      	if it's not.
 * Output:    None
 * Author:    Tobias Blomberg / SM0SVX
 * Created:   2005-08-28
 * Remarks:   
 * Bugs:      
 *----------------------------------------------------------------------------
 */
void ModuleTcl::squelchOpen(bool is_open)
{
  stringstream ss;
  ss << "squelchOpen " << (is_open ? 1 : 0);
  processEvent(ss.str());
} /* squelchOpen */


/*
 *----------------------------------------------------------------------------
 * Method:    allMsgsWritten
 * Purpose:   Called by the core system when all announcement messages has
 *    	      been played. Note that this function also may be called even
 *    	      if it wasn't this module that initiated the message playing.
 * Input:     None
 * Output:    None
 * Author:    Tobias Blomberg / SM0SVX
 * Created:   2005-08-28
 * Remarks:   
 * Bugs:      
 *----------------------------------------------------------------------------
 */
void ModuleTcl::allMsgsWritten(void)
{
  processEvent("allMsgsWritten");
} /* allMsgsWritten */



/*
 * This file has not been truncated
 */
