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
#include <cstdlib>
#include <cstring>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncTimer.h>



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
using namespace Async;



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
  Tcl_CreateCommand(interp, "playFile", playFileHandler, this, NULL);
  Tcl_CreateCommand(interp, "playSilence", playSilenceHandler, this, NULL);
  Tcl_CreateCommand(interp, "playTone", playToneHandler, this, NULL);
  Tcl_CreateCommand(interp, "recordStart", recordHandler, this, NULL);
  Tcl_CreateCommand(interp, "recordStop", recordHandler, this, NULL);
  Tcl_CreateCommand(interp, "deactivateModule", deactivateModuleHandler,
                    this, NULL);
  //Tcl_CreateCommand(interp, "spellWord", spellWord, this, NULL);
  //Tcl_CreateCommand(interp, "playNumber", playNumber, this, NULL);
  //Tcl_CreateCommand(interp, "reportActiveModuleState",
  //    	  reportActiveModuleState, this, NULL);
  setVariable("script_path", event_script);
} /* EventHandler::EventHandler */


EventHandler::~EventHandler(void)
{
  Tcl_Preserve(interp);
  if (!Tcl_InterpDeleted(interp))
  {
    Tcl_DeleteInterp(interp);
  }
  Tcl_Release(interp);
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
  Tcl_Preserve(interp);
  if (Tcl_SetVar(interp, name.c_str(), value.c_str(), TCL_LEAVE_ERR_MSG)
  	== NULL)
  {
    cerr << event_script << ": " << Tcl_GetStringResult(interp) << endl;
  }
  Tcl_Release(interp);
} /* EventHandler::setVariable */


bool EventHandler::processEvent(const string& event)
{
  Tcl_Preserve(interp);
  if (Tcl_Eval(interp, (event + ";").c_str()) != TCL_OK)
  {
    cerr << "*** ERROR: Unable to handle event: " << event
      	 << " (" << Tcl_GetStringResult(interp) << ")" << endl;
    return false;
  }
  Tcl_Release(interp);
  
  return true;
  
} /* EventHandler::processEvent */


const string EventHandler::eventResult(void) const
{
  return Tcl_GetStringResult(interp);
} /* EventHandler::eventResult */


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

int EventHandler::playFileHandler(ClientData cdata, Tcl_Interp *irp, int argc,
      	      	      	   const char *argv[])
{
  if(argc != 2)
  {
    char msg[] = "Usage: playFile: <filename>";
    Tcl_SetResult(irp, msg, TCL_STATIC);
    return TCL_ERROR;
  }
  //cout << "EventHandler::playFile: " << argv[1] << endl;

  EventHandler *self = static_cast<EventHandler *>(cdata);
  string filename(argv[1]);
  self->playFile(filename);

  return TCL_OK;
}


int EventHandler::playSilenceHandler(ClientData cdata, Tcl_Interp *irp,
      	      	      	      int argc, const char *argv[])
{
  if(argc != 2)
  {
    char msg[] = "Usage: playSilence <milliseconds>";
    Tcl_SetResult(irp, msg, TCL_STATIC);
    return TCL_ERROR;
  }
  //cout << "EventHandler::playSilence: " << argv[1] << endl;

  EventHandler *self = static_cast<EventHandler *>(cdata);
  self->playSilence(atoi(argv[1]));

  return TCL_OK;
}


int EventHandler::playToneHandler(ClientData cdata, Tcl_Interp *irp,
      	      	      	      int argc, const char *argv[])
{
  if(argc != 4)
  {
    char msg[] = "Usage: playTone <fq> <amp> <milliseconds>";
    Tcl_SetResult(irp, msg, TCL_STATIC);
    return TCL_ERROR;
  }
  //cout << "EventHandler::playTone: " << argv[1] << endl;

  EventHandler *self = static_cast<EventHandler *>(cdata);
  self->playTone(atoi(argv[1]), atoi(argv[2]), atoi(argv[3]));

  return TCL_OK;
}


int EventHandler::recordHandler(ClientData cdata, Tcl_Interp *irp,
      	      	      	      int argc, const char *argv[])
{
  //cout << "recordHandler: " << argv[0] << endl;
  if (strcmp(argv[0], "recordStart") == 0)
  {
    if(argc != 2)
    {
      char msg[] = "Usage: recordStart <filename>";
      Tcl_SetResult(irp, msg, TCL_STATIC);
      return TCL_ERROR;
    }

    EventHandler *self = static_cast<EventHandler *>(cdata);
    self->recordStart(argv[1]);
  }
  else
  {
    if(argc != 1)
    {
      char msg[] = "Usage: recordStop";
      Tcl_SetResult(irp, msg, TCL_STATIC);
      return TCL_ERROR;
    }

    EventHandler *self = static_cast<EventHandler *>(cdata);
    self->recordStop();
  }
  

  return TCL_OK;
}


int EventHandler::deactivateModuleHandler(ClientData cdata, Tcl_Interp *irp,
      	      	      	      int argc, const char *argv[])
{
  if(argc != 1)
  {
    char msg[] = "Usage: deactivateModuleHandler";
    Tcl_SetResult(irp, msg, TCL_STATIC);
    return TCL_ERROR;
  }

  EventHandler *self = static_cast<EventHandler *>(cdata);
  Timer *t = new Timer(0);
  t->expired.connect(slot(*self, &EventHandler::doDeactivateModule));

  return TCL_OK;
}


void EventHandler::doDeactivateModule(Timer *t)
{
  delete t;
  deactivateModule();
} /* EventHandler::doDeactivateModule */



/*
 * This file has not been truncated
 */

