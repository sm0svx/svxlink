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
#include <cstring>
#include <cmath>
#include <cassert>
#include <iostream>
#include <algorithm>


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

#define DEFAULT_PEAK_THRESH	10.0	// 10dB
#define DEFAULT_DET_DELAY	3
#define DEFAULT_UNDET_DELAY	3
#define DEFAULT_ENERGY_THRESH	0.1f


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
ToneDetector::ToneDetector(float tone_hz, float width_hz)
  : samples_left(0), is_activated(false), tone_fq(tone_hz), block_len(0),
    det_delay_left(DEFAULT_DET_DELAY), undet_delay_left(0),
    peak_thresh(DEFAULT_PEAK_THRESH), energy_thresh(DEFAULT_ENERGY_THRESH)
{
  // The Goertzel algorithm is just a recursive way to evaluate the DFT at a
  // single frequency. For maximum detection reliability, the bandwidth will
  // be adapted to place the tone frequency near the center of the DFT.
  // As a side effect, the detection bandwidth is slightly narrowed, which
  // however is acceptable for the current use cases (CTCSS, 1750Hz, etc..).

  /* Adjust block length to minimize the DFT error */
  block_len = lrintf(INTERNAL_SAMPLE_RATE * ceilf(tone_hz / width_hz) / tone_hz);
  /* Center frequency */
  goertzelInit(&center, tone_hz, INTERNAL_SAMPLE_RATE);
  /* Lower frequency */
  goertzelInit(&lower, tone_hz - 2 * width_hz, INTERNAL_SAMPLE_RATE);
  /* Upper frequency */
  goertzelInit(&upper, tone_hz + 2 * width_hz, INTERNAL_SAMPLE_RATE);
  /* Hamming window */  
  for (int i = 0; i < block_len; i++)
  {
    window_table.push_back(
      0.54 - 0.46 * cosf(2.0f * M_PI * i / (block_len - 1)));
  }
  /* Point to the first table entry */
  win = window_table.begin();
  /* Load sample counter */
  samples_left = block_len;

} /* ToneDetector::ToneDetector */


void ToneDetector::reset(void)
{
  is_activated = false;
  det_delay_left = DEFAULT_DET_DELAY;
  undet_delay_left = 0;
} /* ToneDetector::reset */


int ToneDetector::writeSamples(const float *buf, int len)
{
  float famp;
  float v1;

  /* divide buffer into blocks */
  for (int i = 0;  i < len;  i++)
  {
    famp = *(buf++) * *(win++);
    /* Center frequency */
    v1 = center.v2;
    center.v2 = center.v3;
    center.v3 = center.fac * center.v2 - v1 + famp;
    /* Lower frequency */
    v1 = lower.v2;
    lower.v2 = lower.v3;
    lower.v3 = lower.fac * lower.v2 - v1 + famp;
    /* Upper frequency */
    v1 = upper.v2;
    upper.v2 = upper.v3;
    upper.v3 = upper.fac * upper.v2 - v1 + famp;

    if (--samples_left == 0)
      postProcess();
  }
    
  return len;
  
} /* ToneDetector::writeSamples */


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

void ToneDetector::postProcess(void)
{
  /* Center frequency */
  float res_center = goertzelResult(&center);
  /* Lower frequency */
  float res_lower = goertzelResult(&lower);
  /* Upper frequency */
  float res_upper = goertzelResult(&upper);

  /* Point to the first table entry */
  win = window_table.begin();
  /* Reload sample counter */
  samples_left = block_len;

  if ((res_center > energy_thresh) &&
      (res_center > (res_lower * peak_thresh)) &&
      (res_center > (res_upper * peak_thresh)))
  {
    if (det_delay_left > 0)
    {
      if (--det_delay_left == 0)
      {
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
      if (--undet_delay_left == 0)
      {
        is_activated = false;
	activated(false);
      }
    }
    if (!is_activated)
    {
      det_delay_left = DEFAULT_DET_DELAY;
    }
  }
} /* ToneDetector::postProcess */


void ToneDetector::goertzelInit(GoertzelState *s, float freq, int sample_rate)
{
    s->fac = 2.0f * cosf(2.0f * M_PI * (freq / (float)sample_rate));
    s->v2 = s->v3 = 0.0f;
} /* ToneDetector::goertzelInit */


float ToneDetector::goertzelResult(GoertzelState *s)
{
    float v1, res;

    /* Push a zero through the process to finish things off. */
    v1 = s->v2;
    s->v2 = s->v3;
    s->v3 = s->fac*s->v2 - v1;
    /* Now calculate the non-recursive side of the filter. */
    /* The result here is not scaled down to allow for the magnification
       effect of the filter (the usual DFT magnification effect). */
    res = s->v3*s->v3 + s->v2*s->v2 - s->v2*s->v3*s->fac;
    /* Reset the tone detector state. */
    s->v2 = s->v3 = 0.0f;
    /* Return the calculated signal level. */
    return res;
} /* ToneDetector::goertzelResult */

/*
 * This file has not been truncated
 */
