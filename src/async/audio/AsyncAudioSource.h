/**
@file	 AsyncAudioSource.h
@brief   This file contains the base class for an audio source
@author  Tobias Blomberg / SM0SVX
@date	 2005-04-17

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


#ifndef ASYNC_AUDIO_SOURCE_INCLUDED
#define ASYNC_AUDIO_SOURCE_INCLUDED


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

class AudioSink;
  

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
@brief	The base class for an audio source
@author Tobias Blomberg
@date   2005-04-17

This is the base class for an audio source. An audio source is a class that
can produce audio.
*/
class AudioSource
{
  public:
    /**
     * @brief 	Default constuctor
     */
    AudioSource(void)
      : m_sink(0), m_sink_managed(false), m_handler(0),
        m_auto_unreg_source(false), is_flushing(false)
    {
    }
  
    /**
     * @brief 	Destructor
     */
    virtual ~AudioSource(void);
  
    /**
     * @brief 	Register an audio sink to provide samples to
     * @param 	sink  	The audio sink to register
     * @param 	managed If \em true, the registered sink will be destroyed
     *	      	      	when this object is destroyed.
     * @return	Returns \em true on success or else \em false
     */
    bool registerSink(AudioSink *sink, bool managed=false);

    /**
     * @brief 	Unregister the previously registered audio sink
     */
    void unregisterSink(void);
  
    /**
     * @brief 	Check if an audio sink has been registered
     * @return	Returns \em true if there is an audio sink registerd
     */
    bool isRegistered(void) const { return m_sink != 0; }

    /**
     * @brief 	Get the registered audio sink
     * @return	Returns the registered audio sink if any registered or else
     *          returns 0.
     */
    AudioSink *sink(void) const { return m_sink; }
    
    /**
     * @brief 	Check if the sink is managed or not
     * @returns Returns \em true if the sink is managed or \em false if not
     *
     * This function is used to find out if the connected sink is managed
     * or not. A managed sink will automatically be deleted when this
     * source object is deleted.
     */
    bool sinkManaged(void) const { return m_sink_managed; }

    /**
     * @brief The registered sink has flushed all samples
     *
     * This function will be called when all samples have been flushed in
     * the registered sink.
     * This function is normally only called from a connected sink object.
     */
    void handleAllSamplesFlushed(void)
    {
      is_flushing = false;
      allSamplesFlushed();
    }
    
    /**
     * @brief Resume audio output to the sink
     * 
     * This function must be reimplemented by the inheriting class. It
     * will be called when the registered audio sink is ready to accept
     * more samples.
     * This function is normally only called from a connected sink object.
     */
    virtual void resumeOutput(void)
    {
      assert(m_handler != 0);
      m_handler->resumeOutput();
    }
    

  protected:
    /**
     * @brief The registered sink has flushed all samples
     *
     * This function should be implemented by the inheriting class. It
     * will be called when all samples have been flushed in the
     * registered sink. If it is not reimplemented, a handler must be set
     * that handle the function call.
     * This function is normally only called from a connected sink object.
     */
    virtual void allSamplesFlushed(void)
    {
      assert(m_handler != 0);
      m_handler->handleAllSamplesFlushed();
    }
    
    /*
     * @brief 	Write samples to the connected sink
     * @param 	samples The buffer containing the samples to write
     * @param 	len   	The number of samples in the buffer
     * @return	Return the number of samples that was taken care of
     *
     * This function is used by the inheriting class to write samples to
     * the connected sink, if any. If there is no connected sink, the samples
     * will be thrown away. This function will return the number of samples
     * that was taken care of. Samples that was not taken care of should
     * normally be written again to the sink.
     */
    int sinkWriteSamples(const float *samples, int len);
    
    /*
     * @brief 	Tell the sink to flush any buffered samples
     *
     * This function is used by the inheriting class to tell the connected
     * sink to flush its buffered samples. When the sink have flushed all its
     * samples it will call the allSamplesFlushed function in this class.
     * If there is no registered sink the allSamplesFlushed function will be
     * called right away.
     */
    void sinkFlushSamples(void);
    
    /**
     * @brief 	Setup another source to handle the outgoing audio
     * @param 	handler The source to handle the audio
     * @return	Returns \em true on success or else \em false
     *
     * This function will setup another source to handle outgoing audio.
     * This can be used when an internal object should handle the audio
     * for this object.
     */
    bool setHandler(AudioSource *handler);
    
    /*
     * @brief 	Return the handler
     * @return	Returns the handler previously set with setHandler or 0
     *          if none have been set
     */
    AudioSource *handler(void) const { return m_handler; }
    
    /**
     * @brief Clear a handler that was previously setup with setHandler.
     */
    void clearHandler(void);
    
    
  private:
    AudioSink 	*m_sink;
    bool      	m_sink_managed;
    AudioSource *m_handler;
    bool      	m_auto_unreg_source;
    bool      	is_flushing;
    
    bool registerSinkInternal(AudioSink *sink, bool managed, bool reg);
    void unregisterSinkInternal(bool is_being_destroyed);

};  /* class AudioSource */


} /* namespace */

#endif /* ASYNC_AUDIO_SOURCE_INCLUDED */



/*
 * This file has not been truncated
 */

