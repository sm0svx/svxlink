/**
@file	 AsyncAudioDecoderGsm.h
@brief   An audio decoder for GSM
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

#ifndef ASYNC_AUDIO_DECODER_GSM_INCLUDED
#define ASYNC_AUDIO_DECODER_GSM_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

extern "C" {
#include <gsm.h>
}



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
@brief	An audio decoder GSM
@author Tobias Blomberg / SM0SVX
@date   2008-10-15

This class implements an audio decoder that converts GSM frames into
the native sample format.
*/
class AudioDecoderGsm : public AudioDecoder
{
  public:
    /**
     * @brief 	Default constuctor
     */
    AudioDecoderGsm(void);
  
    /**
     * @brief 	Destructor
     */
    virtual ~AudioDecoderGsm(void);
  
    /**
     * @brief   Get the name of the codec
     * @returns Return the name of the codec
     */
    virtual const char *name(void) const { return "GSM"; }
  
    /**
     * @brief 	A_brief_member_function_description
     * @param 	param1 Description_of_param1
     * @return	Return_value_of_this_member_function
     */
    
    /**
     * @brief 	Write encoded samples into the decoder
     * @param 	buf  Buffer containing encoded samples
     * @param 	size The size of the buffer
     */
    virtual void writeEncodedSamples(void *buf, int size);
    

  protected:
    
  private:
    static const int FRAME_SAMPLE_CNT = 160;
    
    gsm       gsmh;
    gsm_frame frame;
    int       frame_len;
    
    AudioDecoderGsm(const AudioDecoderGsm&);
    AudioDecoderGsm& operator=(const AudioDecoderGsm&);
    
};  /* class AudioDecoderGsm */


} /* namespace */

#endif /* ASYNC_AUDIO_DECODER_GSM_INCLUDED */



/*
 * This file has not been truncated
 */

