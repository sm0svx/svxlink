/**
@file	 AsyncAudioDecoderDummy.h
@brief   An audio "decoder" used when audio should just be thrown away
@author  Tobias Blomberg / SM0SVX
@date	 2017-10-28

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

#ifndef ASYNC_AUDIO_DECODER_DUMMY_INCLUDED
#define ASYNC_AUDIO_DECODER_DUMMY_INCLUDED


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
@brief	An audio "decoder" used when audio should just be thrown away
@author Tobias Blomberg / SM0SVX
@date   2017-10-28

This class implements an audio "decoder" that will just throw away incoming
encoded audio. It may be good to use as a placeholder before a real audio
codec has been chosen.
*/
class AudioDecoderDummy : public AudioDecoder
{
  public:
    /**
     * @brief 	Default constuctor
     */
    AudioDecoderDummy(void) {}

    /**
     * @brief 	Destructor
     */
    virtual ~AudioDecoderDummy(void) {}

    /**
     * @brief   Get the name of the codec
     * @returns Return the name of the codec
     */
    virtual const char *name(void) const { return "DUMMY"; }

    /**
     * @brief 	Write encoded samples into the decoder
     * @param 	buf  Buffer containing encoded samples
     * @param 	size The size of the buffer
     *
     * This DUMMY decoder will just throw away incoming encoded samples.
     */
    virtual void writeEncodedSamples(void*, int) {}

    /**
     * @brief Call this function when all encoded samples have been received
     *
     * This DUMMY decoder will just ignore the flush request.
     */
    virtual void flushEncodedSamples(void) {}

  private:
    AudioDecoderDummy(const AudioDecoderDummy&);
    AudioDecoderDummy& operator=(const AudioDecoderDummy&);

};  /* class AudioDecoderDummy */


} /* namespace */

#endif /* ASYNC_AUDIO_DECODER_DUMMY_INCLUDED */


/*
 * This file has not been truncated
 */
