/**
@file	 AsyncAudioDecoderRaw.h
@brief   A pass through (unencoded) audio "decoder"
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

#ifndef ASYNC_AUDIO_DECODER_RAW_INCLUDED
#define ASYNC_AUDIO_DECODER_RAW_INCLUDED


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
@brief	A pass through (unencoded) audio "decoder"
@author Tobias Blomberg / SM0SVX
@date   2008-10-06

This class implements an audio "decoder" that just passes the native sample
format through.
*/
class AudioDecoderRaw : public AudioDecoder
{
  public:
    /**
     * @brief 	Default constuctor
     */
    AudioDecoderRaw(void) {}
  
    /**
     * @brief 	Destructor
     */
    virtual ~AudioDecoderRaw(void) {}
  
    /**
     * @brief   Get the name of the codec
     * @return  Return the name of the codec
     */
    virtual const char *name(void) const { return "RAW"; }
  
    /**
     * @brief 	Write encoded samples into the decoder
     * @param 	buf  Buffer containing encoded samples
     * @param 	size The size of the buffer
     */
    virtual void writeEncodedSamples(void *buf, int size)
    {
      const float *samples = reinterpret_cast<const float *>(buf);
      int count = size / sizeof(*samples);
      sinkWriteSamples(samples, count);
    }
    

  protected:
    
  private:
    AudioDecoderRaw(const AudioDecoderRaw&);
    AudioDecoderRaw& operator=(const AudioDecoderRaw&);
    
};  /* class AudioDecoderRaw */


} /* namespace */

#endif /* ASYNC_AUDIO_DECODER_RAW_INCLUDED */



/*
 * This file has not been truncated
 */

