/**
@file	 AudioPaser.h
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

/** @example AudioPaser_demo.cpp
An example of how to use the AudioPaser class
*/


#ifndef AUDIO_PASER_INCLUDED
#define AUDIO_PASER_INCLUDED


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
@date   2004-04-03

A_detailed_class_description

\include AudioPaser_demo.cpp
*/
class AudioPaser : public SigC::Object
{
  public:
    /**
     * @brief 	Default constuctor
     */
    AudioPaser(int sample_rate, int block_size, int prebuf_time);
  
    /**
     * @brief 	Destructor
     */
    ~AudioPaser(void);
  
    /**
     * @brief 	A_brief_member_function_description
     * @param 	param1 Description_of_param1
     * @return	Return_value_of_this_member_function
     */
    int audioInput(short *samples, int count);
    void flushAllAudio(void);
    
    
    SigC::Signal1<void, bool>       audioInputBufFull;
    SigC::Signal2<int, short*, int> audioOutput;
    SigC::Signal0<void>       	    allAudioFlushed;
    
    
  protected:
    
  private:
    int       	  sample_rate;
    int       	  buf_size;
    int       	  prebuf_time;
    short     	  *buf;
    int       	  buf_pos;
    int       	  prebuf_samples;
    Async::Timer  *pase_timer;
    bool      	  do_flush;
    
    void outputNextBlock(Async::Timer *t);

};  /* class AudioPaser */


//} /* namespace */

#endif /* AUDIO_PASER_INCLUDED */



/*
 * This file has not been truncated
 */

