/**
@file	 AsyncAudioNoiseAdder.h
@brief   A class to add white gaussian noise to an audio stream
@author  Tobias Blomberg / SM0SVX
@date	 2015-03-08

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


#ifndef ASYNC_AUDIO_NOISE_ADDER
#define ASYNC_AUDIO_NOISE_ADDER


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <string>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include <AsyncAudioProcessor.h>



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
@brief	A class to add gaussian white noise to an audio stream
@author Tobias Blomberg / SM0SVX
@date   2015-03-08

This class implement a noise generator that add white gaussian noise to an
audio stream. The noise is generated using the Box-Muller transform which for
example is described on Wikipedia:

  http://en.wikipedia.org/wiki/Box%E2%80%93Muller_transform

The class is not implemented as a pure audio source but rather as an audio pipe
component that should be inserted in the audio path.
*/
class AudioNoiseAdder : public AudioProcessor
{
  public:
    /**
     * @brief 	Constuctor
     * @param   level_db The noise level in dB
     *
     * The level_db parameter deserve some clarification. A level of 0dB give
     * the same power as in a full scale sine wave. This make it possible to
     * add noise to a generated signal under controlled conditions.
     * For example, if a sine wave is generated in SvxLink with a level of 0dB
     * and noise is added with a level of -10dB, we get a signal to noise ratio
     * (SNR) of 10dB.
     */
    AudioNoiseAdder(float level_db);
  
    /**
     * @brief 	Destructor
     */
    ~AudioNoiseAdder(void);
    
  protected:
    /**
     * @brief Process incoming samples and put them into the output buffer
     * @param dest  Destination buffer
     * @param src   Source buffer
     * @param count Number of samples in the source buffer
     *
     * This function is called from the base class to do the actual
     * processing of the incoming samples. All samples must
     * be processed, otherwise they are lost and the output buffer will
     * contain garbage.
     */
    void processSamples(float *dest, const float *src, int count);

  private:
    float sigma;        // Standard deviation of the generated noise
    float z1;
    bool generate;
    unsigned int seed;

    AudioNoiseAdder(const AudioNoiseAdder&);
    AudioNoiseAdder& operator=(const AudioNoiseAdder&);
    float generateGaussianNoise(void);

};  /* class AudioNoiseAdder */


} /* namespace */

#endif /* ASYNC_AUDIO_NOISE_ADDER */


/*
 * This file has not been truncated
 */
