/**
@file	 AsyncAudioReader.h
@brief   An audio pipe component for on-demand reading samples
@author  Tobias Blomberg / SM0SVX
@date	 2008-02-22

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2003-2008 Tobias Blomberg / SM0SVX

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

#ifndef ASYNC_AUDIO_READER_INCLUDED
#define ASYNC_AUDIO_READER_INCLUDED


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
@brief	An audio pipe component for on demand reading samples
@author Tobias Blomberg / SM0SVX
@date   2008-02-22

This audio pipe component is used when reading samples on demand is preferred
rather than getting them pushed at you.
*/
class AudioReader : public AudioSink
{
  public:
    /**
     * @brief 	Default constuctor
     */
    AudioReader(void);
  
    /**
     * @brief 	Destructor
     */
    ~AudioReader(void);
  
    /**
     * @brief 	Read at most the specified number of samples
     * @param 	samples A buffer to put the read samples into
     * @param 	count The maximum number of samples to read. This value must
     *	      	      not be larger than the size of the "samples" buffer
     *	      	      given in the first argument.
     * @return	Returns the number of samples actually read
     */
    int readSamples(float *samples, int count);
    
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
     * samples. When done flushing, the sourceAllSamplesFlushed function
     * will be called.
     * This function is normally only called from a connected source object.
     */
    virtual void flushSamples(void);
    
  protected:
    
  private:
    AudioReader(const AudioReader&);
    AudioReader& operator=(const AudioReader&);
    
    float *buf;
    int   buf_size;
    bool  input_stopped;
    int   samples_in_buf;
    
};  /* class AudioReader */


} /* namespace */

#endif /* ASYNC_AUDIO_READER_INCLUDED */



/*
 * This file has not been truncated
 */

