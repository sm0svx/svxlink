/**
@file	 EventHandler.cpp
@brief   Manage the TCL interpreter and call TCL functions for different events.
@author  Tobias Blomberg / SM0SVX
@date	 2005-04-09

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2022 Tobias Blomberg / SM0SVX

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

#include <AsyncApplication.h>



/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "EventHandler.h"
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


EventHandler::EventHandler(const string& event_script, const string& logic_name)
  : event_script(event_script), logic_name(logic_name), interp(0)
{
  interp = Tcl_CreateInterp();
  if (interp == 0)
  {
    cerr << "*** ERROR: Could not create TCL interpreter for logic "
         << logic_name << "\n";
    return;
  }
  
  if (Tcl_Init(interp) != TCL_OK)
  {
    cerr << event_script << " in logic " << logic_name
         << " faild to initialize the TCL interpreter: "
         << Tcl_GetStringResult(interp) << endl;
    Tcl_DeleteInterp(interp);
    interp = 0;
    return;
  }
  
  Tcl_CreateCommand(interp, "playFile", playFileHandler, this, NULL);
  Tcl_CreateCommand(interp, "playSilence", playSilenceHandler, this, NULL);
  Tcl_CreateCommand(interp, "playTone", playToneHandler, this, NULL);
  Tcl_CreateCommand(interp, "recordStart", recordHandler, this, NULL);
  Tcl_CreateCommand(interp, "recordStop", recordHandler, this, NULL);
  Tcl_CreateCommand(interp, "deactivateModule", deactivateModuleHandler,
                    this, NULL);
  Tcl_CreateCommand(interp, "publishStateEvent", publishStateEventHandler,
                    this, NULL);
  Tcl_CreateCommand(interp, "playDtmf", playDtmfHandler, this, NULL);
  Tcl_CreateCommand(interp, "injectDtmf", injectDtmfHandler, this, NULL);
  Tcl_CreateCommand(interp, "getConfigValue", getConfigValueHandler,
                    this, NULL);
  Tcl_CreateCommand(interp, "setConfigValue", setConfigValueHandler,
                    this, NULL);

  //setVariable("script_path", event_script);

} /* EventHandler::EventHandler */


EventHandler::~EventHandler(void)
{
  if (interp != 0)
  {
    Tcl_Preserve(interp);
    if (!Tcl_InterpDeleted(interp))
    {
      Tcl_DeleteInterp(interp);
    }
    Tcl_Release(interp);
  }
} /* EventHandler::~EventHandler */


bool EventHandler::initialize(void)
{
  if (interp == 0)
  {
    return false;
  }
  
  if (Tcl_EvalFile(interp, event_script.c_str()) != TCL_OK)
  {
    const char *trace = Tcl_GetVar(interp, "errorInfo", TCL_GLOBAL_ONLY); 
    cerr << "*** ERROR[" << logic_name << "]: Failed to load event script\n"
         << trace << endl;
    return false;
  }
  
  return true;
  
} /* EventHandler::initialize */


void EventHandler::addCommand(const std::string& name, CommandHandler f)
{
  Tcl_CreateCommand(interp, name.c_str(), genericCommandHandler,
      new CommandHandler(f),
      [](ClientData cdata) {
        delete static_cast<CommandHandler*>(cdata);
      });
} /* EventHandler::addCommand */


void EventHandler::setVariable(const string& name, const string& value)
{
  if (interp == 0)
  {
    return;
  }
  
  Tcl_Preserve(interp);
  if (Tcl_SetVar(interp, name.c_str(), value.c_str(), TCL_LEAVE_ERR_MSG)
  	== NULL)
  {
    cerr << event_script << " in logic " << logic_name
         << " failed setting variable \"" << name << "=" << value << "\": "
         << Tcl_GetStringResult(interp) << endl;
  }
  Tcl_Release(interp);
} /* EventHandler::setVariable */


bool EventHandler::processEvent(const string& event)
{
  if (interp == 0)
  {
    return false;
  }
  
  bool success = true;
  Tcl_Preserve(interp);
  if (Tcl_Eval(interp, (event + ";").c_str()) != TCL_OK)
  {
    const char *trace = Tcl_GetVar(interp, "errorInfo", TCL_GLOBAL_ONLY); 
    std::cerr << "*** ERROR[" << logic_name << "]: Unable to handle event "
              << "\"" << event << "\"\n" << trace << std::endl;
    success = false;
  }
  Tcl_Release(interp);
  
  return success;
  
} /* EventHandler::processEvent */


const string EventHandler::eventResult(void) const
{
  if (interp == 0)
  {
    return 0;
  }
  
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

int EventHandler::playFileHandler(ClientData cdata, Tcl_Interp *irp, int argc,
      	      	      	   const char *argv[])
{
  if(argc != 2)
  {
    static char msg[] = "Usage: playFile: <filename>";
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
    static char msg[] = "Usage: playSilence <milliseconds>";
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
    static char msg[] = "Usage: playTone <fq> <amp> <milliseconds>";
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
    if((argc < 2) || (argc > 3))
    {
      static char msg[] = "Usage: recordStart <filename> [max_time]";
      Tcl_SetResult(irp, msg, TCL_STATIC);
      return TCL_ERROR;
    }

    EventHandler *self = static_cast<EventHandler *>(cdata);
    unsigned max_time = 0;
    if (argc == 3)
    {
      max_time = atoi(argv[2]);
    }
    self->recordStart(argv[1], max_time);
  }
  else
  {
    if(argc != 1)
    {
      static char msg[] = "Usage: recordStop";
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
    static char msg[] = "Usage: deactivateModule";
    Tcl_SetResult(irp, msg, TCL_STATIC);
    return TCL_ERROR;
  }

  EventHandler *self = static_cast<EventHandler *>(cdata);
  Application::app().runTask(self->deactivateModule.make_slot());

  return TCL_OK;
}


int EventHandler::publishStateEventHandler(ClientData cdata, Tcl_Interp *irp,
      	      	      	      int argc, const char *argv[])
{
  if (argc != 3)
  {
    static char msg[] = "Usage: publishStateEvent <event name> <event msg>";
    Tcl_SetResult(irp, msg, TCL_STATIC);
    return TCL_ERROR;
  }

  EventHandler *self = static_cast<EventHandler *>(cdata);
  self->publishStateEvent(argv[1], argv[2]);

  return TCL_OK;
}


int EventHandler::playDtmfHandler(ClientData cdata, Tcl_Interp *irp,
                                  int argc, const char *argv[])
{
  if(argc != 4)
  {
    static char msg[] = "Usage: playDtmf <digits> <amp> <milliseconds>";
    Tcl_SetResult(irp, msg, TCL_STATIC);
    return TCL_ERROR;
  }
  //cout << "EventHandler::playDtmf: " << argv[1] << ", "
  //    << argv[2] << ", " << argv[3]<< endl;
  EventHandler *self = static_cast<EventHandler *>(cdata);
  self->playDtmf(argv[1], atoi(argv[2]), atoi(argv[3]));

  return TCL_OK;
} /* EventHandler::playDtmfHandler */


int EventHandler::injectDtmfHandler(ClientData cdata, Tcl_Interp *irp,
                                    int argc, const char *argv[])
{
  if((argc < 2) or (argc > 3))
  {
    static char msg[] = "Usage: injectDtmf <digits> [milliseconds]";
    Tcl_SetResult(irp, msg, TCL_STATIC);
    return TCL_ERROR;
  }
  string digits(argv[1]);
  int duration = 100;
  if (argc == 3)
  {
    duration = atoi(argv[2]);
  }
  //cout << "EventHandler::injectDtmf: " << digits << ", " << duration << endl;
  EventHandler *self = static_cast<EventHandler *>(cdata);
  self->injectDtmf(digits, duration);

  return TCL_OK;
} /* EventHandler::injectDtmfHandler */


int EventHandler::getConfigValueHandler(ClientData cdata, Tcl_Interp *irp,
                                        int argc, const char *argv[])
{
  if((argc < 3) || (argc > 4))
  {
    static char msg[] = "Usage: getConfigValue <section> <tag> [default]";
    Tcl_SetResult(irp, msg, TCL_STATIC);
    return TCL_ERROR;
  }
  std::string section(argv[1]);
  std::string tag(argv[2]);
  std::string value;
  if (argc > 3)
  {
    value = argv[3];
  }
  EventHandler *self = static_cast<EventHandler*>(cdata);
  if (!self->getConfigValue(section, tag, value))
  {
    static char msg[] = "getConfigValue: Failed to read configuration variable";
    Tcl_SetResult(irp, msg, TCL_STATIC);
    return TCL_ERROR;
  }
  //std::cout << "### EventHandler::getConfigValueHandler: " << section << "/"
  //          << tag << "=" << value << std::endl;
  char* cvalue = Tcl_Alloc(value.size()+1);
  strcpy(cvalue, value.c_str());
  Tcl_SetResult(irp, cvalue, TCL_DYNAMIC);

  return TCL_OK;
} /* EventHandler::getConfigValueHandler */


int EventHandler::setConfigValueHandler(ClientData cdata, Tcl_Interp *irp,
                                        int argc, const char *argv[])
{
  if(argc != 4)
  {
    static char msg[] = "Usage: setConfigValue <section> <tag> <value>";
    Tcl_SetResult(irp, msg, TCL_STATIC);
    return TCL_ERROR;
  }
  string section(argv[1]);
  string tag(argv[2]);
  string value(argv[3]);
  //std::cout << "### EventHandler::setConfigValueHandler: " << section << "/"
  //          << tag << "=" << value << std::endl;
  EventHandler *self = static_cast<EventHandler *>(cdata);
  self->setConfigValue(section, tag, value);

  return TCL_OK;
} /* EventHandler::setConfigValueHandler */


int EventHandler::genericCommandHandler(ClientData cdata, Tcl_Interp *irp,
                                        int argc, const char *argv[])
{
  const auto& func = *static_cast<CommandHandler*>(cdata);
  std::string msg = func(argc, argv);
  if (!msg.empty())
  {
    auto msg_alloc_len = msg.size()+1;
    char* msg_copy = static_cast<char*>(Tcl_Alloc(msg_alloc_len));
    memcpy(msg_copy, msg.c_str(), msg_alloc_len);
    Tcl_SetResult(irp, msg_copy, TCL_DYNAMIC);
    return TCL_ERROR;
  }
  return TCL_OK;
} /* EventHandler::genericCommandHandler */


/*
 * This file has not been truncated
 */

