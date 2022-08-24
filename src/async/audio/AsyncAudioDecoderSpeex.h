/**
@file	 AsyncAudioDecoderSpeex.h
@brief   An audio decoder that use the Speex audio codec
@author  Tobias Blomberg / SM0SVX
@date	 2008-10-15

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

#ifndef ASYNC_AUDIO_DECODER_SPEEX_INCLUDED
#define ASYNC_AUDIO_DECODER_SPEEX_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <speex/speex.h>


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
@brief	An audio decoder that use the Speex audio codec
@author Tobias Blomberg / SM0SVX
@date   2008-10-15

This class implements an audio decoder that use the Speex audio codec.
*/
class AudioDecoderSpeex : public AudioDecoder
{
  public:
    /**
     * @brief 	Default constuctor
     */
    AudioDecoderSpeex(void);
  
    /**
     * @brief 	Destructor
     */
    virtual ~AudioDecoderSpeex(void);
  
    /**
     * @brief   Get the name of the codec
     * @returns Return the name of the codec
     */
    virtual const char *name(void) const { return "SPEEX"; }
  
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
    
    /**
     * @brief 	Get the frame size for the decoder
     * @return	Returns the decoder frame size
     */
    int frameSize(void) const { return frame_size; }
  
    /**
     * @brief 	Enable or disable the perceptual enhancer
     * @param 	enable Set to \em true to enable or \em false to disable
     * @return	Returns the new setting
     */
    bool enableEnhancer(bool enable);
  
    /**
     * @brief 	Get the perceptual enhance enable/disable state
     * @return	Returns \em true if the enhancer is enabled or \em false if
     *          it's not
     */
    bool enhancerEnabled(void) const;
    
    /**
     * @brief 	Write encoded samples into the decoder
     * @param 	buf  Buffer containing encoded samples
     * @param 	size The size of the buffer
     */
    virtual void writeEncodedSamples(void *buf, int size);
    

  protected:
    
  private:
    SpeexBits bits;
    void      *dec_state;
    int       frame_size;
    
    AudioDecoderSpeex(const AudioDecoderSpeex&);
    AudioDecoderSpeex& operator=(const AudioDecoderSpeex&);
    
};  /* class AudioDecoderSpeex */


} /* namespace */

#endif /* ASYNC_AUDIO_DECODER_SPEEX_INCLUDED */



/*
 * This file has not been truncated
 */

