/**
@file	 AsyncAudioEncoder.h
@brief   Base class for an audio decoder
@author  Tobias Blomberg / SM0SVX
@date	 2008-10-06

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2003-2017 Tobias Blomberg / SM0SVX

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

#ifndef ASYNC_AUDIO_ENCODER_INCLUDED
#define ASYNC_AUDIO_ENCODER_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <sigc++/sigc++.h>
#include <string>


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
@brief	Base class for an audio encoder
@author Tobias Blomberg / SM0SVX
@date   2008-10-06

This is the base class for implementing an audio encoder.
*/
class AudioEncoder : public AudioSink, public sigc::trackable
{
  public:
    /**
     * @brief   Check if a specific encoder is available
     * @param   name The name of the encoder to look for
     */
    static bool isAvailable(const std::string &name);

    /**
     * @brief   Create a new encoder of the specified type
     * @param   name The name of the encoder to create
     */
    static AudioEncoder *create(const std::string &name);
    
    /**
     * @brief 	Default constuctor
     */
    AudioEncoder(void) {}
  
    /**
     * @brief 	Destructor
     */
    ~AudioEncoder(void) {}
  
    /**
     * @brief   Get the name of the codec
     * @returns Return the name of the codec
     */
    virtual const char *name(void) const = 0;
    
    /**
     * @brief 	Set an option for the encoder
     * @param 	name The name of the option
     * @param 	value The value of the option
     */
    virtual void setOption(const std::string &name, const std::string &value) {}

    /**
     * @brief Print codec parameter settings
     */
    virtual void printCodecParams(void) {}
    
    /**
     * @brief 	Call this function when all encoded samples have been flushed
     */
    void allEncodedSamplesFlushed(void) { sourceAllSamplesFlushed(); }
    
    /**
     * @brief 	Tell the sink to flush the previously written samples
     *
     * This function is used to tell the sink to flush previously written
     * samples. When done flushing, the sink should call the
     * sourceAllSamplesFlushed function.
     * This function is normally only called from a connected source object.
     */
    virtual void flushSamples(void) { flushEncodedSamples(); }
    
    /**
     * @brief 	A signal emitted when encoded samples are available
     * @param 	buf  Buffer containing encoded samples
     * @param 	size The size of the buffer
     */
    sigc::signal<void,const void *,int> writeEncodedSamples;
    
    /**
     * @brief This signal is emitted when the source calls flushSamples
     */
    sigc::signal<void> flushEncodedSamples;
    
  
  protected:
    
  private:
    AudioEncoder(const AudioEncoder&);
    AudioEncoder& operator=(const AudioEncoder&);
    
};  /* class AudioEncoder */


} /* namespace */

#endif /* ASYNC_AUDIO_ENCODER_INCLUDED */



/*
 * This file has not been truncated
 */

