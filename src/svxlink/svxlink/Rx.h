/**
@file	 Rx.h
@brief   The base class for a receiver
@author  Tobias Blomberg
@date	 2004-03-21

A_detailed_description_for_this_file

\verbatim
<A brief description of the program or library this file belongs to>
Copyright (C) 2004  Tobias Blomberg / SM0SVX

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

/** @example Template_demo.cpp
An example of how to use the Template class
*/


#ifndef RX_INCLUDED
#define RX_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <sigc++/signal_system.h>

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
@brief	A_brief_class_description
@author Tobias Blomberg
@date   2003-04-

A_detailed_class_description

\include Template_demo.cpp
*/
class Rx : public SigC::Object
{
  public:
    /**
     * @brief 	Default constuctor
     */
    explicit Rx(const std::string& name) {}
  
    /**
     * @brief 	Destructor
     */
    virtual ~Rx(void) {}
  
    /**
     * @brief 	Initialize the receiver object
     * @return 	Return \em true on success, or \em false on failure
     */
    virtual bool initialize(void) = 0;
    
    /**
     * @brief 	Mute the receiver
     * @param 	do_mute Set to \em true to mute or \em false to unmute
     */
    virtual void mute(bool do_mute) = 0;
    
    /**
     * @brief 	Check the squelch status
     * @return	Return \em true if the squelch is open or else \em false
     */
    virtual bool squelchIsOpen(void) const = 0;
    
    /**
     * @brief 	Call this function to enable/disable the detection of 1750Hz
     *	      	tone call.
     * @param 	required_duration The required time in milliseconds that
     *	      	the tone must be active for activity to be reported.
     * @return	Return \em true if the Rx is capable of detecting 1750 or
     *	      	\em false if it's not.
     */
    virtual bool detect1750(int required_duration) { return false; }
    
    /**
     * @brief 	A signal that indicates if the squelch is open or not
     * @param 	is_open \em True if the squelch is open or \em false if not
     */
    SigC::Signal1<void, bool> squelchOpen;
    
    /**
     * @brief 	A signal that is emitted when an audio block has been received
     * @param 	samples The buffer where the samples are stored
     * @param 	count 	The number of samples in the buffer
     * @return	The number of samples that the associated slot took care of
     */
    SigC::Signal2<int, short *, int> audioReceived;
    
    /**
     * @brief 	A signal that is emitted when a DTMF digit has been detected
     * @param 	digit The detected digit (0-9, A-D, *, #)
     */
    SigC::Signal1<void, char> dtmfDigitDetected;
    
    /**
     * @brief 	A signal that is emitted when a 1750Hz tone call has been
     *	      	detected
     */
    SigC::Signal0<void> detected1750;
    
    
  protected:
    
  private:
    
};  /* class Rx */


//} /* namespace */

#endif /* RX_INCLUDED */



/*
 * This file has not been truncated
 */

