/**
@file	 AsyncAudioProcessor.h
@brief   The base class for an audio processor class
@author  Tobias Blomberg / SM0SVX
@date	 2006-04-23

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


#ifndef ASYNC_AUDIO_PROCESSOR_INCLUDED
#define ASYNC_AUDIO_PROCESSOR_INCLUDED


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

#include <AsyncAudioSource.h>
#include <AsyncAudioSink.h>
#include <AsyncAudioFifo.h>
#include <SigCAudioSource.h>



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
@brief	The base class for an audio processor
@author Tobias Blomberg / SM0SVX
@date   2006-04-23

This class is the base class for an audio processor. An audio processor is
a class that is both an audio sink and source. It receives samples, process
them in some way and send them further down the chain. 
*/
class AudioProcessor : public SigC::Object, public AudioSink, public AudioSource
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
     * @param 	count	The number of samples in the buffer
     * @return	Return the number of samples processed
     */
    int writeSamples(const float *samples, int count);
    
    /**
     * @brief Order a flush of all samples
     */
    void flushSamples(void);


  protected:

    /**
     * @brief Request samples from this source
     */
    void onRequestSamples(int count);

    /**
     * @brief All samples have been flushed by the sink
     */
    void onAllSamplesFlushed(void);

    /**
     * @brief Set the input and output sample rates
     * @param input_rate The input sample rate
     * @param output_rate The output sample rate
     */
    void setInputOutputSampleRate(int input_rate, int output_rate);
    
    /**
     * @brief Process incoming samples and put them into the output buffer
     * @param dest  Destination buffer
     * @param src   Source buffer
     * @param count Number of samples in the source buffer
     * @return Return number of samples written into destination buffer
     *
     * This function should be reimplemented by the inheriting class to
     * do the actual processing of the incoming samples. All samples must
     * be processed, otherwise they are lost and the output buffer will
     * contain garbage.
     */
    virtual int processSamples(float *dest, const float *src, int count) = 0;
    
    
  private:
    AudioFifo       fifo;
    SigCAudioSource sigsrc;
    int       	    input_rate;
    int       	    output_rate;
    
    AudioProcessor(const AudioProcessor&);
    AudioProcessor& operator=(const AudioProcessor&);

};  /* class AudioProcessor */


} /* namespace */

#endif /* ASYNC_AUDIO_PROCESSOR_INCLUDED */



/*
 * This file has not been truncated
 */

