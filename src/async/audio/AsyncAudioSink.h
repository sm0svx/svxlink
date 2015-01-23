/**
@file	 AsyncAudioSink.h
@brief   This file contains the base class for an audio sink
@author  Tobias Blomberg / SM0SVX
@date	 2005-04-17

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2003-2015 Tobias Blomberg / SM0SVX

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


#ifndef ASYNC_AUDIO_SINK_INCLUDED
#define ASYNC_AUDIO_SINK_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/


#include <cassert>


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

namespace Async
{


/****************************************************************************
 *
 * Forward declarations of classes inside of the declared namespace
 *
 ****************************************************************************/

class AudioSource;
  

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
@brief	The base class for an audio sink
@author Tobias Blomberg
@date   2005-04-17

This is the base class for an audio sink. An audio sink is a class that
can consume audio.
*/
class AudioSink
{
  public:
    /**
     * @brief 	Default constuctor
     */
    AudioSink(void) : m_source(0), m_handler(0), m_auto_unreg_sink(false) {}
  
    /**
     * @brief 	Destructor
     */
    virtual ~AudioSink(void);
  
    /**
     * @brief 	Register an audio source to provide samples to this sink
     * @param 	source    The audio source to use
     * @return	Returns \em true on success or else \em false
     */
    bool registerSource(AudioSource *source);
  
    /**
     * @brief 	Unregister the previously registered audio source
     */
    void unregisterSource(void);
  
    /**
     * @brief 	Check if an audio source has been registered
     * @return	Returns \em true if there is an audio source registerd
     */
    bool isRegistered(void) const { return m_source != 0; }
    
    /**
     * @brief 	Get the registered audio source
     * @return	Returns the registered audio source if any registered or else
     *          returns 0.
     */
    AudioSource *source(void) const { return m_source; }
    
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
      assert(m_handler != 0);
      return m_handler->writeSamples(samples, count);
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
      assert(m_handler != 0);
      m_handler->flushSamples();    
    }
    
    
  protected:
    /**
     * @brief 	Tell the source that we are ready to accept more samples
     */
    void sourceResumeOutput(void);
    
    /**
     * @brief 	Tell the source that all samples have been flushed
     *
     * This function is called by the inheriting class to indicate that
     * all samples have been flushed. It may only be called after a
     * flushSamples call has been received and no more samples has been
     * written to the sink.
     */
    void sourceAllSamplesFlushed(void);
    
    /**
     * @brief 	Setup another sink to handle the incoming audio
     * @param 	handler The sink to handle the audio
     * @return	Returns \em true on success or else \em false
     *
     * This function will setup another sink to handle incoming audio.
     * This can be used when an internal object should handle the audio
     * for this object.
     */
    bool setHandler(AudioSink *handler);
    
    /**
     * @brief Clear a handler that was previously setup with setHandler.
     */
    void clearHandler(void);
    
    /*
     * @brief 	Return the handler
     * @return	Returns the handler previously set with setHandler or 0
     *          if none have been set
     */
    AudioSink *handler(void) const { return m_handler; }
    
    
  private:
    AudioSource *m_source;
    AudioSink 	*m_handler;
    bool      	m_auto_unreg_sink;
    
    bool registerSourceInternal(AudioSource *source, bool reg_sink);
    
};  /* class AudioSink */


} /* namespace */

#endif /* ASYNC_AUDIO_SINK_INCLUDED */



/*
 * This file has not been truncated
 */

