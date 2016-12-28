/**
@file	 Dh1dmSwDtmfDecoder.cpp
@brief   This file contains a class that implements a sw DTMF decoder
@author  Tobias Blomberg / SM0SVX
@date	 2003-04-16

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
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

/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <iostream>
#include <algorithm>
#include <cstring>
#include <cmath>
#include <cstdlib>

#include <stdint.h>



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

#include "Dh1dmSwDtmfDecoder.h"



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

// All values are squared magnitude ratios !
#define DTMF_NORMAL_TWIST           6.3f   /* 8dB */
#define DTMF_REVERSE_TWIST          6.3f   /* 8dB */
#define DTMF_RELATIVE_PEAK          20.0f  /* 13dB */

// The Goertzel algorithm is just a recursive way to evaluate the DFT at a
// single frequency. According to ITU-T Q.23 und Q.24, the bandwidth of
// each detector has to be set to 3% of its center frequency for maximum
// detection reliability. To achieve this, the detectors use individual
// block lengths resulting in different decision periods. The detection
// process itself involves scaling and sampling of the tone detector results.
#define DTMF_BLOCK_LENGTH           (INTERNAL_SAMPLE_RATE / 1000)


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

Dh1dmSwDtmfDecoder::Dh1dmSwDtmfDecoder(Config &cfg, const string &name)
  : DtmfDecoder(cfg, name), samples_left(DTMF_BLOCK_LENGTH),
    last_hit(0), last_stable(0), stable_timer(0), active_timer(0),
    normal_twist(DTMF_NORMAL_TWIST), reverse_twist(DTMF_REVERSE_TWIST)
{
    memset(row_energy, 0, sizeof(row_energy));
    memset(col_energy, 0, sizeof(col_energy));
    
    /* Init row detectors */
    goertzelInit(&row_out[0], 697.0f, 0.0f);
    goertzelInit(&row_out[1], 770.0f, 0.0f);
    goertzelInit(&row_out[2], 852.0f, 0.0f);
    goertzelInit(&row_out[3], 941.0f, 0.0f);

    /* Sliding window Goertzel algorithm */
    goertzelInit(&row_out[4], 697.0f, 0.5f);
    goertzelInit(&row_out[5], 770.0f, 0.5f);
    goertzelInit(&row_out[6], 852.0f, 0.5f);
    goertzelInit(&row_out[7], 941.0f, 0.5f);

    /* Init column detectors */
    goertzelInit(&col_out[0], 1209.0f, 0.0f);
    goertzelInit(&col_out[1], 1336.0f, 0.0f);
    goertzelInit(&col_out[2], 1477.0f, 0.0f);
    goertzelInit(&col_out[3], 1633.0f, 0.0f);

    /* Sliding window Goertzel algorithm */
    goertzelInit(&col_out[4], 1209.0f, 0.5f);
    goertzelInit(&col_out[5], 1336.0f, 0.5f);
    goertzelInit(&col_out[6], 1477.0f, 0.5f);
    goertzelInit(&col_out[7], 1633.0f, 0.5f);

} /* Dh1dmSwDtmfDecoder::Dh1dmSwDtmfDecoder */


bool Dh1dmSwDtmfDecoder::initialize(void)
{
  if (!DtmfDecoder::initialize())
  {
    return false;
  }
  
  string value;

  if (cfg().getValue(name(), "DTMF_MAX_FWD_TWIST", value))
  {
    int cfg_fwd_twist = atoi(value.c_str());
    if (cfg_fwd_twist >= 0)
      normal_twist = powf(10,cfg_fwd_twist/10.0f);
  }
  
  if (cfg().getValue(name(), "DTMF_MAX_REV_TWIST", value))
  {
    int cfg_rev_twist = atoi(value.c_str());
    if (cfg_rev_twist >= 0)
      reverse_twist = powf(10,cfg_rev_twist/10.0f);
  }
  
  return true;
  
} /* Dh1dmSwDtmfDecoder::initialize */


int Dh1dmSwDtmfDecoder::writeSamples(const float *buf, int len)
{
    for (int i = 0; i < len; i++)
    {
        float v1;
        float famp = *(buf++);

        /* Row detectors */
        v1 = row_out[0].v2;
        row_out[0].v2 = row_out[0].v3;
        row_out[0].v3 = row_out[0].fac * row_out[0].v2 - v1 + famp * *(row_out[0].win++);

        v1 = row_out[1].v2;
        row_out[1].v2 = row_out[1].v3;
        row_out[1].v3 = row_out[1].fac * row_out[1].v2 - v1 + famp * *(row_out[1].win++);
        
        v1 = row_out[2].v2;
        row_out[2].v2 = row_out[2].v3;
        row_out[2].v3 = row_out[2].fac * row_out[2].v2 - v1 + famp * *(row_out[2].win++);

        v1 = row_out[3].v2;
        row_out[3].v2 = row_out[3].v3;
        row_out[3].v3 = row_out[3].fac * row_out[3].v2 - v1 + famp * *(row_out[3].win++);

        v1 = row_out[4].v2;
        row_out[4].v2 = row_out[4].v3;
        row_out[4].v3 = row_out[4].fac * row_out[4].v2 - v1 + famp * *(row_out[4].win++);

        v1 = row_out[5].v2;
        row_out[5].v2 = row_out[5].v3;
        row_out[5].v3 = row_out[5].fac * row_out[5].v2 - v1 + famp * *(row_out[5].win++);
        
        v1 = row_out[6].v2;
        row_out[6].v2 = row_out[6].v3;
        row_out[6].v3 = row_out[6].fac * row_out[6].v2 - v1 + famp * *(row_out[6].win++);

        v1 = row_out[7].v2;
        row_out[7].v2 = row_out[7].v3;
        row_out[7].v3 = row_out[7].fac * row_out[7].v2 - v1 + famp * *(row_out[7].win++);

        /* Column detectors */
        v1 = col_out[0].v2;
        col_out[0].v2 = col_out[0].v3;
        col_out[0].v3 = col_out[0].fac * col_out[0].v2 - v1 + famp * *(col_out[0].win++);

        v1 = col_out[1].v2;
        col_out[1].v2 = col_out[1].v3;
        col_out[1].v3 = col_out[1].fac * col_out[1].v2 - v1 + famp * *(col_out[1].win++);

        v1 = col_out[2].v2;
        col_out[2].v2 = col_out[2].v3;
        col_out[2].v3 = col_out[2].fac * col_out[2].v2 - v1 + famp * *(col_out[2].win++);

        v1 = col_out[3].v2;
        col_out[3].v2 = col_out[3].v3;
        col_out[3].v3 = col_out[3].fac * col_out[3].v2 - v1 + famp * *(col_out[3].win++);

        v1 = col_out[4].v2;
        col_out[4].v2 = col_out[4].v3;
        col_out[4].v3 = col_out[4].fac * col_out[4].v2 - v1 + famp * *(col_out[4].win++);

        v1 = col_out[5].v2;
        col_out[5].v2 = col_out[5].v3;
        col_out[5].v3 = col_out[5].fac * col_out[5].v2 - v1 + famp * *(col_out[5].win++);

        v1 = col_out[6].v2;
        col_out[6].v2 = col_out[6].v3;
        col_out[6].v3 = col_out[6].fac * col_out[6].v2 - v1 + famp * *(col_out[6].win++);

        v1 = col_out[7].v2;
        col_out[7].v2 = col_out[7].v3;
        col_out[7].v3 = col_out[7].fac * col_out[7].v2 - v1 + famp * *(col_out[7].win++);

        /* Row result calculators */
        if (--row_out[0].samples_left == 0)
            row_energy[0] = goertzelResult(&row_out[0]);
        if (--row_out[1].samples_left == 0)
            row_energy[1] = goertzelResult(&row_out[1]);
        if (--row_out[2].samples_left == 0)
            row_energy[2] = goertzelResult(&row_out[2]);
        if (--row_out[3].samples_left == 0)
            row_energy[3] = goertzelResult(&row_out[3]);
        if (--row_out[4].samples_left == 0)
            row_energy[0] = goertzelResult(&row_out[4]);
        if (--row_out[5].samples_left == 0)
            row_energy[1] = goertzelResult(&row_out[5]);
        if (--row_out[6].samples_left == 0)
            row_energy[2] = goertzelResult(&row_out[6]);
        if (--row_out[7].samples_left == 0)
            row_energy[3] = goertzelResult(&row_out[7]);

        /* Column result calculators */
        if (--col_out[0].samples_left == 0)
            col_energy[0] = goertzelResult(&col_out[0]);
        if (--col_out[1].samples_left == 0)
            col_energy[1] = goertzelResult(&col_out[1]);
        if (--col_out[2].samples_left == 0)
            col_energy[2] = goertzelResult(&col_out[2]);
        if (--col_out[3].samples_left == 0)
            col_energy[3] = goertzelResult(&col_out[3]);
        if (--col_out[4].samples_left == 0)
            col_energy[0] = goertzelResult(&col_out[4]);
        if (--col_out[5].samples_left == 0)
            col_energy[1] = goertzelResult(&col_out[5]);
        if (--col_out[6].samples_left == 0)
            col_energy[2] = goertzelResult(&col_out[6]);
        if (--col_out[7].samples_left == 0)
            col_energy[3] = goertzelResult(&col_out[7]);
    
        /* Now we are at the end of the detection block */
        if (--samples_left == 0)
            dtmfReceive();
    }
    
    return len;
    
} /* Dh1dmSwDtmfDecoder::writeSamples */


/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/


/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/

void Dh1dmSwDtmfDecoder::dtmfReceive(void)
{
    const char dtmf_table[] = "123A456B789C*0#D";

    /* Find the peak row and the peak column */
    int best_row = findMaxIndex(row_energy);
    int best_col = findMaxIndex(col_energy);

    uint8_t hit = 0;
    /* Valid index test */
    if ((best_row >= 0) && (best_col >= 0))
    {
         /* Twist test */
        if ((col_energy[best_col] < row_energy[best_row] * reverse_twist) &&
            (row_energy[best_row] < col_energy[best_col] * normal_twist))
        {
            /* Got a hit */
            hit = dtmf_table[(best_row << 2) + best_col];
        }
    }

    /* Call the post-processing function. */
    dtmfPostProcess(hit);

    /* Reset the sample counter. */
    samples_left = DTMF_BLOCK_LENGTH;

} /* Dh1dmSwDtmfDecoder::dtmfReceive */


void Dh1dmSwDtmfDecoder::dtmfPostProcess(uint8_t hit)
{
  /* This function is called when a complete block has been received. */
  active_timer++; stable_timer++;
  
  /* The digit has not changed or is not stable. */
  if ((hit == last_stable) || (hit != last_hit))
  {
    stable_timer = 0;
  }

  /* Save the current digit. */
  last_hit = hit;

  /* A non-zero digit was stable for at least 50ms. */
  if (hit && (stable_timer >= 50))
  {
    if (last_stable) digitDeactivated(last_stable, active_timer);
    last_stable = hit;

    active_timer = 0;
    digitActivated(hit);
  }

  /* A zero digit was stable for at least 50ms. */
  if (!hit && (stable_timer >= 50 + hangtime()))
  {
    digitDeactivated(last_stable, active_timer);
    last_stable = 0;
  }
  
} /* Dh1dmSwDtmfDecoder::dtmfPostProcess */


void Dh1dmSwDtmfDecoder::goertzelInit(GoertzelState *s, float freq, float offset)
{
    /* Adjust the block length for 2.5% bandwidth. The real bandwidth will */
    /* be approx. 3% because we apply a Hamming window. */
    s->block_length = lrintf(40.0f * INTERNAL_SAMPLE_RATE / freq);
    /* Scale output values to achieve same levels at different block lengths. */
    s->scale_factor = 1.0e6f / (s->block_length * s->block_length);
    /* Init detector frequency. */
    s->fac = 2.0f * cosf(2.0f * M_PI * freq / INTERNAL_SAMPLE_RATE);
    /* Reset the tone detector state. */
    s->v2 = s->v3 = 0.0f;
    s->samples_left = static_cast<int>(s->block_length * (1.0f - offset));
    /* Hamming window */
    for (int i = 0; i < s->block_length; i++)
    {
        s->window_table.push_back(
           0.54 - 0.46 * cosf(2.0f * M_PI * i / (s->block_length - 1)));
    }
    /* Point to the first table entry */
    s->win = s->window_table.begin();

} /* Dh1dmSwDtmfDecoder::goertzelInit */


float Dh1dmSwDtmfDecoder::goertzelResult(GoertzelState *s)
{
    float v1, res;

    /* Push a zero through the process to finish things off. */
    v1 = s->v2;
    s->v2 = s->v3;
    s->v3 = s->fac*s->v2 - v1;
    /* Now calculate the non-recursive side of the filter. */
    /* The result here is not scaled down to allow for the magnification
       effect of the filter (the usual DFT magnification effect). */
    res = (s->v3*s->v3 + s->v2*s->v2 - s->v2*s->v3*s->fac) * s->scale_factor;
    /* Reset the tone detector state. */
    s->v2 = s->v3 = 0.0f;
    s->samples_left = s->block_length;
    s->win = s->window_table.begin();
    /* Return the calculated signal level. */
    return res;
    
} /* Dh1dmSwDtmfDecoder::goertzelResult */


int Dh1dmSwDtmfDecoder::findMaxIndex(const float f[])
{
    float threshold = 1.0f;
    int idx = -1;
    int i;

    /* Peak search */    
    for (i = 0; i < 4; i++)
    {
        if (f[i] > threshold)
        {
            threshold = f[i];
            idx = i;
        }
    }
    if (idx < 0)
        return -1;

    /* Peak test */   
    threshold *= 1.0f / DTMF_RELATIVE_PEAK;

    for (i = 0; i < 4; i++)
    {
        if (idx != i && f[i] > threshold)
            return -1;
    }
    return idx;
    
} /* Dh1dmSwDtmfDecoder::findMaxIndex */

/*- End of file ------------------------------------------------------------*/
