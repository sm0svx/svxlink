/**
@file	 AudioProcessor.h
@brief   The base class for an audio processor class
@author  Tobias Blomberg / SM0SVX
@date	 2006-04-23

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


#ifndef AUDIO_PROCESSOR_INCLUDED
#define AUDIO_PROCESSOR_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

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

extern "C" {
#include <fidlib.h>
};

#include "AudioSource.h"
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
@brief	The base class for an audio processor
@author Tobias Blomberg / SM0SVX
@date   2006-04-23

This class is the base class for an audio processor. An audio processor is
a class that is both an audio sink and source. It receives samples, process
them in some way and send them further down the chain. 
*/
class AudioProcessor : public AudioSink, public AudioSource
{
  public:
    /**
     * @brief 	Default constuctor
     */
    AudioProcessor(void);
  
    /**
     * @brief 	Destructor
     */
    virtual ~AudioProcessor(void);
  
    /**
     * @brief 	Write audio to the filter
     * @param 	samples The buffer containing the samples
     * @param 	len   	The number of samples in the buffer
     * @return	Return the number of samples processed
     */
    int writeSamples(const float *samples, int len);
    
    /**
     * @brief Order a flush of all samples
     */
    void flushSamples(void);

    /**
     * @brief Resume output to the sink if previously stopped
     */
    void resumeOutput(void);
    
    /**
     * @brief All samples have been flushed by the sink
     */
    void allSamplesFlushed(void);


  protected:
    virtual void processSamples(float *dest, const float *src, int count) = 0;
    
    
  private:
    float     	buf[1024];
    int       	buf_cnt;
    bool      	do_flush;
    bool      	buf_full;
    
    AudioProcessor(const AudioProcessor&);
    AudioProcessor& operator=(const AudioProcessor&);
    void writeFromBuf(void);

};  /* class AudioProcessor */


//} /* namespace */

#endif /* AUDIO_PROCESSOR_INCLUDED */



/*
 * This file has not been truncated
 */

