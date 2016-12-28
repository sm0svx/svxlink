/**
@file	 AsyncSigCAudioSource.h
@brief   Contains an adapter class to connect to an AudioSink using SigC
@author  Tobias Blomberg / SM0SVX
@date	 2005-04-17

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2003-2013  Tobias Blomberg / SM0SVX

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


#ifndef ASYNC_SIGC_AUDIO_SOURCE_INCLUDED
#define ASYNC_SIGC_AUDIO_SOURCE_INCLUDED


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
@brief	An adapter class to connect to an AudioSink class using SigC
@author Tobias Blomberg
@date   2005-04-17

This is an adapter class that can be used to interact with an AudioSink
class using SigC signals and slots.
*/
class SigCAudioSource : public AudioSource, public sigc::trackable
{
  public:
    /**
     * @brief 	Default constuctor
     */
    SigCAudioSource(void) {}
  
    /**
     * @brief 	Destructor
     */
    ~SigCAudioSource(void) {}
  
    /**
     * @brief Resume audio output to the sink
     * 
     * This function will be called when the registered audio sink is ready
     * to accept more samples. It will then emit the sigResumeOutput signal.
     */
    virtual void resumeOutput(void)
    {
      sigResumeOutput();
    }
    
    /**
     * @brief The registered sink has flushed all samples
     *
     * This function will be called when all samples have been flushed in the
     * registered sink. It will then emit the sigAllSamplesFlushed signal.
     */
    virtual void allSamplesFlushed(void)
    {
      sigAllSamplesFlushed();
    }

    /**
     * @brief   Write samples into this audio sink
     * @param   samples The buffer containing the samples
     * @param   count The number of samples in the buffer
     * @return  Returns the number of samples that has been taken care of
     *
     * This function is used to write audio into this object. If it
     * returns 0, no more samples should be written until the sigResumeOutput
     * signal have been emitted.
     */
    int writeSamples(float *samples, int count)
    {
      return sinkWriteSamples(samples, count);
    }

    /**
     * @brief   Tell the sink to flush the previously written samples
     *
     * This function is used to tell this object to flush previously written
     * samples. When done flushing, the sigAllSamplesFlushed signal will be
     * emitted.
     */
    void flushSamples(void)
    {
      sinkFlushSamples();
    }

    /**
     * @brief A signal that is emitted when more samples can be written
     * 
     * This signal will be emitted when the registered audio sink is ready
     * to accept more samples.
     */
    sigc::signal<void> sigResumeOutput;

    /**
     * @brief Signal that is emitted when the connected sink is done flushing
     *
     * This signal will be emitted when all samples have been flushed in the
     * registered sink.
     */
    sigc::signal<void> sigAllSamplesFlushed;
    
};  /* class SigCAudioSource */


} /* namespace */

#endif /* ASYNC_SIGC_AUDIO_SOURCE_INCLUDED */



/*
 * This file has not been truncated
 */

