/**
@file	 AsyncAudioRecorder.h
@brief   Contains a class for recording raw audio to a file
@author  Tobias Blomberg / SM0SVX
@date	 2005-08-29

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

#ifndef ASYNC_AUDIO_RECORDER_INCLUDED
#define ASYNC_AUDIO_RECORDER_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <stdio.h>
#include <sigc++/sigc++.h>

#include <string>

#include <AsyncAudioSink.h>


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
@brief	A class for recording raw audio to a file
@author Tobias Blomberg / SM0SVX
@date   2005-08-29

Use this class to stream audio into a file. The audio is stored in raw format,
only samples no header.
*/
class AudioRecorder : public Async::AudioSink
{
  public:
    /**
     * @brief 	Default constuctor
     */
    explicit AudioRecorder(const std::string& filename);
  
    /**
     * @brief 	Destructor
     */
    ~AudioRecorder(void);
  
    /**
     * @brief 	Initialize the recorder
     * @return	Return \em true if the initialization was successful
     */
    bool initialize(void);
    
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
    virtual int writeSamples(const float *samples, int count);
    
    /**
     * @brief 	Tell the sink to flush the previously written samples
     *
     * This function is used to tell the sink to flush previously written
     * samples. When done flushing, the sink should call the
     * sourceAllSamplesFlushed function.
     * This function is normally only called from a connected source object.
     */
    virtual void flushSamples(void);

    
  private:
    std::string filename;
    FILE      	*file;
    
    AudioRecorder(const AudioRecorder&);
    AudioRecorder& operator=(const AudioRecorder&);
    
};  /* class AudioRecorder */


} /* namespace */

#endif /* ASYNC_AUDIO_RECORDER_INCLUDED */



/*
 * This file has not been truncated
 */

