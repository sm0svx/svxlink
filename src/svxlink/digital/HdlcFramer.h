/**
@file	 HdlcFramer.h
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

/** @example HdlcFramer_demo.cpp
An example of how to use the HdlcFramer class
*/


#ifndef HDLC_FRAMER_INCLUDED
#define HDLC_FRAMER_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <vector>
#include <stdint.h>
#include <sigc++/sigc++.h>


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

\include HdlcFramer_demo.cpp
*/
class HdlcFramer : public sigc::trackable
{
  public:
    /**
     * @brief 	Default constuctor
     */
    HdlcFramer(void);
  
    /**
     * @brief 	Destructor
     */
    ~HdlcFramer(void);
  
    /**
     * @brief 	A_brief_member_function_description
     * @param 	param1 Description_of_param1
     * @return	Return_value_of_this_member_function
     */
    void setStartFlagCnt(size_t cnt) { start_flag_cnt = cnt; }
    size_t startFlagCnt(void) const { return start_flag_cnt; }
    void sendBytes(const std::vector<uint8_t> &frame);
    
    sigc::signal<void, const std::vector<bool>&> sendBits;

  protected:
    
  private:
    static const size_t DEFAULT_START_FLAG_CNT = 4;

    unsigned  ones;
    bool      prev_was_mark;
    size_t    start_flag_cnt;

    HdlcFramer(const HdlcFramer&);
    HdlcFramer& operator=(const HdlcFramer&);
    void encodeByte(std::vector<bool> &bitbuf, uint8_t data);
    
};  /* class HdlcFramer */


//} /* namespace */

#endif /* HDLC_FRAMER_INCLUDED */



/*
 * This file has not been truncated
 */

