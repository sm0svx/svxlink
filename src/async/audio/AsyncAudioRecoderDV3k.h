/**
@file	 AsyncAudioRecoderDV3k.h
@brief   Contains an audio pipe class for amplification/attenuation
@author  Tobias Blomberg / SM0SVX & Adi Bier / DL1HRC
@date	 2017-03-10

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2004-2017  Tobias Blomberg / SM0SVX

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

#ifndef ASYNC_AUDIO_RECODER_DV3K
#define ASYNC_AUDIO_RECODER_DV3K


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <cmath>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncAudioRecoder.h>


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
@brief	An audio pipe class for AudioRecoderDV3k
@author Tobias Blomberg / SM0SVX
@date   2006-07-08

Use this class to amplify or attenuate an audio stream.
*/
class AudioRecoderDV3k : public AudioRecoder
{
  public:
    /**
     * @brief 	Default constuctor
     */
    AudioRecoderDV3k(void) {};
  
    /**
     * @brief 	Destructor
     */
    virtual ~AudioRecoderDV3k(void) {};
  
    /**
     * @brief 	Set the gain to use
     * @param 	gain_db The gain given in dB
     */
 //   virtual void setGain(float gain_db) { m_gain = powf(10, gain_db / 20); }
    
    /**
     * @brief 	Read the gain
     * @return	Return the gain in dB
     */
  //  virtual float gain(void) const { return 20 * log10(m_gain); }
    
    
  protected:
   /* void processSamples(float *dest, const float *src, int count)
    {
      for (int i=0; i<count; ++i)
      {
      	dest[i] = src[i] * m_gain;
      }
    }
    */
    
  private:
    AudioRecoderDV3k(const AudioRecoderDV3k&);
    AudioRecoderDV3k& operator=(const AudioRecoderDV3k&);
    
    float m_gain;
    
};  /* class AudioRecoderDV3k */


} /* namespace */

#endif /* ASYNC_AUDIO_RECODER_DV3K */



/*
 * This file has not been truncated
 */

