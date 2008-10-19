/**
@file	 AsyncAudioEncoderGsm.h
@brief   An audio encoder that encodes to GSM
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

#ifndef ASYNC_AUDIO_ENCODER_GSM_INCLUDED
#define ASYNC_AUDIO_ENCODER_GSM_INCLUDED


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

#include <AsyncAudioEncoder.h>


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
@brief	An audio encoder that encodes to GSM
@author Tobias Blomberg / SM0SVX
@date   2008-10-15

This class implements an audio encoder that converts the native sample
format to GSM audio blocks.
*/
class AudioEncoderGsm : public AudioEncoder
{
  public:
    /**
     * @brief 	Default constuctor
     */
    AudioEncoderGsm(void);
  
    /**
     * @brief 	Destructor
     */
    virtual ~AudioEncoderGsm(void);
  
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
    
    
  protected:
    
  private:
    static const int FRAME_SAMPLE_CNT = 160;
    static const int FRAME_COUNT      = 4;
    static const int GSM_BUF_SIZE     = FRAME_COUNT * FRAME_SAMPLE_CNT;
    
    gsm         gsmh;
    gsm_signal  gsm_buf[GSM_BUF_SIZE];
    int         gsm_buf_len;
    
    AudioEncoderGsm(const AudioEncoderGsm&);
    AudioEncoderGsm& operator=(const AudioEncoderGsm&);
    
};  /* class AudioEncoderGsm */


} /* namespace */

#endif /* ASYNC_AUDIO_ENCODER_GSM_INCLUDED */



/*
 * This file has not been truncated
 */

