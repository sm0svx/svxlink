/**
@file	 AsyncAudioPassthrough.h
@brief   This file contains a class that just pass the audio through
@author  Tobias Blomberg / SM0SVX
@date	 2006-08-07

\verbatim
Async - A library for programming event driven applications
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


#ifndef AUDIO_PASSTHROUGH_INCLUDED
#define AUDIO_PASSTHROUGH_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncAudioSink.h>
#include <AsyncAudioSource.h>


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

namespace Async
{


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
@brief	This class just let the audio pass through
@author Tobias Blomberg / SM0SVX
@date   2006-08-07

This class is both a source and a sink and just let the audio pass through.
It can be used standalone but maybe is of more use when inheriting from it.
For example if you want to snoop in on an audio streem without affecting it,
just reimplement the writeSamples method and you have access to the samples.
Just remember to call the sinkWriteSamples method to forward the samples.
*/
class AudioPassthrough : public AudioSink, public AudioSource
{
  public:
    /**
     * @brief 	Default constuctor
     */
    AudioPassthrough(void) {}
  
    /**
     * @brief 	Destructor
     */
    virtual ~AudioPassthrough(void) {}
  
    /**
     * @brief 	Write samples into this audio sink
     * @param 	samples The buffer containing the samples
     * @param 	count The number of samples in the buffer
     * @return	Returns the number of samples that has been taken care of
     *
     * This function is used to write audio into this audio sink. If it
     * returns 0, no more samples should be written until the resumeOutput
     * function in the source have been called.
     * This function is normally only called from a connected source object.
     */
    virtual int writeSamples(const float *samples, int count)
    {
      return sinkWriteSamples(samples, count);
    }
    
    /**
     * @brief 	Tell the sink to flush the previously written samples
     *
     * This function is used to tell the sink to flush previously written
     * samples. When done flushing, the sink should call the
     * sourceAllSamplesFlushed function.
     * This function is normally only called from a connected source object.
     */
    virtual void flushSamples(void)
    {
      sinkFlushSamples();
    }
    
    /**
     * @brief Resume audio output to the sink
     * 
     * This function will be called when the registered audio sink is ready
     * to accept more samples.
     * This function is normally only called from a connected sink object.
     */
    virtual void resumeOutput(void)
    {
      sourceResumeOutput();
    }
    
    /**
     * @brief The registered sink has flushed all samples
     *
     * This function will be called when all samples have been flushed in the
     * registered sink.
     * This function is normally only called from a connected sink object.
     */
    virtual void allSamplesFlushed(void)
    {
      sourceAllSamplesFlushed();
    }
    
  protected:
    
  private:
    AudioPassthrough(const AudioPassthrough&);
    AudioPassthrough& operator=(const AudioPassthrough&);
    
}; /* AudioPassthrough */


} /* namespace */

#endif /* AUDIO_PASSTHROUGH_INCLUDED */



/*
 * This file has not been truncated
 */

