/**
@file	 AsyncAudioDecimator.cpp
@brief   Decimates a higher sample rate to a lower one
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



/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <cstring>


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

#include "AsyncAudioDecimator.h"



/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/

using namespace std;
using namespace Async;



/****************************************************************************
 *
 * Defines & typedefs
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Local class definitions
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Prototypes
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Exported Global Variables
 *
 ****************************************************************************/




/****************************************************************************
 *
 * Local Global Variables
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/

AudioDecimator::AudioDecimator(int decimation_factor,
      	      	      	       const float *filter_coeff, int taps)
  : factor_M(decimation_factor), H_size(taps), p_H(filter_coeff)
{
  setInputOutputSampleRate(factor_M, 1);
  p_Z = new float[H_size];
  memset(p_Z, 0, H_size * sizeof(*p_Z));
} /* AudioDecimator::AudioDecimator */


AudioDecimator::~AudioDecimator(void)
{
  delete [] p_Z;
} /* AudioDecimator::~AudioDecimator */




/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/

void AudioDecimator::processSamples(float *dest, const float *src, int count)
{
  int orig_count = count;
  
  //printf("count=%d\n", count);
  
    // this implementation assumes num_inp is a multiple of factor_M
  assert(count % factor_M == 0);

  int num_out = 0;
  while (count >= factor_M)
  {
      // shift Z delay line up to make room for next samples
    memmove(p_Z + factor_M, p_Z, (H_size - factor_M) * sizeof(float));

      // copy next samples from input buffer to bottom of Z delay line
    for (int tap = factor_M - 1; tap >= 0; tap--)
    {
      p_Z[tap] = *src++;
    }
    count -= factor_M;

      // calculate FIR sum
    float sum = 0.0;
    for (int tap = 0; tap < H_size; tap++)
    {
      sum += p_H[tap] * p_Z[tap];
    }
    *dest++ = sum;     /* store sum and point to next output */
    num_out++;
  }

  //printf("num_out=%d  count=%d  factor_M=%d\n", num_out, count, factor_M);
  assert(num_out == orig_count / factor_M);
  
} /* AudioDecimator::processSamples */



/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/



/*
 * This file has not been truncated
 */

