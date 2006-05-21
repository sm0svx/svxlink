/**
@file	 AudioSplitter.h
@brief   A class that splits an audio stream into multiple streams
@author  Tobias Blomberg / SM0SVX
@date	 2005-05-05

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

/** @example AudioSplitter_demo.cpp
An example of how to use the AudioSplitter class
*/


#ifndef AUDIO_SPLITTER_INCLUDED
#define AUDIO_SPLITTER_INCLUDED


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



/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include <AudioSink.h>


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

namespace Async
{


/****************************************************************************
 *
 * Forward declarations of classes inside of the declared namespace
 *
 ****************************************************************************/

class Branch;
  

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
@brief	A class that splits an audio stream into multiple streams
@author Tobias Blomberg
@date   2005-05-05

This class is part of the audio pipe concept. It is used to split one
incoming audio source into multiple outgoing sources.

\include AudioSplitter_demo.cpp
*/
class AudioSplitter : public Async::AudioSink
{
  public:
    /**
     * @brief 	Default constuctor
     */
    AudioSplitter(void);
  
    /**
     * @brief 	Destructor
     */
    ~AudioSplitter(void);
  
    /**
     * @brief 	A_brief_member_function_description
     * @param 	param1 Description_of_param1
     * @return	Return_value_of_this_member_function
     */
    void addSink(Async::AudioSink *sink, bool managed=false);
    void removeAllSinks(void);
    
    int writeSamples(const float *samples, int len);
    
    void flushSamples(void);
    
    
  protected:
    
  private:
    std::list<Branch *> branches;
    float     	      	*buf;
    int       	      	buf_size;
    int       	      	buf_len;
    bool      	      	do_flush;
    bool      	      	input_stopped;
    int       	      	flushed_branches;
    
    void writeFromBuffer(void);
    void flushAllBranches(void);

    friend class Branch;
    void branchResumeOutput(void);
    void branchAllSamplesFlushed(void);
    
};  /* class AudioSplitter */


} /* namespace */

#endif /* AUDIO_SPLITTER_INCLUDED */



/*
 * This file has not been truncated
 */

