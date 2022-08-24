/**
@file	 AsyncAudioDebugger.h
@brief   This file contains a class that can be used for debugging
@author  Tobias Blomberg / SM0SVX
@date	 2007-10-14

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


#ifndef AUDIO_DEBUGGER_INCLUDED
#define AUDIO_DEBUGGER_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <sys/time.h>
#include <iostream>
#include <string>
#include <stdint.h>


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
@brief	This class is used to debug an audio stream
@author Tobias Blomberg / SM0SVX
@date   2007-10-14

This class is used to debug an audio stream. It can be inserted in the flow
and it will print out a debug message for each function call between the sink
and the source.
*/
class AudioDebugger : public AudioSink, public AudioSource
{
  public:
    /**
     * @brief 	Default constuctor
     */
    AudioDebugger(Async::AudioSource *src=0,
                  const std::string& name="AudioDebugger")
      : name(name), sample_count(0)
    {
      gettimeofday(&start_time, 0);
      if (src != 0)
      {
      	Async::AudioSink *sink = src->sink();
      	if (sink != 0)
	{
	  src->unregisterSink();
	  registerSink(sink);
	}
      	registerSource(src);
      }
    }
  
    /**
     * @brief 	Destructor
     */
    virtual ~AudioDebugger(void) {}
  
    /**
     * @brief   Set the name that is displayed before debug messages
     * @param   debug_name The name to set
     */
    void setName(std::string debug_name) { name = debug_name; }

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
      int ret = sinkWriteSamples(samples, count);
      sample_count += ret;

      float max_samp = 0.0f;
      for (int i=0; i<count; ++i)
      {
        if (samples[i] > max_samp)
        {
          max_samp = samples[i];
        }
        if (-samples[i] > max_samp)
        {
          max_samp = -samples[i];
        }
      }

      struct timeval time, diff;
      gettimeofday(&time, 0);

      timersub(&time, &start_time, &diff);
      uint64_t diff_ms = diff.tv_sec * 1000 + diff.tv_usec / 1000;

      std::cout << name << "::writeSamples: count=" << count
                << " ret=" << ret << " sample_rate=";
      if (diff_ms > 0)
      {
        std::cout << sample_count * 1000 / diff_ms;
      }
      else
      {
        std::cout << "inf";
      }
      std::cout << " max=" << max_samp;
      std::cout << std::endl;
      return ret;
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
      std::cout << name << "::flushSamples\n";
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
      std::cout << name << "::resumeOutput\n";
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
      std::cout << name << "::allSamplesFlushed\n";
      sourceAllSamplesFlushed();
    }
    
  protected:
    
  private:
    std::string name;
    struct timeval start_time;
    uint64_t sample_count;
    
    AudioDebugger(const AudioDebugger&);
    AudioDebugger& operator=(const AudioDebugger&);
    
}; /* AudioDebugger */


} /* namespace */

#endif /* AUDIO_DEBUGGER_INCLUDED */



/*
 * This file has not been truncated
 */

