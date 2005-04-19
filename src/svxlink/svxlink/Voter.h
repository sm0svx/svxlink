/**
@file	 Voter.h
@brief   A_brief_description_for_this_file
@author  Tobias Blomberg / SM0SVX
@date	 2005-02-

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

/** @example Voter_demo.cpp
An example of how to use the Voter class
*/


#ifndef VOTER_INCLUDED
#define VOTER_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <list>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncConfig.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "Rx.h"


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
@date   2005-04-18

A_detailed_class_description

\include Voter_demo.cpp
*/
class Voter : public Rx
{
  public:
    /**
     * @brief 	Default constuctor
     */
    Voter(Async::Config &cfg, const std::string& name);
  
    /**
     * @brief 	Destructor
     */
    ~Voter(void);
  
    /**
     * @brief 	Initialize the receiver object
     * @return 	Return \em true on success, or \em false on failure
     */
    bool initialize(void);
    
    /**
     * @brief 	Mute the receiver
     * @param 	do_mute Set to \em true to mute or \em false to unmute
     */
    void mute(bool do_mute);
    
    /**
     * @brief 	Check the squelch status
     * @return	Return \em true if the squelch is open or else \em false
     */
    bool squelchIsOpen(void) const;
    
    /**
     * @brief 	Call this function to add a tone detector to the RX
     * @param 	fq The tone frequency to detect
     * @param 	bw The bandwidth of the detector
     * @param 	required_duration The required time in milliseconds that
     *	      	the tone must be active for activity to be reported.
     * @return	Return \em true if the Rx is capable of tone detection or
     *	      	\em false if it's not.
     */
    bool addToneDetector(int fq, int bw, int required_duration);
    
    
  protected:
    
  private:
    std::list<Rx *>   rxs;
    Rx	      	      *active_rx;
    bool      	      is_muted;
    
    void satSquelchOpen(bool is_open);

};  /* class Voter */


//} /* namespace */

#endif /* VOTER_INCLUDED */



/*
 * This file has not been truncated
 */

