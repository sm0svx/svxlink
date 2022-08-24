/**
@file	 AsyncAudioEncoderNull.h
@brief   A null audio "encoder" that just encode silence
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

#ifndef ASYNC_AUDIO_ENCODER_NULL_INCLUDED
#define ASYNC_AUDIO_ENCODER_NULL_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <limits>
#include <stdint.h>


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
@brief	A null audio "encoder" that just communicate zero samples
@author Tobias Blomberg / SM0SVX
@date   2014-05-05

This class implements an audio "encoder" that will just produce zero samples.
The only thing transmitted by the encoder is the number of samples in the
block but no real samples are encoded. The NULL codec may be of use when the
real audio is communicated through another path.
*/
class AudioEncoderNull : public AudioEncoder
{
  public:
    /**
     * @brief 	Default constuctor
     */
    AudioEncoderNull(void) {}
  
    /**
     * @brief 	Destructor
     */
    virtual ~AudioEncoderNull(void) {}
  
    /**
     * @brief   Get the name of the codec
     * @return  Return the name of the codec
     */
    virtual const char *name(void) const { return "NULL"; }
  
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
    virtual int writeSamples(const float *samples, int count)
    {
      if (count > std::numeric_limits<uint16_t>::max())
      {
        count = std::numeric_limits<uint16_t>::max();
      }
      else if (count < 0)
      {
        return -1;
      }
      uint8_t buf[2];
      buf[0] = static_cast<uint8_t>(count & 0xff);
      buf[1] = static_cast<uint8_t>(count >> 8);
      writeEncodedSamples(buf, sizeof(buf));
      return count;
    }
    
  protected:
    
  private:
    AudioEncoderNull(const AudioEncoderNull&);
    AudioEncoderNull& operator=(const AudioEncoderNull&);
    
};  /* class AudioEncoderNull */


} /* namespace */

#endif /* ASYNC_AUDIO_ENCODER_NULL_INCLUDED */



/*
 * This file has not been truncated
 */

