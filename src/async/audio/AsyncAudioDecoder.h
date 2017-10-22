/**
@file	 AsyncAudioDecoder.h
@brief   Base class of an audio decoder
@author  Tobias Blomberg / SM0SVX
@date	 2008-10-06

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

#ifndef ASYNC_AUDIO_DECODER_INCLUDED
#define ASYNC_AUDIO_DECODER_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <string>
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
@brief	Base class for an audio decoder
@author Tobias Blomberg / SM0SVX
@date   2008-10-06

This is the base class for an audio decoder.
*/
class AudioDecoder : public AudioSource, public sigc::trackable
{
  public:
    /**
     * @brief   Check if a specific decoder is available
     * @param   name The name of the decoder to look for
     */
    static bool isAvailable(const std::string &name);

    /**
     * @brief   Create a new decoder of the specified type
     * @param   name The name of the decoder to create
     */
    static AudioDecoder *create(const std::string &name);
    
    /**
     * @brief 	Default constuctor
     */
    AudioDecoder(void) {}
  
    /**
     * @brief 	Destructor
     */
    virtual ~AudioDecoder(void) {}
    
    /**
     * @brief   Get the name of the codec
     * @returns Return the name of the codec
     */
    virtual const char *name(void) const = 0;
  
    /**
     * @brief 	Set an option for the decoder
     * @param 	name The name of the option
     * @param 	value The value of the option
     */
    virtual void setOption(const std::string &name, const std::string &value) {}

    /**
     * @brief Print codec parameter settings
     */
    virtual void printCodecParams(void) const {}
    
    /**
     * @brief 	Write encoded samples into the decoder
     * @param 	buf  Buffer containing encoded samples
     * @param 	size The size of the buffer
     */
    virtual void writeEncodedSamples(void *buf, int size) = 0;
    
    /**
     * @brief Call this function when all encoded samples have been received
     */
    virtual void flushEncodedSamples(void) { sinkFlushSamples(); }
    
    /**
     * @brief Resume audio output to the sink
     * 
     * This function will be called when the registered audio sink is ready to
     * accept more samples.
     * This function is normally only called from a connected sink object.
     */
    virtual void resumeOutput(void) {}
    
    /**
     * @brief This signal is emitted when all encoded samples have been flushed
     */
    sigc::signal<void> allEncodedSamplesFlushed;
    

  protected:
    /**
     * @brief The registered sink has flushed all samples
     *
     * This function will be called when all samples have been flushed in the
     * registered sink.
     * This function is normally only called from a connected sink object.
     */
    virtual void allSamplesFlushed(void) { allEncodedSamplesFlushed(); }
    
    
  private:
    AudioDecoder(const AudioDecoder&);
    AudioDecoder& operator=(const AudioDecoder&);
    
};  /* class AudioDecoder */


} /* namespace */

#endif /* ASYNC_AUDIO_DECODER_INCLUDED */



/*
 * This file has not been truncated
 */

