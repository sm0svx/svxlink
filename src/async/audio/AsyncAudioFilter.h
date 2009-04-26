/**
@file	 AsyncAudioFilter.h
@brief   Contains a class for creating a wide range of audio filters
@author  Tobias Blomberg / SM0SVX
@date	 2006-04-23

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


#ifndef ASYNC_AUDIO_FILTER_INCLUDED
#define ASYNC_AUDIO_FILTER_INCLUDED


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

class FidVars;
  

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
@brief	A class for creating a wide range of audio filters
@author Tobias Blomberg / SM0SVX
@date   2006-04-23
*/
class AudioFilter : public AudioProcessor
{
  public:
    /**
     * @brief 	Constuctor
     * @param 	filter_spec The filter specification
     * @param 	sample_rate The sampling rate
     */
    explicit AudioFilter(const std::string &filter_spec,
      	      	      	 int sample_rate = INTERNAL_SAMPLE_RATE);
  
    /**
     * @brief 	Destructor
     */
    ~AudioFilter(void);
  
    /**
     * @brief 	Set the output gain of the filter
     * @param 	gain The gain to set
     */
    void setOutputGain(float gain) { output_gain = gain; }
    
    /**
     * @brief Reset the filter state
     */
    void reset(void);
    
    
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
    FidVars   	*fv;
    float     	output_gain;
    
    AudioFilter(const AudioFilter&);
    AudioFilter& operator=(const AudioFilter&);

};  /* class AudioFilter */


} /* namespace */

#endif /* ASYNC_AUDIO_FILTER_INCLUDED */



/*
 * This file has not been truncated
 */

