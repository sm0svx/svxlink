/**
@file	 AsyncAudioAmp.h
@brief   A_brief_description_for_this_file
@author  Tobias Blomberg / SM0SVX
@date	 2005-08-

A_detailed_description_for_this_file

\verbatim
<A brief description of the program or library this file belongs to>
Copyright (C) 2004-2005  Tobias Blomberg / SM0SVX

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

/** @example AsyncAudioAmp_demo.cpp
An example of how to use the AsyncAudioAmp class
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
@brief	A_brief_class_description
@author Tobias Blomberg / SM0SVX
@date   2006-07-08

A_detailed_class_description

\include AsyncAudioAmp_demo.cpp
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

