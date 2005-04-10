/**
@file	 EventHandler.cpp
@brief   A_brief_description_for_this_file
@author  Tobias Blomberg / SM0SVX
@date	 2005-04-09

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

#include <iostream>
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

#include "EventHandler.h"
#include "Logic.h"
#include "Module.h"



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
EventHandler::EventHandler(const string& event_script, Logic *logic)
  : event_script(event_script), logic(logic)
{
  interp = Tcl_CreateInterp();
  Tcl_CreateCommand(interp, "playFile", playFile, this, NULL);
  Tcl_CreateCommand(interp, "playSilence", playSilence, this, NULL);
  //Tcl_CreateCommand(interp, "spellWord", spellWord, this, NULL);
  //Tcl_CreateCommand(interp, "playNumber", playNumber, this, NULL);
  Tcl_CreateCommand(interp, "reportActiveModuleState",
      	  reportActiveModuleState, this, NULL);
} /* EventHandler::EventHandler */


EventHandler::~EventHandler(void)
{
  Tcl_DeleteInterp(interp);
} /* EventHandler::~EventHandler */


bool EventHandler::initialize(void)
{
  if (Tcl_EvalFile(interp, event_script.c_str()) != TCL_OK)
  {
    cerr << event_script << ": " << Tcl_GetStringResult(interp) << endl;
    return false;
  }
  
  return true;
  
} /* EventHandler::initialize */


void EventHandler::setVariable(const string& name, const string& value)
{
  Tcl_SetVar(interp, name.c_str(), value.c_str(), 0);
} /* EventHandler::setVariable */


bool EventHandler::processEvent(const string& event)
{
  if (Tcl_Eval(interp, (event + ";").c_str()) != TCL_OK)
  {
    cerr << "*** ERROR: Unable to handle event: " << event
      	 << " (" << Tcl_GetStringResult(interp) << ")" << endl;
    return false;
  }
  
  return true;
  
} /* EventHandler::processEvent */



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

int EventHandler::playFile(ClientData cdata, Tcl_Interp *irp, int argc,
      	      	      	   const char *argv[])
{
  if(argc != 2)
  {
    Tcl_SetResult(irp,"Usage: playFile: <filename>", TCL_STATIC);
    return TCL_ERROR;
  }
  cout << "EventHandler::playFile: " << argv[1] << endl;

  EventHandler *self = static_cast<EventHandler *>(cdata);
  self->logic->playFile(argv[1]);

  return TCL_OK;
}


int EventHandler::playSilence(ClientData cdata, Tcl_Interp *irp, int argc,
      	      	      	      const char *argv[])
{
  if(argc != 2)
  {
    Tcl_SetResult(irp,"Usage: playSilence <milliseconds>", TCL_STATIC);
    return TCL_ERROR;
  }
  cout << "EventHandler::playSilence: " << argv[1] << endl;

  EventHandler *self = static_cast<EventHandler *>(cdata);
  self->logic->playSilence(atoi(argv[1]));

  return TCL_OK;
}


#if 0
int EventHandler::spellWord(ClientData cdata, Tcl_Interp *irp, int argc,
      	      	      	    const char *argv[])
{
  if(argc != 2)
  {
    Tcl_SetResult(irp,"Usage: spellWord <word>", TCL_STATIC);
    return TCL_ERROR;
  }
  cout << "EventHandler::spellWord: " << argv[1] << endl;

  EventHandler *self = static_cast<EventHandler *>(cdata);
  self->logic->spellWord(argv[1]);
  
  return TCL_OK;
}


int EventHandler::playNumber(ClientData cdata, Tcl_Interp *irp, int argc,
      	      	      	     const char *argv[])
{
  if(argc != 2)
  {
    Tcl_SetResult(irp, "Usage: playNumber <word>", TCL_STATIC);
    return TCL_ERROR;
  }
  cout << "EventHandler::playNumber: " << argv[1] << endl;

  EventHandler *self = static_cast<EventHandler *>(cdata);
  self->logic->playNumber(atof(argv[1]));

  return TCL_OK;
}
#endif


int EventHandler::reportActiveModuleState(ClientData cdata, Tcl_Interp *irp,
      	      	      	      	      	  int argc, const char *argv[])
{
  if(argc != 1)
  {
    Tcl_SetResult(irp, "Usage: reportActiveModuleState", TCL_STATIC);
    return TCL_ERROR;
  }
  cout << "EventHandler::reportActiveModuleState" << endl;

  EventHandler *self = static_cast<EventHandler *>(cdata);
  Module *module = self->logic->activeModule();
  if (module == 0)
  {
    Tcl_SetResult(irp, "reportActiveModuleState: No active module", TCL_STATIC);
    return TCL_ERROR;
  }
  module->reportState();
  
  return TCL_OK;
}





/*
 * This file has not been truncated
 */

