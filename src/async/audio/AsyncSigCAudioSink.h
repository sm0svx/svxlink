/**
@file	 AsyncSigCAudioSink.h
@brief   Contains an adapter class to connect to an AudioSource using SigC
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


#ifndef ASYNC_SIGC_AUDIO_SINK_INCLUDED
#define ASYNC_SIGC_AUDIO_SINK_INCLUDED


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

#include <AsyncAudioSink.h>


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
@brief	An adapter class to connect to an AudioSource class using SigC
@author Tobias Blomberg
@date   2005-04-17

This is an adapter class that can be used to interact with an AudioSource
class using SigC signals and slots.
*/
class SigCAudioSink : public AudioSink, public sigc::trackable
{
  public:
    /**
     * @brief 	Default constuctor
     */
    SigCAudioSink(void) {}
  
    /**
     * @brief 	Destructor
     */
    ~SigCAudioSink(void) {}
  
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
     * When called by the source, the sigWriteSamples function will be
     * emitted.
     */
    virtual int writeSamples(const float *samples, int count)
    {
      return sigWriteSamples(const_cast<float *>(samples), count);
    }
    
    /**
     * @brief 	Tell the sink to flush the previously written samples
     *
     * This function is used to tell the sink to flush previously written
     * samples. When done flushing, the sink should call the
     * sourceAllSamplesFlushed function.
     * This function is normally only called from a connected source object.
     * When called by the conected source, the sigFlushSamples signal will
     * be emitted.
     */
    virtual void flushSamples(void)
    {
      sigFlushSamples();
    }
    
    /**
     * @brief 	Tell the source that we are ready to accept more samples
     *
     * This function should be called by the object user to indicate to the
     * connected source that we are now ready to accept more samples.
     */
    void resumeOutput(void)
    {
      sourceResumeOutput();
    }
    
    /**
     * @brief 	Tell the source that all samples have been flushed
     *
     * This function is called by the object user to indicate that
     * all samples have been flushed. It may only be called after a
     * flushSamples call has been received and no more samples has been
     * written to the sink.
     */
    void allSamplesFlushed(void)
    {
      sourceAllSamplesFlushed();
    }
    
    /**
     * @brief 	Signal that is emitted when the source write samples
     * @param 	samples The buffer containing the samples
     * @param 	count The number of samples in the buffer
     * @return	Returns the number of samples that has been taken care of
     *
     * This signal is emitted when the connected source object write samples.
     * If 0 is returned, the source should write no more samples until the
     * resumeOutput function in the source have been called.
     */
    sigc::signal<int, float *, int> sigWriteSamples;

    /**
     * @brief 	Signal emitted when the source are finished writing samples
     *
     * This signal is emitted when the connected source object have finished
     * writing samples. When done flushing, the allSamplesFlushed function
     * should be called.
     */
    sigc::signal<void>       	    sigFlushSamples;
    
};  /* class SigCAudioSink */


} /* namespace */

#endif /* ASYNC_SIGC_AUDIO_SINK_INCLUDED */



/*
 * This file has not been truncated
 */
