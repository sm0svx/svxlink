/**
@file	 Vox.h
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


#ifndef VOX_INCLUDED
#define VOX_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <sigc++/signal_system.h>


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
@date   2004-02-15

A_detailed_class_description

\include Vox_demo.cpp
*/
class Vox : public SigC::Object
{
  public:
    /**
     * @brief 	Default constuctor
     */
    explicit Vox(int buf_len);
  
    /**
     * @brief 	Destructor
     */
    ~Vox(void);
  
    /**
     * @brief 	A_brief_member_function_description
     * @param 	param1 Description_of_param1
     * @return	Return_value_of_this_member_function
     */
    void setVoxLimit(short limit);
    void setHangtime(int hangtime);
    int audioIn(short *samples, int count);
    bool isUp(void) const { return is_up; }
    
    SigC::Signal1<void, bool> 	      squelchOpen;
    SigC::Signal2<int, short *, int>  audioOut;
    
    
  protected:
    
  private:
    short *buf;
    int   buf_size;
    int   head;
    long  sum;
    long  up_limit;
    long  down_limit;
    int   hangtime;
    int   samples_since_up;
    bool  is_up;
    
};  /* class Vox */


//} /* namespace */

#endif /* VOX_INCLUDED */



/*
 * This file has not been truncated
 */

