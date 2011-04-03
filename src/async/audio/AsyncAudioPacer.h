/**
@file	 AsyncAudioPacer.h
@brief   An audio pipe class for pacing audio output
@author  Tobias Blomberg / SM0SVX
@date	 2007-11-17

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2003-2008 Tobias Blomberg / SM0SVX

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


#ifndef AUDIO_PACER_INCLUDED
#define AUDIO_PACER_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <sys/time.h>
#include <sigc++/sigc++.h>
#include <stdint.h>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncAudioReader.h>
#include <AsyncAudioSource.h>
#include <AsyncTimer.h>


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
@brief	An audio pipe class that pace audio output
@author Tobias Blomberg
@date   2007-11-17

This class is used in an audio pipe chain to pace audio output.
*/
class AudioPacer : public AudioReader, public AudioSource, public SigC::Object
{
  public:
    /**
     * @brief 	Constuctor
     * @param 	sample_rate The sample rate of the incoming samples
     * @param 	block_size  The size of the audio blocks
     */
    AudioPacer(unsigned sample_rate, unsigned block_size);
  
    /**
     * @brief 	Destructor
     */
    ~AudioPacer(void);
  
    /**
     * @brief 	Write samples into this audio sink
     * @param 	samples The buffer containing the samples
     * @param 	count The number of samples in the buffer
     * @return	Returns the number of samples that has been taken care of
     *
     * This function is used to write audio into this audio sink. If it
     * returns 0, no more samples could be written.
     * If the returned number of written samples is lower than the count
     * parameter value, the sink is not ready to accept more samples.
     * In this case, the audio source requires sample buffering to temporarily
     * store samples that are not immediately accepted by the sink.
     * The writeSamples function should be called on source buffer updates
     * and after a source output request has been received through the
     * requestSamples function.
     * This function is normally only called from a connected source object.
     */
    virtual int writeSamples(const float *samples, int count);
    
    /**
     * @brief 	Tell the sink to flush the previously written samples
     *
     * This function is used to tell the sink to flush previously written
     * samples. When done flushing, the sink should call the
     * sourceAllSamplesFlushed function.
     * This function is normally only called from a connected source object.
     */
    virtual void flushSamples(void);    
    
    /**
     * @brief Request audio samples from this source
     * @param count
     * 
     * This function will be called when the registered audio sink is ready
     * to accept more samples.
     * This function is normally only called from a connected sink object.
     */
    virtual void requestSamples(int count);
    

  protected:
    /**
     * @brief The registered sink has flushed all samples
     *
     * This function will be called when all samples have been flushed in the
     * registered sink. If it is not reimplemented, a handler must be set
     * that handle the function call.
     * This function is normally only called from a connected sink object.
     */
    virtual void allSamplesFlushed(void);
    
    
  private:

    typedef enum
    {
      STREAM_IDLE, STREAM_ACTIVE, STREAM_FLUSHING
    } StreamState;
                  
    unsigned  	   sample_rate;
    unsigned       block_size;
    float     	   *buf;
    uint64_t	   output_samples;
    Timer          *pace_timer;
    StreamState    stream_state;
    struct timeval output_start;
    
    void outputNextBlock(Timer *t);
    
};  /* class AudioPacer */


} /* namespace */

#endif /* AUDIO_PACER_INCLUDED */



/*
 * This file has not been truncated
 */

