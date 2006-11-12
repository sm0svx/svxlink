/**
@file	 ToneDetector.cpp
@brief   A tone detector that use the Goertzel algorithm
@author  Tobias Blomberg / SM0SVX
@date	 2003-04-15

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




/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cassert>
#include <iostream>
#include <algorithm>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AudioFilter.h>
#include <SigCAudioSink.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "ToneDetector.h"



/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/

using namespace std;
using namespace SigC;
using namespace Async;



/****************************************************************************
 *
 * Defines & typedefs
 *
 ****************************************************************************/

#define SAMPLING_RATE	    8000.0
//#define THRESHOLD   	    5000000.0
#define DEFAUILT_SNR_THRESH -5.0
#define DEFAULT_DET_DELAY   2
#define DEFAULT_UNDET_DELAY 3


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


/*
 *------------------------------------------------------------------------
 * Method:    
 * Purpose:   
 * Input:     
 * Output:    
 * Author:    
 * Created:   
 * Remarks:   
 * Bugs:      
 *------------------------------------------------------------------------
 */
ToneDetector::ToneDetector(float tone_hz, int base_N)
  : block_pos(0), is_activated(false), result(0), tone_fq(tone_hz),
    filter(0), det_delay_left(DEFAULT_DET_DELAY), undet_delay_left(0),
    snr_thresh(DEFAUILT_SNR_THRESH)
{
  //int 	    k;
  FLOATING  k;
  FLOATING  floatN;
  FLOATING  omega;

  floatN = (FLOATING) base_N;
  N = base_N;
  //k = (int) (0.5 + ((floatN * tone_hz) / SAMPLING_RATE));
  k = ((floatN * tone_hz) / SAMPLING_RATE);
  //k = (int)(((floatN * tone_hz) / SAMPLING_RATE));
  //N = (int)(k * SAMPLING_RATE / tone_hz + 0.5);
  floatN = N;
  omega = (2.0 * M_PI * k) / floatN;
  sine = sin(omega);
  cosine = cos(omega);
  coeff = 2.0 * cosine;

  //printf("For SAMPLING_RATE = %f", SAMPLING_RATE);
  //printf(" N = %.2f", floatN);
  //printf(" and FREQUENCY = %d,\n", tone_hz);
  //printf("k = %d and coeff = %f\n\n", k, coeff);

  block = new float[N];
  
  resetGoertzel();

  sigc_sink = new SigCAudioSink;
  sigc_sink->sigWriteSamples.connect(slot(*this, &ToneDetector::processSamples));
  sigc_sink->sigFlushSamples.connect(
      slot(*sigc_sink, &SigCAudioSink::allSamplesFlushed));
  assert(setHandler(sigc_sink));
  
} /* ToneDetector::ToneDetector */


ToneDetector::~ToneDetector(void)
{
  delete [] block;
  
  setFilter("");
  setHandler(0);
  
  delete sigc_sink;
  
} /* ToneDetector::~ToneDetector */


void ToneDetector::setFilter(const std::string &filter_spec)
{
  this->filter_spec = filter_spec;
  
  if (filter != 0)
  {
    sigc_sink->unregisterSource();
    delete filter;
    filter = 0;
    assert(setHandler(sigc_sink));
  }
  
  if (!filter_spec.empty())
  {
    filter = new AudioFilter(filter_spec);
    assert(setHandler(filter));
    assert(sigc_sink->registerSource(filter));
  }
      
} /* ToneDetector::setFilter */


void ToneDetector::reset(void)
{
  resetGoertzel();
  is_activated = false;
  det_delay_left = DEFAULT_DET_DELAY;
  undet_delay_left = 0;
} /* ToneDetector::reset */


/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/


/*
 *------------------------------------------------------------------------
 * Method:    
 * Purpose:   
 * Input:     
 * Output:    
 * Author:    
 * Created:   
 * Remarks:   
 * Bugs:      
 *------------------------------------------------------------------------
 */






/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/


/*
 *----------------------------------------------------------------------------
 * Method:    
 * Purpose:   
 * Input:     
 * Output:    
 * Author:    
 * Created:   
 * Remarks:   
 * Bugs:      
 *----------------------------------------------------------------------------
 */

void ToneDetector::resetGoertzel(void)
{
  Q2 = 0;
  Q1 = 0;
  block_pos = 0;
} /* ToneDetector::resetGoertzel */


void ToneDetector::processSample(SAMPLE sample)
{
  if (sample > 1)
  {
    sample = 32767;
  }
  else if (sample < -1)
  {
    sample = -32767;
  }
  else
  {
    sample *= 32767.0;
  }
  unsigned char usample = ((int)sample + 0x8000) >> 8;
  FLOATING Q0;
  Q0 = coeff * Q1 - Q2 + (FLOATING) usample;
  Q2 = Q1;
  Q1 = Q0;
} /* ToneDetector::processSample */


#if 0
/* Basic Goertzel */
/* Call this routine after every block to get the complex result. */
void ToneDetector::getRealImag(FLOATING *realPart, FLOATING *imagPart)
{
  *realPart = (Q1 - Q2 * cosine);
  *imagPart = (Q2 * sine);
}
#endif

/* Optimized Goertzel */
/* Call this after every block to get the RELATIVE magnitude squared. */
FLOATING ToneDetector::getMagnitudeSquared(void)
{
  FLOATING result;

  result = Q1 * Q1 + Q2 * Q2 - Q1 * Q2 * coeff;
  return result;
}


int ToneDetector::processSamples(float *buf, int len)
{
  //printf("Processing %d samples\n", len);
  
  int orig_len = len;
  
  while (len > 0)
  {
    int samples_to_read = min(len, N - block_pos);
    memcpy(block + block_pos, buf, samples_to_read * sizeof(*buf));
    block_pos += samples_to_read;
    len -= samples_to_read;
    buf += samples_to_read;
    
    if (block_pos == N)
    {
      resetGoertzel();
      
      if (filter != 0)
      {
      	filter->reset();
      }
      
      double rms = 0.0;
      double Q1 = 0.0, Q2 = 0.0;
      for (int i=0; i<N; i++)
      {
	double sample = static_cast<double>(block[i]);
	sample *= 0.5 - 0.5 * cos(2 * M_PI * i / N);
      	rms += sample * sample;
	processSample(sample);
	double Q0 = coeff * Q1 - Q2 + sample;
	Q2 = Q1;
	Q1 = Q0;
      }
      rms = sqrt(rms / N);
      double res = sqrt(Q1 * Q1 - coeff * Q1 * Q2 + Q2 * Q2);
      res /= 1000.0;
      double snr = 10 * log10(res / (rms - res));
      result = getMagnitudeSquared();
      /*
      if (toneFq() < 300)
      {
      	printf("fq=%.1f  rms=%.3f  result=%.3f  snr=%.3f\n",
	      	toneFq(), rms, res, snr);
      }
      */
      
      valueChanged(this, result);
      if (snr >= snr_thresh)
      {
        if (det_delay_left > 0)
	{
      	  --det_delay_left;
	  if (det_delay_left == 0)
	  {
      	    //printf("tone=%.1f  result=%14.0f  activated\n", tone_fq, result);
	    is_activated = true;
	    activated(true);
	  }
	}
	if (is_activated)
	{
	  undet_delay_left = DEFAULT_UNDET_DELAY;
	}
      }
      else
      {
	if (undet_delay_left > 0)
	{
	  --undet_delay_left;
	  if (undet_delay_left == 0)
	  {
      	    //printf("tone=%.1f  result=%14.0f deactivated\n", tone_fq, result);
	    is_activated = false;
	    activated(false);
	  }
	}
	if (!is_activated)
	{
	  det_delay_left = DEFAULT_DET_DELAY;
	}
      }
    }
  }
    
  return orig_len;
  
} /* ToneDetector::processSamples */




/*
 * This file has not been truncated
 */

