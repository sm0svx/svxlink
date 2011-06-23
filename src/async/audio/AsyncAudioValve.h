/**
@file	 AsyncAudioValve.h
@brief   This file contains a class that implements a "valve" for audio.
@author  Tobias Blomberg / SM0SVX
@date	 2006-07-08

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2004-2008  Tobias Blomberg / SM0SVX

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


#ifndef ASYNC_AUDIO_VALVE_INCLUDED
#define ASYNC_AUDIO_VALVE_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <iostream>


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
@brief	Implements a "valve" for audio
@author Tobias Blomberg / SM0SVX
@date   2006-07-08

This class implements a "valve" for audio. That is, the audio stream can be
turned on or off. It's named "valve" since the whole Async audio concept is
called audio pipe.
*/
class AudioValve : public Async::AudioSink, public Async::AudioSource
{
  public:
    /**
     * @brief 	Default constuctor
     */
    explicit AudioValve(void) : stream_state(STREAM_IDLE), is_open(true)
    {
    }
  
    /**
     * @brief 	Destructor
     */
    ~AudioValve(void)
    {
    }
    
    /**
     * @brief 	Open or close the valve
     * @param 	do_open If \em true the valve is open or else it's closed
     *
     * This function is used to open or close the audio valve. When the valve
     * is closed, the connected sink is flushed.
     */
    void setOpen(bool do_open)
    {
      if (is_open == do_open)
      {
      	return;
      }
      
      is_open = do_open;
      
      if (do_open)
      {
        if (stream_state == STREAM_ACTIVE)
        {
          sinkAvailSamples();
        }
      }
      else
      {
        if (stream_state == STREAM_FLUSHING)
        {
	  stream_state = STREAM_IDLE;
	  sourceAllSamplesFlushed();
        }
        else if (stream_state == STREAM_ACTIVE)
        {
      	  sinkFlushSamples();
        }
      }
    }

    /**
     * @brief 	Check if the valve is open
     * @returns Return \em true if the valve is open or else \em false
     */
    bool isOpen(void) const
    {
      return is_open;
    }
    
    /**
     * @brief 	Write samples into the valve
     * @param 	samples The buffer containing the samples
     * @param 	count The number of samples in the buffer
     * @return	Returns the number of samples that has been taken care of
     *
     * This function is used to write audio into the valve. If it
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
    int writeSamples(const float *samples, int count)
    {
      stream_state = STREAM_ACTIVE;
      return is_open ? sinkWriteSamples(samples, count) : count;
    }

    /**
     * @brief   Tell the sink that there are samples available on request
     *
     * This function is used to tell the sink that there are samples available
     * that can be requested by calling the sourceRequestSamples function.
     * This function is normally only called from a connected source object.
     */    
    void availSamples(void)
    {
      stream_state = STREAM_ACTIVE;
      if (is_open)
      {
        sinkAvailSamples();
      }
    }
    
    /**
     * @brief 	Tell the valve to flush the previously written samples
     *
     * This function is used to tell the valve to flush previously written
     * samples. When done flushing, the valve will call the
     * allSamplesFlushed function in the connected source object.
     * This function is normally only called from a connected source object.
     */
    void flushSamples(void)
    {
      if (is_open)
      {
      	stream_state = STREAM_FLUSHING;
      	sinkFlushSamples();
      }
      else
      {
	stream_state = STREAM_IDLE;
      	sourceAllSamplesFlushed();
      }
    }

    /**
     * @brief Request audio samples from this source
     * @param count Number of samples requested by a connected sink
     *
     * This function will be called when the registered audio sink is
     * ready to accept more samples.
     * This function is normally only called from a connected sink object.
     * If the source object fails to provide the requested sample count,
     * it can conclude an underrun condition and perform a stream reset.
     */
    void requestSamples(int count)
    {
      if (is_open)
      {
      	sourceRequestSamples(count);
      }
    }

    /**
     * @brief The registered sink has flushed all samples
     *
     * This function will be called when all samples have been flushed in
     * the registered sink.
     * This function is normally only called from a connected sink object.
     */
    void allSamplesFlushed(void)
    {
      if (is_open && (stream_state == STREAM_FLUSHING))
      {
        stream_state = STREAM_IDLE;
      	sourceAllSamplesFlushed();
      }
    }
    
  private:

    AudioValve(const AudioValve&);
    AudioValve& operator=(const AudioValve&);

    typedef enum
    {
      STREAM_IDLE, STREAM_ACTIVE, STREAM_FLUSHING
    } StreamState;

    StreamState stream_state;
    bool is_open;
    
};  /* class AudioValve */


} /* namespace */

#endif /* ASYNC_AUDIO_VALVE_INCLUDED */



/*
 * This file has not been truncated
 */

