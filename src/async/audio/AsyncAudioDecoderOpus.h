/**
@file	 AsyncAudioDecoderOpus.h
@brief   An audio decoder that use the Opus audio codec
@author  Tobias Blomberg / SM0SVX
@date	 2013-10-12

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2003-2013 Tobias Blomberg / SM0SVX

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

#ifndef ASYNC_AUDIO_DECODER_OPUS_INCLUDED
#define ASYNC_AUDIO_DECODER_OPUS_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <opus.h>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncAudioDecoder.h>


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
@brief	An audio decoder that use the Opus audio codec
@author Tobias Blomberg / SM0SVX
@date   2013-10-12

This class implements an audio decoder that use the Opus audio codec.
*/
class AudioDecoderOpus : public AudioDecoder
{
  public:
    /**
     * @brief 	Default constuctor
     */
    AudioDecoderOpus(void);
  
    /**
     * @brief 	Destructor
     */
    virtual ~AudioDecoderOpus(void);
  
    /**
     * @brief   Get the name of the codec
     * @returns Return the name of the codec
     */
    virtual const char *name(void) const { return "OPUS"; }
  
    /**
     * @brief 	Set an option for the decoder
     * @param 	name The name of the option
     * @param 	value The value of the option
     */
    virtual void setOption(const std::string &name, const std::string &value);

    /**
     * @brief Print codec parameter settings
     */
    virtual void printCodecParams(void) const;
    
#if OPUS_MAJOR
    /**
     * @brief   Configures decoder gain adjustment
     * @param   new_gain The new gain to set [dB]
     * @returns Retunrs the newly set gain
     */
    float setGain(float new_gain);

    /**
     * @brief   Get the currently set gain
     * @returns Returns the currently set gain in dB
     */
    float gain(void) const;
#endif

    /**
     * @brief   Resets encoder to be equivalent to a freshly initialized one
     */
    void reset(void);

    /**
     * @brief 	Write encoded samples into the decoder
     * @param 	buf  Buffer containing encoded samples
     * @param 	size The size of the buffer
     */
    virtual void writeEncodedSamples(void *buf, int size);
    

  protected:
    
  private:
    OpusDecoder *dec;
    int         frame_size;
    
    AudioDecoderOpus(const AudioDecoderOpus&);
    AudioDecoderOpus& operator=(const AudioDecoderOpus&);
    
};  /* class AudioDecoderOpus */


} /* namespace */

#endif /* ASYNC_AUDIO_DECODER_OPUS_INCLUDED */



/*
 * This file has not been truncated
 */

