/**
@file	 EventHandler.h
@brief   A_brief_description_for_this_file
@author  Tobias Blomberg / SM0SVX
@date	 2005-04-09

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

/** @example EventHandler_demo.cpp
An example of how to use the EventHandler class
*/


#ifndef EVENT_HANDLER_INCLUDED
#define EVENT_HANDLER_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <tcl.h>
#include <sigc++/sigc++.h>

#include <string>


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

class Logic;

namespace Async
{
  class Timer;
}


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
@brief	A_brief_class_description
@author Tobias Blomberg
@date   2005-04-09

A_detailed_class_description

\include EventHandler_demo.cpp
*/
class EventHandler : public SigC::Object
{
  public:
    /**
     * @brief 	Default constuctor
     */
    EventHandler(const std::string& event_script, Logic *logic);
  
    /**
     * @brief 	Destructor
     */
    ~EventHandler(void);
  
    /**
     * @brief 	A_brief_member_function_description
     * @param 	param1 Description_of_param1
     * @return	Return_value_of_this_member_function
     */
    bool initialize(void);
    void setVariable(const std::string& name, const std::string& value);
    bool processEvent(const std::string& event);
    
    SigC::Signal1<void, const std::string&> playFile;
    SigC::Signal1<void, int>   	      	    playSilence;
    SigC::Signal3<void, int, int, int>      playTone;
    SigC::Signal1<void, const std::string&> recordStart;
    SigC::Signal0<void>       	      	    recordStop;
    SigC::Signal0<void>       	      	    deactivateModule;
    
    
  protected:
    
  private:
    std::string event_script;
    Logic	*logic;
    Tcl_Interp  *interp;
    
    static int playFileHandler(ClientData cdata, Tcl_Interp *irp,
      	      	    int argc, const char *argv[]);
    static int playSilenceHandler(ClientData cdata, Tcl_Interp *irp,
      	      	    int argc, const char *argv[]);
    static int playToneHandler(ClientData cdata, Tcl_Interp *irp,
      	      	    int argc, const char *argv[]);
    static int recordHandler(ClientData cdata, Tcl_Interp *irp,
      	      	    int argc, const char *argv[]);
    static int deactivateModuleHandler(ClientData cdata, Tcl_Interp *irp,
      	      	    int argc, const char *argv[]);
    
    void doDeactivateModule(Async::Timer *t);

};  /* class EventHandler */


//} /* namespace */

#endif /* EVENT_HANDLER_INCLUDED */



/*
 * This file has not been truncated
 */

