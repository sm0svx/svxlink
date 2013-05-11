/**
@file	 HdlcDeframer.h
@brief   A_brief_description_for_this_file
@author  Tobias Blomberg / SM0SVX
@date	 2010-

A_detailed_description_for_this_file

\verbatim
<A brief description of the program or library this file belongs to>
Copyright (C) 2003-2010 Tobias Blomberg / SM0SVX

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

/** @example HdlcDeframer_demo.cpp
An example of how to use the HdlcDeframer class
*/


#ifndef HDLC_DEFRAMER_INCLUDED
#define HDLC_DEFRAMER_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <vector>
#include <sigc++/sigc++.h>
#include <stdint.h>


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
@author Tobias Blomberg / SM0SVX
@date   2008-

A_detailed_class_description

\include HdlcDeframer_demo.cpp
*/
class HdlcDeframer : public sigc::trackable
{
  public:
    /**
     * @brief 	Default constuctor
     */
    HdlcDeframer(void);
  
    /**
     * @brief 	Destructor
     */
    ~HdlcDeframer(void);
  
    /**
     * @brief 	A_brief_member_function_description
     * @param 	param1 Description_of_param1
     * @return	Return_value_of_this_member_function
     */
    void bitsReceived(std::vector<bool> &bits);
    
    sigc::signal<void, std::vector<uint8_t>&> frameReceived;

  protected:
    
  private:
    typedef enum {
      STATE_SYNCHRONIZING, STATE_FRAME_START_WAIT, STATE_RECEIVING
    } State;
    State state;
    uint8_t next_byte;
    uint8_t bit_cnt;
    std::vector<uint8_t> frame;
    unsigned ones;

    HdlcDeframer(const HdlcDeframer&);
    HdlcDeframer& operator=(const HdlcDeframer&);
    
};  /* class HdlcDeframer */


//} /* namespace */

#endif /* HDLC_DEFRAMER_INCLUDED */



/*
 * This file has not been truncated
 */

