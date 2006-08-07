/**
@file	 AsyncAudioValve.h
@brief   This file contains a class that implements a "valve" for audio.
@author  Tobias Blomberg / SM0SVX
@date	 2006-07-08

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2004-2006  Tobias Blomberg / SM0SVX

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



/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AudioSink.h>
#include <AudioSource.h>



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
     * @param 	block_when_closed When \em true, block the incoming audio
     *	      	stream when the valve is closed. When \em false, incoming
     *	      	audio is thrown away when the valve is closed.
     */
    explicit AudioValve(bool block_when_closed = false)
      : block_when_closed(block_when_closed), is_open(true),
      	all_flushed(false), flush_samples(false)
    {
    }
  
    /**
     * @brief 	Destructor
     */
    ~AudioValve(void) {}
    
    /**
     * @brief 	Open or close the valve
     * @param 	do_open If \em true the valve is open or else it's closed
     *
     * This function is used to open or close the audio valve. When the valve
     * is closed, the connected sink is flushed. What is done with the incoming
     * audio when the valve is closed depends on the block_when_closed
     * parameter to the constructor.
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
      	sourceResumeOutput();
      	if (flush_samples && all_flushed)
	{
      	  sourceAllSamplesFlushed();
	}
      	all_flushed = false;
	flush_samples = false;
      }
      else
      {
      	all_flushed = false;
	flush_samples = false;
      	sinkFlushSamples();
      }
    }
  
    /**
     * @brief 	Write samples into the valve
     * @param 	samples The buffer containing the samples
     * @param 	count The number of samples in the buffer
     * @return	Returns the number of samples that has been taken care of
     *
     * This function is used to write audio into the valve. If it
     * returns 0, no more samples should be written until the resumeOutput
     * function in the source have been called.
     * This function is normally only called from a connected source object.
     */
    int writeSamples(const float *samples, int count)
    {
      flush_samples = false;
      if (!is_open)
      {
      	return (block_when_closed ? 0 : count);
      }
      return sinkWriteSamples(samples, count);
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
      	sinkFlushSamples();
      }
      else
      {
      	if (block_when_closed)
	{
	  flush_samples = true;
	}
	else
	{
      	  sourceAllSamplesFlushed();
	}
      }
    }
    
    /**
     * @brief Resume audio output to the sink
     * 
     * This function must be reimplemented by the inheriting class. It
     * will be called when the registered audio sink is ready to accept
     * more samples.
     * This function is normally only called from a connected sink object.
     */
    void resumeOutput(void)
    {
      if (is_open)
      {
      	sourceResumeOutput();
      }
    }
    
    /**
     * @brief The registered sink has flushed all samples
     *
     * This function must be implemented by the inheriting class. This
     * function will be called when all samples have been flushed in
     * the registered sink.
     */
    virtual void allSamplesFlushed(void)
    {
      if (is_open)
      {
      	sourceAllSamplesFlushed();
      }
      else
      {
      	all_flushed = true;
      }
    }
    
    
  protected:
    
  private:
    AudioValve(const AudioValve&);
    AudioValve& operator=(const AudioValve&);
    
    bool block_when_closed;
    bool is_open;
    bool all_flushed;
    bool flush_samples;
    
};  /* class AudioValve */


} /* namespace */

#endif /* ASYNC_AUDIO_VALVE_INCLUDED */



/*
 * This file has not been truncated
 */

