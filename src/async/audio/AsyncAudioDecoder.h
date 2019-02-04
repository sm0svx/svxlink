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
class AudioDecoder : public AudioSource, virtual public sigc::trackable
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
     * @brief   Release a previously created decoder
     *
     * This function is used instead of 'delete' to deallocate a decoder.
     * Calling it twice is not allowed.
     */
    virtual void release(void) { delete this; }

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
    virtual void writeEncodedSamples(const void *buf, int size) = 0;
    
    /**
     * @brief Call this function when all encoded samples have been received
     */
    virtual void flushEncodedSamples(void) { sinkFlushSamples(); }
    
    /**
     * @brief This signal is emitted when all encoded samples have been flushed
     */
    sigc::signal<void> allEncodedSamplesFlushed;

  protected:
    class DefaultSourceHandler : public Async::AudioSource
    {
      public:
        DefaultSourceHandler(AudioDecoder *dec) : m_dec(dec) {}
        virtual void resumeOutput(void) {}
      protected:
        virtual void allSamplesFlushed(void)
        {
          m_dec->allEncodedSamplesFlushed();
        }
      private:
        AudioDecoder *m_dec;
    };

    /**
     * @brief 	Default constuctor
     */
    AudioDecoder(void) : m_default_source_handler(this)
    {
      if (handler() == 0)
      {
        setHandler(&m_default_source_handler);
      }
    }

    /**
     * @brief 	Destructor
     */
    virtual ~AudioDecoder(void) {}

  private:
    DefaultSourceHandler m_default_source_handler;

    AudioDecoder(const AudioDecoder&);
    AudioDecoder& operator=(const AudioDecoder&);
    
};  /* class AudioDecoder */


} /* namespace */

#endif /* ASYNC_AUDIO_DECODER_INCLUDED */



/*
 * This file has not been truncated
 */

