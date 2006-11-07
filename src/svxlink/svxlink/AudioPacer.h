/**
@file	 AudioPacer.h
@brief   A_brief_description_for_this_file
@author  Tobias Blomberg / SM0SVX
@date	 2004-04-03

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

/** @example AudioPacer_demo.cpp
An example of how to use the AudioPacer class
*/


#ifndef AUDIO_PASER_INCLUDED
#define AUDIO_PASER_INCLUDED


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

namespace Async
{
  class Timer;
};


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
@date   2004-04-03

A_detailed_class_description

\include AudioPacer_demo.cpp
*/
class AudioPacer : public SigC::Object
{
  public:
    /**
     * @brief 	Default constuctor
     */
    AudioPacer(int sample_rate, int block_size, int prebuf_time);
  
    /**
     * @brief 	Destructor
     */
    ~AudioPacer(void);
  
    /**
     * @brief 	A_brief_member_function_description
     * @param 	param1 Description_of_param1
     * @return	Return_value_of_this_member_function
     */
    int audioInput(float *samples, int count);
    void flushAllAudio(void);
    
    
    SigC::Signal1<void, bool>       audioInputBufFull;
    SigC::Signal2<int, float*, int> audioOutput;
    SigC::Signal0<void>       	    allAudioFlushed;
    
    
  protected:
    
  private:
    int       	  sample_rate;
    int       	  buf_size;
    int       	  prebuf_time;
    float     	  *buf;
    int       	  buf_pos;
    int       	  prebuf_samples;
    Async::Timer  *pase_timer;
    bool      	  do_flush;
    
    void outputNextBlock(Async::Timer *t);

};  /* class AudioPacer */


//} /* namespace */

#endif /* AUDIO_PASER_INCLUDED */



/*
 * This file has not been truncated
 */

