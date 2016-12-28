/**
@file	 AsyncAudioStreamStateDetector.h
@brief   This file contains a class that just passes the audio through and
         fires an event when the stream state changes.
@author  Tobias Blomberg / SM0SVX
@date	 2008-05-30

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2004-2009 Tobias Blomberg / SM0SVX

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


#ifndef ASYNC_AUDIO_STREAM_STATE_DETECTOR_INCLUDED
#define ASYNC_AUDIO_STREAM_STATE_DETECTOR_INCLUDED


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

#include <AsyncAudioPassthrough.h>


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
 * @brief A class that just passes the audio through and fires an event when
 *    	  the stream state changes.
 */
class AudioStreamStateDetector : public AudioPassthrough, public sigc::trackable
{
  public:
    /**
     * @brief 	Default constuctor
     */
    AudioStreamStateDetector(void) : stream_state(STREAM_IDLE) {}
  
    /**
     * @brief 	Destructor
     */
    virtual ~AudioStreamStateDetector(void) {}
  
    /**
     * @brief 	Write samples into this audio sink
     * @param 	samples The buffer containing the samples
     * @param 	count 	The number of samples in the buffer
     * @return	Returns the number of samples that has been taken care of
     *
     * This function is used to write audio into this audio sink. If it
     * returns 0, no more samples should be written until the resumeOutput
     * function in the source have been called.
     * This function is normally only called from a connected source object.
     */
    virtual int writeSamples(const float *samples, int count)
    {
      if (stream_state != STREAM_ACTIVE)
      {
        stream_state = STREAM_ACTIVE;
        sigStreamStateChanged(true, false);
      }
      return AudioPassthrough::writeSamples(samples, count);
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
      if (stream_state != STREAM_FLUSHING)
      {
        stream_state = STREAM_FLUSHING;
        sigStreamStateChanged(false, false);
      }
      AudioPassthrough::flushSamples();
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
      if (stream_state != STREAM_IDLE)
      {
        stream_state = STREAM_IDLE;
        sigStreamStateChanged(false, true);
      }
      AudioPassthrough::allSamplesFlushed();
    }

    /**
     * @brief 	Check if the steam is idle or not
     * @returns Returns \em true if the stream is idle or \em false if it's not
     */
    bool isIdle(void)     const { return (stream_state == STREAM_IDLE); }

    /**
     * @brief 	Check if the steam is active or not
     * @returns Returns \em true if the stream is active or \em false if it's
     *        	not
     */
    bool isActive(void)   const { return (stream_state == STREAM_ACTIVE); }

    /**
     * @brief 	Check if the steam is flushing or not
     * @returns Returns \em true if the stream is flushing or \em false if
     *	      	it's not
     */
    bool isFlushing(void) const { return (stream_state == STREAM_FLUSHING); }
    
    /**
     * @brief A signal that is emitted when the stream state changes
     * @param is_active Is \em true if the stream is active
     * @param is_idle 	Is \em  true if the stream is idle
     */
    sigc::signal<void, bool, bool> sigStreamStateChanged;
    
    
  private:
    AudioStreamStateDetector(const AudioStreamStateDetector&);
    AudioStreamStateDetector& operator=(const AudioStreamStateDetector&);

    typedef enum
    {
      STREAM_IDLE, STREAM_ACTIVE, STREAM_FLUSHING
    } StreamState;

    StreamState stream_state;
    
}; /* AudioStreamStateDetector */


} /* namespace */

#endif /* ASYNC_AUDIO_STREAM_STATE_DETECTOR_INCLUDED */



/*
 * This file has not been truncated
 */

