/**
@file	 AudioSource.h
@brief   This file contains the base class for an audio source
@author  Tobias Blomberg / SM0SVX
@date	 2005-04-17

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


#ifndef AUDIO_SOURCE_INCLUDED
#define AUDIO_SOURCE_INCLUDED


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
    AudioSource(void) : m_sink(0), m_sink_managed(false) {}
  
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
     * @brief Resume audio output to the sink
     * 
     * This function must be reimplemented by the inheriting class. This
     * function will be called when the registered audio sink is ready to
     * accept more samples.
     */
    virtual void resumeOutput(void) = 0;
    
    /**
     * @brief The registered sink has flushed all samples
     *
     * This function must be implemented by the inheriting class. This
     * function will be called when all samples have been flushed in
     * the registered sink.
     */
    virtual void allSamplesFlushed(void) = 0;

    
  protected:
    int sinkWriteSamples(const float *samples, int len);
    void sinkFlushSamples(void);
    
    
  private:
    AudioSink *m_sink;
    bool      m_sink_managed;
    
    
};  /* class AudioSource */


} /* namespace */

#endif /* AUDIO_SOURCE_INCLUDED */



/*
 * This file has not been truncated
 */

