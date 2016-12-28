/**
@file	 AsyncAudioInterpolator.h
@brief   Interpolates a lower sampling rate to a higher one
@author  Tobias Blomberg / SM0SVX
@date	 2008-04-06

\verbatim
Original code by by Grant R. Griffin modified by Tobias Blomberg / SM0SVX.
Provided by Iowegian's "dspGuru" service (http://www.dspguru.com).
Copyright 2001, Iowegian International Corporation (http://www.iowegian.com)

                         The Wide Open License (WOL)

Permission to use, copy, modify, distribute and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice and this license appear in all source copies. 
THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT EXPRESS OR IMPLIED WARRANTY OF
ANY KIND. See http://www.dspguru.com/wol.htm for more information.
\endverbatim
*/

#ifndef ASYNC_AUDIO_INTERPOLATOR_INCLUDED
#define ASYNC_AUDIO_INTERPOLATOR_INCLUDED


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
@brief	Interpolates a lower sampling rate to a higher one
@author Tobias Blomberg / SM0SVX
@date   2008-04-06

This audio pipe class will interpolate an audio stream up to a higher sampling
rate. Interpolation is a process where the sampling rate is increased by an
integer factor. After the increase in sampling rate, a lowpass filter must be
applied to avoid aliasing effects. This filter is built into this component.
However, the filter coefficients (FIR) must be calculated manually.

Use this web page to calculate the coefficients:
http://www.dsptutor.freeuk.com/remez/RemezFIRFilterDesign.html

This implementation is based on the multirate FAQ at dspguru.com:
http://dspguru.com/info/faqs/mrfaq.htm
*/
class AudioInterpolator : public Async::AudioProcessor
{
  public:
    /**
     * @brief 	Constuctor
     * @param 	interpolation_factor The factor to increase the sample rate with
     * @param 	filter_coeff The filter coefficients
     * @param 	taps The number of taps in the filter
     */
    AudioInterpolator(int interpolation_factor, const float *filter_coeff,
      	      	      int taps);
  
    /**
     * @brief 	Destructor
     */
    ~AudioInterpolator(void);
  

  protected:
    /**
     * @brief Process incoming samples and put them into the output buffer
     * @param dest  Destination buffer
     * @param src   Source buffer
     * @param count Number of samples in the source buffer
     *
     * This function should be reimplemented by the inheriting class to
     * do the actual processing of the incoming samples. All samples must
     * be processed, otherwise they are lost and the output buffer will
     * contain garbage.
     */
    virtual void processSamples(float *dest, const float *src, int count);

    
  private:
    const int 	factor_L;
    float     	*p_Z;
    int       	L_size;
    const float *p_H;

    AudioInterpolator(const AudioInterpolator&);
    AudioInterpolator& operator=(const AudioInterpolator&);
    
};  /* class AudioInterpolator */


} /* namespace */

#endif /* ASYNC_AUDIO_INTERPOLATOR_INCLUDED */



/*
 * This file has not been truncated
 */

