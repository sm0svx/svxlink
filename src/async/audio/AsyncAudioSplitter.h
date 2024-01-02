/**
@file	 AsyncAudioSplitter.h
@brief   A class that splits an audio stream into multiple streams
@author  Tobias Blomberg / SM0SVX
@date	 2005-05-05

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2004-2024 Tobias Blomberg / SM0SVX

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


#ifndef ASYNC_AUDIO_SPLITTER_INCLUDED
#define ASYNC_AUDIO_SPLITTER_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <list>
#include <sigc++/sigc++.h>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncTimer.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include <AsyncAudioSink.h>
#include <AsyncAudioSource.h>


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
@brief	A class that splits an audio stream into multiple streams
@author Tobias Blomberg
@date   2005-05-05

This class is part of the audio pipe framework. It is used to split one
incoming audio source into multiple outgoing sources.
*/
class AudioSplitter : public Async::AudioSink, public Async::AudioSource,
                      public sigc::trackable
{
  public:
    /**
     * @brief 	Default constuctor
     */
    AudioSplitter(void);
  
    /**
     * @brief 	Destructor
     */
    ~AudioSplitter(void);
  
    /**
     * @brief 	Add an audio sink to the splitter
     * @param 	sink  	The sink object to add
     * @param 	managed If managed is \em true the attached sink will be
     *                  deleted when the splitter is deleted
     */
    void addSink(AudioSink *sink, bool managed=false);
  
    /**
     * @brief 	Remove an audio sink from the splitter
     * @param 	sink The sink object to remove
     */
    void removeSink(AudioSink *sink);
  
    /**
     * @brief 	Remove all audio sinks from this splitter
     */
    void removeAllSinks(void);
  
    /**
     * @brief 	Enable or disable audio output to the given audio sink
     * @param 	sink  	The audio sink to enable/disable
     * @param 	enable  Set to \em true to enable the sink or \em false to
     *	      	      	disable it
     */
    void enableSink(AudioSink *sink, bool enable);

    /**
     * @brief 	Write samples into this audio sink
     * @param 	samples The buffer containing the samples
     * @param 	len   	The number of samples in the buffer
     * @return	Returns the number of samples that has been taken care of
     *
     * This function is used to write audio into this audio sink. If it
     * returns 0, no more samples should be written until the resumeOutput
     * function in the source have been called.
     * This function is normally only called from a connected source object.
     */
    int writeSamples(const float *samples, int len) override;

    /**
     * @brief 	Tell the sink to flush the previously written samples
     *
     * This function is used to tell the sink to flush previously written
     * samples. When done flushing, the sink should call the
     * sourceAllSamplesFlushed function.
     * This function is normally only called from a connected source object.
     */
    void flushSamples(void) override;

  protected:
    
  private:
    class Branch;
    
    std::list<Branch *> branches;
    float     	      	*buf;
    int       	      	buf_size;
    int       	      	buf_len;
    bool      	      	do_flush;
    bool      	      	input_stopped;
    int       	      	flushed_branches;
    Branch              *main_branch;
    
    void writeFromBuffer(void);
    void flushAllBranches(void);

    friend class Branch;
    void branchResumeOutput(void);
    void branchAllSamplesFlushed(void);
    void cleanupBranches(void);

};  /* class AudioSplitter */


} /* namespace */

#endif /* ASYNC_AUDIO_SPLITTER_INCLUDED */



/*
 * This file has not been truncated
 */
