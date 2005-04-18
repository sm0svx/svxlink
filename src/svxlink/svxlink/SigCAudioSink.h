/**
@file	 SigCAudioSink.h
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

/** @example SigCAudioSink_demo.cpp
An example of how to use the SigCAudioSink class
*/


#ifndef SIGC_AUDIO_SINK_INCLUDED
#define SIGC_AUDIO_SINK_INCLUDED


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

#include "AudioSink.h"


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
@date   2005-04-17

A_detailed_class_description

\include SigCAudioSink_demo.cpp
*/
class SigCAudioSink : public AudioSink, public SigC::Object
{
  public:
    /**
     * @brief 	Default constuctor
     */
    SigCAudioSink(void) {}
  
    /**
     * @brief 	Destructor
     */
    ~SigCAudioSink(void) {}
  
    /**
     * @brief 	A_brief_member_function_description
     * @param 	param1 Description_of_param1
     * @return	Return_value_of_this_member_function
     */
    virtual int writeSamples(const short *samples, int len)
    {
      return sigWriteSamples(samples, len);
    }
    
    virtual void flushSamples(void)
    {
      sigFlushSamples();
    }
    
    SigC::Signal2<int, const short *, int>  sigWriteSamples;
    SigC::Signal0<void>       	      	    sigFlushSamples;
    
    
  protected:
    
  private:
    
};  /* class SigCAudioSink */


//} /* namespace */

#endif /* SIGC_AUDIO_SINK_INCLUDED */



/*
 * This file has not been truncated
 */

