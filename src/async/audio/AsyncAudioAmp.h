/**
@file	 AsyncAudioAmp.h
@brief   Contains an audio pipe class for amplification/attenuation
@author  Tobias Blomberg / SM0SVX
@date	 2006-07-08

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2004-2008  Tobias Blomberg / SM0SVX

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

#ifndef ASYNC_AUDIO_AMP_INCLUDED
#define ASYNC_AUDIO_AMP_INCLUDED


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

#include <AsyncAudioProcessor.h>


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
@brief	An audio pipe class for amplification/attenuation of an audio stream
@author Tobias Blomberg / SM0SVX
@date   2006-07-08

Use this class to amplify or attenuate an audio stream.
*/
class AudioAmp : public Async::AudioProcessor
{
  public:
    /**
     * @brief 	Default constuctor
     */
    AudioAmp(void) : m_gain(1.0) {}
  
    /**
     * @brief 	Destructor
     */
    ~AudioAmp(void) {}
  
    /**
     * @brief 	Set the gain to use
     * @param 	gain_db The gain given in dB
     */
    void setGain(float gain_db) { m_gain = powf(10, gain_db / 20); }
    
    /**
     * @brief 	Read the gain
     * @return	Return the gain in dB
     */
    float gain(void) const { return 20 * log10(m_gain); }
    
    
  protected:
    void processSamples(float *dest, const float *src, int count)
    {
      for (int i=0; i<count; ++i)
      {
      	dest[i] = src[i] * m_gain;
      }
    }
    
    
  private:
    AudioAmp(const AudioAmp&);
    AudioAmp& operator=(const AudioAmp&);
    
    float m_gain;
    
};  /* class AudioAmp */


} /* namespace */

#endif /* ASYNC_AUDIO_AMP_INCLUDED */



/*
 * This file has not been truncated
 */

