/**
@file	 EventHandler.h
@brief   Manage the TCL interpreter and call TCL functions for different events.
@author  Tobias Blomberg / SM0SVX
@date	 2005-04-09

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2019 Tobias Blomberg / SM0SVX

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
@brief	Manage the TCL interpreter and call TCL functions for different events.
@author Tobias Blomberg
@date   2005-04-09
*/
class EventHandler : public sigc::trackable
{
  public:
    /**
     * @brief 	Constuctor
     */
    EventHandler(const std::string& event_script, const std::string& logic_name);

    /**
     * @brief 	Destructor
     */
    ~EventHandler(void);
  
    /**
     * @brief 	Load the event handling script
     * @return	Returns \em true on success or else \em false
     */
    bool initialize(void);
  
    /**
     * @brief 	Set a TCL variable
     * @param 	name The name of the variable to set
     * @param 	value The value to set the given variable to
     */
    void setVariable(const std::string& name, const std::string& value);
  
    /**
     * @brief 	Process the given event
     * @param 	event The event must be a valid TCL function call
     * @return	Returns \em true on success or else \em false
     */
    bool processEvent(const std::string& event);
  
    /**
     * @brief 	Return the event result from the last call
     * @return	This is the return value from the called TCL function
     */
    const std::string eventResult(void) const;
    
    /**
     * @brief 	A signal that is emitted when the TCL script want to play
     *	      	back an audio file
     * @param 	filename The name of the file to plat
     */
    sigc::signal<void, const std::string&> playFile;
    
    /**
     * @brief 	A signal that is emitted when the TCL script want to play
     *	      	back silence
     * @param 	duration  The duration of the silence in milliseconds
     */
    sigc::signal<void, int>   	      	    playSilence;

    /**
     * @brief 	A signal that is emitted when the TCL script want to play
     *	      	back a tone
     * @param 	fq    	  The tone frequency to use
     * @param 	amp   	  The tone amplitude to use (0-1000)
     * @param 	duration  The duration of the tone in milliseconds
     */
    sigc::signal<void, int, int, int>      playTone;
    
    /**
     * @brief 	A signal that is emitted when the TCL script want to play
     *	      	back a dtmf digit
     * @param 	digit     The dtmf-tone as character (0-9, A-D, *, #)
     * @param 	amp   	  The tone amplitude to use (0-1000)
     * @param 	duration  The duration of the tone in milliseconds
     */
    sigc::signal<void, const std::string&, int, int> playDtmf;

    /**
     * @brief 	A signal that is emitted when the TCL script want to start
     *	      	a recording
     * @param 	filename The name of the file to record the audio to
     * @param   max_time The maximum recording time in milliseconds
     */
    sigc::signal<void, const std::string&, unsigned> recordStart;
    
    /**
     * @brief 	A signal that is emitted when the TCL script want to stop
     *	      	the current recording
     */
    sigc::signal<void>       	      	    recordStop;
    
    /**
     * @brief 	A signal that is emitted when the TCL script want to deactivate
     *	      	the currently active module
     */
    sigc::signal<void>       	      	    deactivateModule;

    /**
     * @brief   A signal that is emitted when the TCL script want to publish
     *          a state event
     * @param   event_name The name of the event (<context>:<name>)
     * @param   event_msg  The event message
     */
    sigc::signal<void, const std::string&,
                 const std::string&> publishStateEvent;
    
    /**
     * @brief 	A signal that is emitted when the TCL script want to inject
     *	      	DTMF digits in the command queue
     * @param 	digits    The dtmf-digits as character (0-9, A-D, *, #)
     * @param 	duration  The duration of each digit in milliseconds
     */
    sigc::signal<void, const std::string&, int> injectDtmf;
    
  protected:

  private:
    std::string   event_script;
    std::string   logic_name;
    Tcl_Interp *  interp;

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
    static int publishStateEventHandler(ClientData cdata, Tcl_Interp *irp,
      	            int argc, const char *argv[]);
    static int playDtmfHandler(ClientData cdata, Tcl_Interp *irp,
                    int argc, const char *argv[]);
    static int injectDtmfHandler(ClientData cdata, Tcl_Interp *irp,
                    int argc, const char *argv[]);

};  /* class EventHandler */


//} /* namespace */

#endif /* EVENT_HANDLER_INCLUDED */



/*
 * This file has not been truncated
 */

