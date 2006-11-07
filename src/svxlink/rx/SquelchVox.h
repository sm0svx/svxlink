/**
@file	 SquelchVox.h
@brief   A_brief_description_for_this_file
@author  Tobias Blomberg
@date	 2004-02-15

A_detailed_description_for_this_file

\verbatim
<A brief description of the program or library this file belongs to>
Copyright (C) 2003  Tobias Blomberg / SM0SVX

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

/** @example Vox_demo.cpp
An example of how to use the Vox class
*/


#ifndef SQUELCH_VOX_INCLUDED
#define SQUELCH_VOX_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <sigc++/sigc++.h>


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

#include "Squelch.h"


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
@date   2004-02-15

A_detailed_class_description

\include Vox_demo.cpp
*/
class SquelchVox : public Squelch
{
  public:
    /**
     * @brief 	Default constuctor
     */
    explicit SquelchVox(void);
  
    /**
     * @brief 	Destructor
     */
    ~SquelchVox(void);
    
    /**
     * @brief 	A_brief_member_function_description
     * @param 	param1 Description_of_param1
     * @return	Return_value_of_this_member_function
     */
    bool initialize(Async::Config& cfg, const std::string& rx_name);
  
    /**
     * @brief 	A_brief_member_function_description
     * @param 	param1 Description_of_param1
     * @return	Return_value_of_this_member_function
     */
    void setVoxLimit(short limit);
    
    /**
     * @brief 	Set the vox start delay
     * @param 	delay The delay in milliseconds to set
     *
     * Use this function to set the vox startup delay. The delay is specified
     * in milliseconds. When a value > 0 is specified, the vox will not trigger
     * within this time after Squelch::reset function has been called.
     */
    void setVoxStartDelay(int delay);

    /**
     * @brief 	Reset the squelch detector
     *
     * Reset the squelch so that the detection process starts from
     * the beginning again.
     */
    virtual void reset(void);

    
  protected:
    int processSamples(const float *samples, int count);
    
  private:
    float   *buf;
    int     buf_size;
    int     head;
    double  sum;
    double  up_limit;
    double  down_limit;
    int     start_delay;
    int     start_delay_left;
    
};  /* class SquelchVox */


//} /* namespace */

#endif /* SQUELCH_VOX_INCLUDED */



/*
 * This file has not been truncated
 */

