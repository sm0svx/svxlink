/**
@file	 AsyncAudioDecoderNull.h
@brief   An audio "decoder" used when no real audio need to be communicated
@author  Tobias Blomberg / SM0SVX
@date	 2014-05-05

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2003-2015 Tobias Blomberg / SM0SVX

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

#ifndef ASYNC_AUDIO_DECODER_NULL_INCLUDED
#define ASYNC_AUDIO_DECODER_NULL_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <cstring>
#include <stdint.h>


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
@brief	An audio "decoder" used when no real audio need to be communicated
@author Tobias Blomberg / SM0SVX
@date   2014-05-05

This class implements an audio "decoder" that will just produce zero samples.
The only thing transmitted by the encoder is the number of samples in the
block but no real samples are encoded. The NULL codec may be of use when the
real audio is communicated through another path.
*/
class AudioDecoderNull : public AudioDecoder
{
  public:
    /**
     * @brief 	Default constuctor
     */
    AudioDecoderNull(void) {}
  
    /**
     * @brief 	Destructor
     */
    virtual ~AudioDecoderNull(void) {}
  
    /**
     * @brief   Get the name of the codec
     * @returns Return the name of the codec
     */
    virtual const char *name(void) const { return "NULL"; }
  
    /**
     * @brief 	Write encoded samples into the decoder
     * @param 	buf  Buffer containing encoded samples
     * @param 	size The size of the buffer
     *
     * This NULL decoder will just write zero samples. The incoming buffer
     * will contain a 16 bit unsigned integer that indicate how many zero
     * samples that should be written to the audio sink.
     */
    virtual void writeEncodedSamples(void *buf, int size)
    {
        // Sanity check
      if (size != sizeof(uint16_t))
      {
        return;
      }

        // Decode incoming buffer
      uint8_t *ptr = reinterpret_cast<uint8_t*>(buf);
      uint16_t cnt = static_cast<uint16_t>(ptr[0]);
      cnt |= static_cast<uint16_t>(ptr[1]) << 8;

        // Allocate a zeroed out buffer and write it to the sink
      float samples[cnt];
      std::memset(samples, 0, cnt * sizeof(*samples));
      sinkWriteSamples(samples, cnt);
    }

  protected:
    
  private:
    AudioDecoderNull(const AudioDecoderNull&);
    AudioDecoderNull& operator=(const AudioDecoderNull&);
    
};  /* class AudioDecoderNull */


} /* namespace */

#endif /* ASYNC_AUDIO_DECODER_NULL_INCLUDED */



/*
 * This file has not been truncated
 */
