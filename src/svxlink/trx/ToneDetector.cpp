/**
@file	 ToneDetector.cpp
@brief   A tone detector that use the Goertzel algorithm
@author  Tobias Blomberg / SM0SVX
@date	 2003-04-15

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2004-2011  Tobias Blomberg / SM0SVX

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
#define DEFAULT_DET_POSITIVES	3
#define DEFAULT_GAP_NEGATIVES	3
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
ToneDetector::ToneDetector(float tone_hz, float width_hz, int det_delay_ms)
  : tone_fq(tone_hz), det_delay(DEFAULT_DET_POSITIVES),
    gap_delay(DEFAULT_GAP_NEGATIVES), block_len(0), samples_left(0),
    is_activated(false), last_active(false), peak_thresh(DEFAULT_PEAK_THRESH),
    stable_count(0)
{
  // The Goertzel algorithm is just a recursive way to evaluate the DFT at a
  // single frequency. For maximum detection reliability, the bandwidth will
  // be adapted to place the tone frequency near the center of the DFT.
  // As a side effect, the detection bandwidth is slightly narrowed, which
  // however is acceptable for the current use cases (CTCSS, 1750Hz, etc..).

    // Adjust block length to minimize the DFT error
  block_len = lrintf(INTERNAL_SAMPLE_RATE * ceilf(tone_hz / width_hz) / tone_hz);
  goertzelInit(&center, tone_hz, INTERNAL_SAMPLE_RATE);
  goertzelInit(&lower, tone_hz - 2 * width_hz, INTERNAL_SAMPLE_RATE);
  goertzelInit(&upper, tone_hz + 2 * width_hz, INTERNAL_SAMPLE_RATE);

    // Set up Hamming window coefficients
  for (int i = 0; i < block_len; i++)
  {
    window_table.push_back(
      0.54 - 0.46 * cosf(2.0f * M_PI * i / (block_len - 1)));
  }

  setDetectionDelay(det_delay_ms);
  reset();
} /* ToneDetector::ToneDetector */


void ToneDetector::reset(void)
{
  is_activated = false;
  last_active = false;
  stable_count = 0;
  samples_left = block_len;

    // Point to the first windowing table entry
  win = window_table.begin();

    // Reset Goertzel filters
  center.v2 = center.v3 = 0.0f;
  lower.v2 = lower.v3 = 0.0f;
  upper.v2 = upper.v3 = 0.0f;
} /* ToneDetector::reset */


void ToneDetector::setDetectionDelay(int delay_ms)
{
  if (delay_ms > 0)
  {
    det_delay = static_cast<int>(
		ceil(delay_ms * INTERNAL_SAMPLE_RATE / (block_len * 1000.0)));
  }
  else
  {
    det_delay = DEFAULT_DET_POSITIVES;
  }
} /* ToneDetector::setDetectionDelay */


int ToneDetector::detectionDelay(void) const
{
  return det_delay * block_len * 1000 / INTERNAL_SAMPLE_RATE;
} /* ToneDetector::detectionDelay */


void ToneDetector::setGapDelay(int delay_ms)
{
  if (delay_ms > 0)
  {
    gap_delay = static_cast<int>(
		ceil(delay_ms * INTERNAL_SAMPLE_RATE / (block_len * 1000.0)));
  }
  else
  {
    gap_delay = DEFAULT_GAP_NEGATIVES;
  }
} /* ToneDetector::setGapDelay */


int ToneDetector::gapDelay(void) const
{
  return gap_delay * block_len * 1000 / INTERNAL_SAMPLE_RATE;
} /* ToneDetector::gapDelay */


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

void ToneDetector::postProcess(void)
{
  float res_center = goertzelResult(&center);
  float res_lower = goertzelResult(&lower);
  float res_upper = goertzelResult(&upper);

    // Point to the first table entry
  win = window_table.begin();
    // Reload sample counter
  samples_left = block_len;

  bool active = ((res_center > DEFAULT_ENERGY_THRESH) &&
      (res_center > (res_lower * peak_thresh)) &&
      (res_center > (res_upper * peak_thresh)));

  if (active == last_active)
  {
    stable_count += 1;
  }
  else
  {
    stable_count = 1;
  }
  last_active = active;
  
  if (!is_activated)
  {
    if (active && (stable_count >= det_delay))
    {
      is_activated = true;
      activated(true);
      detected(tone_fq);
    }
  }
  else
  {
    if (!active && (stable_count >= gap_delay))
    {
      is_activated = false;
      activated(false);
    }
  }
} /* ToneDetector::postProcess */


void ToneDetector::goertzelInit(GoertzelState *s, float freq, int sample_rate)
{
  s->fac = 2.0f * cosf(2.0f * M_PI * (freq / (float)sample_rate));
    // Reset Goertzel filter
  s->v2 = s->v3 = 0.0f;
} /* ToneDetector::goertzelInit */


float ToneDetector::goertzelResult(GoertzelState *s)
{
  float v1, res;

    // Push a zero through the process to finish things off
  v1 = s->v2;
  s->v2 = s->v3;
  s->v3 = s->fac * s->v2 - v1;

    // Now calculate the non-recursive side of the filter.
    // The result here is not scaled down to allow for the magnification
    // effect of the filter (the usual DFT magnification effect).
  res = s->v3 * s->v3 + s->v2 * s->v2 - s->v2 * s->v3 * s->fac;

    // Reset Goertzel filter
  s->v2 = s->v3 = 0.0f;

  return res;

} /* ToneDetector::goertzelResult */


/*
 * This file has not been truncated
 */
