/**
@file	 AsyncAudioInterpolator.cpp
@brief   A_brief_description_for_this_file
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

#include "AsyncAudioInterpolator.h"



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

AudioInterpolator::AudioInterpolator(int interpolation_factor,
      	      	      	      	     const float *filter_coeff, int taps)
  : factor_L(interpolation_factor), L_size(taps), p_H(filter_coeff)
{
  //p_H = new float[taps];
  //memcpy(p_H, filter_coeff, taps * sizeof(*p_H));
  
  setInputOutputSampleRate(1, factor_L);

    // FIXME: What if L_size does not divide evenly with factor_L?
  size_t p_Z_size = L_size / factor_L;
  p_Z = new float[p_Z_size];
  memset(p_Z, 0, sizeof(*p_Z) * p_Z_size);
} /* AudioInterpolator::AudioInterpolator */


AudioInterpolator::~AudioInterpolator(void)
{
  delete [] p_Z;
} /* AudioInterpolator::~AudioInterpolator */




/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/

void AudioInterpolator::processSamples(float *dest, const float *src, int count)
{
  int orig_count = count;
  int num_taps_per_phase = L_size / factor_L;
  
  int num_out = 0;
  while (count-- > 0)
  {
      // shift Z delay line up to make room for next sample
    memmove(p_Z + 1, p_Z, (num_taps_per_phase - 1) * sizeof(float));

      // copy next sample from input buffer to bottom of Z delay line
    p_Z[0] = *src++;

      // calculate outputs
    for (int phase_num = 0; phase_num < factor_L; phase_num++)
    {
      	// point to the current polyphase filter
      const float *p_coeff = p_H + phase_num;

      	// calculate FIR sum
      float sum = 0.0;
      for (int tap = 0; tap < num_taps_per_phase; tap++)
      {
        sum += *p_coeff * p_Z[tap];
        p_coeff += factor_L;          /* point to next coefficient */
      }
      *dest++ = sum * factor_L; /* store scaled sum and point to next output */
      num_out++;
    }
  }

  //printf("num_out=%d  orig_count=%d\n", num_out, orig_count);
  assert(num_out == orig_count * factor_L);
  
} /* AudioInterpolator::processSamples */




/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/



/*
 * This file has not been truncated
 */

