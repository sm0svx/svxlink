/**
@file	 SwDtmfDecoder.cpp
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

#include "SwDtmfDecoder.h"



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

// All values are squared magnitude ratios !
#define DTMF_NORMAL_TWIST           6.3f   /* 8dB */
#define DTMF_REVERSE_TWIST          6.3f   /* 8dB */
#define DTMF_RELATIVE_PEAK          10.0f  /* 10dB */

// The Goertzel algorithm is just a recursive way to evaluate the DFT at a
// single frequency. The DTMF detection bandwidth of 39Hz was chosen to
// place the DTMF frequencies near the centers of the DFT's.
// This is generally an implementation trade-off, since the DFT's binary
// frequencies are arithmetically spaced, whereas DTMF's frequencies are
// geometrically spaced.
#define DTMF_BANDWIDTH              39     /* 39Hz */

#define DTMF_BLOCK_SAMPLES          (INTERNAL_SAMPLE_RATE / DTMF_BANDWIDTH)
#define DTMF_BLOCK_TIME             (1000000 / DTMF_BANDWIDTH)


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

SwDtmfDecoder::SwDtmfDecoder(Config &cfg, const string &name)
  : DtmfDecoder(cfg, name), last_hit(0), last_stable(0),
    stable_timer(0), active_timer(0),
    normal_twist(DTMF_NORMAL_TWIST), reverse_twist(DTMF_REVERSE_TWIST)
{
    /* Init row detectors */
    goertzelInit(&detector[0].row_out[0], 697.0f, INTERNAL_SAMPLE_RATE);
    goertzelInit(&detector[0].row_out[1], 770.0f, INTERNAL_SAMPLE_RATE);
    goertzelInit(&detector[0].row_out[2], 852.0f, INTERNAL_SAMPLE_RATE);
    goertzelInit(&detector[0].row_out[3], 941.0f, INTERNAL_SAMPLE_RATE);

    /* Init column detectors */
    goertzelInit(&detector[0].col_out[0], 1209.0f, INTERNAL_SAMPLE_RATE);
    goertzelInit(&detector[0].col_out[1], 1336.0f, INTERNAL_SAMPLE_RATE);
    goertzelInit(&detector[0].col_out[2], 1477.0f, INTERNAL_SAMPLE_RATE);
    goertzelInit(&detector[0].col_out[3], 1633.0f, INTERNAL_SAMPLE_RATE);

    memcpy(&detector[1], &detector[0], sizeof(DtmfState));

    /* Sliding window Goertzel algorithm */
    detector[0].current_sample = 0;
    detector[1].current_sample = DTMF_BLOCK_SAMPLES / 2;

} /* SwDtmfDecoder::SwDtmfDecoder */


bool SwDtmfDecoder::initialize(void)
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
      normal_twist = exp10f(cfg_fwd_twist/10.0f);
  }
  
  if (cfg().getValue(name(), "DTMF_MAX_REV_TWIST", value))
  {
    int cfg_rev_twist = atoi(value.c_str());
    if (cfg_rev_twist >= 0)
      reverse_twist = exp10f(cfg_rev_twist/10.0f);
  }
  
  return true;
  
} /* SwDtmfDecoder::initialize */


int SwDtmfDecoder::writeSamples(const float *buf, int len)
{
  int ret = len;
  
  while (len > 0)
  {
    int block_len = min(len, DTMF_BLOCK_SAMPLES);

    /* Sliding window Goertzel algorithm */
    if (detector[0].current_sample > detector[1].current_sample)
    {
      dtmfReceive(&detector[0], buf, block_len);
      dtmfReceive(&detector[1], buf, block_len);
    }
    else
    {
      dtmfReceive(&detector[1], buf, block_len);
      dtmfReceive(&detector[0], buf, block_len);
    }

    buf += block_len; len -= block_len;
  }
  
  return ret;
  
} /* SwDtmfDecoder::writeSamples */


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

void SwDtmfDecoder::dtmfReceive(DtmfState *d, const float *buf, int len)
{
    const char dtmf_table[] = "123A456B789C*0#D";
    float row_energy[4];
    float col_energy[4];
    float famp;
    float v1;
    int best_row;
    int best_col;

    for (int i = 0; i < len; i++)
    {
        famp = *(buf++);

        /* Row detectors */
        v1 = d->row_out[0].v2;
        d->row_out[0].v2 = d->row_out[0].v3;
        d->row_out[0].v3 = d->row_out[0].fac*d->row_out[0].v2 - v1 + famp;

        v1 = d->row_out[1].v2;
        d->row_out[1].v2 = d->row_out[1].v3;
        d->row_out[1].v3 = d->row_out[1].fac*d->row_out[1].v2 - v1 + famp;
        
        v1 = d->row_out[2].v2;
        d->row_out[2].v2 = d->row_out[2].v3;
        d->row_out[2].v3 = d->row_out[2].fac*d->row_out[2].v2 - v1 + famp;

        v1 = d->row_out[3].v2;
        d->row_out[3].v2 = d->row_out[3].v3;
        d->row_out[3].v3 = d->row_out[3].fac*d->row_out[3].v2 - v1 + famp;

        /* Column detectors */
        v1 = d->col_out[0].v2;
        d->col_out[0].v2 = d->col_out[0].v3;
        d->col_out[0].v3 = d->col_out[0].fac*d->col_out[0].v2 - v1 + famp;

        v1 = d->col_out[1].v2;
        d->col_out[1].v2 = d->col_out[1].v3;
        d->col_out[1].v3 = d->col_out[1].fac*d->col_out[1].v2 - v1 + famp;

        v1 = d->col_out[2].v2;
        d->col_out[2].v2 = d->col_out[2].v3;
        d->col_out[2].v3 = d->col_out[2].fac*d->col_out[2].v2 - v1 + famp;

        v1 = d->col_out[3].v2;
        d->col_out[3].v2 = d->col_out[3].v3;
        d->col_out[3].v3 = d->col_out[3].fac*d->col_out[3].v2 - v1 + famp;

        if (++d->current_sample < DTMF_BLOCK_SAMPLES)
            continue;

        /* We are at the end of a DTMF detection block */
        row_energy[0] = goertzelResult(&d->row_out[0]);
        row_energy[1] = goertzelResult(&d->row_out[1]);
        row_energy[2] = goertzelResult(&d->row_out[2]);
        row_energy[3] = goertzelResult(&d->row_out[3]);

        col_energy[0] = goertzelResult(&d->col_out[0]);
        col_energy[1] = goertzelResult(&d->col_out[1]);
        col_energy[2] = goertzelResult(&d->col_out[2]);
        col_energy[3] = goertzelResult(&d->col_out[3]);

        /* Find the peak row and the peak column */
        best_row = findMaxIndex(row_energy);
        best_col = findMaxIndex(col_energy);

        uint8_t hit = 0;
        /* Valid index test */
        if ((best_row >= 0) && (best_col >= 0))
        {
            /* Twist test */
            if ((col_energy[best_col] < row_energy[best_row] * reverse_twist) &&
                (col_energy[best_col] * normal_twist > row_energy[best_row]))
            {
                /* Got a hit */
                hit = dtmf_table[(best_row << 2) + best_col];
            }
        }

        /* Call the post-processing function. */
        dtmfPostProcess(hit);

        /* Reinitialise the detectors for the next block */
        goertzelReset(&d->row_out[0]);
        goertzelReset(&d->row_out[1]);            
        goertzelReset(&d->row_out[2]);
        goertzelReset(&d->row_out[3]);
        
        goertzelReset(&d->col_out[0]);
        goertzelReset(&d->col_out[1]);
        goertzelReset(&d->col_out[2]);
        goertzelReset(&d->col_out[3]);

        d->current_sample = 0;
    }
    
} /* SwDtmfDecoder::dtmfReceive */


void SwDtmfDecoder::dtmfPostProcess(uint8_t hit)
{
  /* This function is called when a complete block has been received. */
  /* Since we use a sliding window algorithm, this happens twice in */
  /* every DTMF_BLOCK_TIME interval. */
  active_timer += DTMF_BLOCK_TIME / 2;
  
  /* The digit has changed. */
  if (hit != last_stable)
  {
    /* Here we check if the digit is stable. */
    if (hit != last_hit)
      stable_timer = DTMF_BLOCK_TIME / 2;
    else
      stable_timer += DTMF_BLOCK_TIME / 2;
  }
  /* We still receive the same digit or space. */
  else
  {
    stable_timer = 0;
  }

  /* Save the current digit. */
  last_hit = hit;

  /* A non-zero digit was stable for at least 50ms. */
  if (hit && (stable_timer >= 50000))
  {
    if (last_stable) digitDeactivated(last_stable, active_timer / 1000);
    last_stable = hit;

    active_timer = 0;
    digitActivated(hit);
  }

  /* A zero digit was stable for at least 50ms. */
  if (!hit && (stable_timer >= 50000 + hangtime() * 1000))
  {
    digitDeactivated(last_stable, active_timer / 1000);
    last_stable = 0;
  }
  
} /* SwDtmfDecoder::dtmfPostProcess */


void SwDtmfDecoder::goertzelInit(GoertzelState *s, float freq, int sample_rate)
{
    s->fac = 2.0f * cosf(2.0f * M_PI * (freq / (float)sample_rate));
    goertzelReset(s);
    
} /* SwDtmfDecoder::goertzelInit */


float SwDtmfDecoder::goertzelResult(GoertzelState *s)
{
    float v1;

    /* Push a zero through the process to finish things off. */
    v1 = s->v2;
    s->v2 = s->v3;
    s->v3 = s->fac*s->v2 - v1;
    /* Now calculate the non-recursive side of the filter. */
    /* The result here is not scaled down to allow for the magnification
       effect of the filter (the usual DFT magnification effect). */
    return s->v3*s->v3 + s->v2*s->v2 - s->v2*s->v3*s->fac;
    
} /* SwDtmfDecoder::goertzelResult */


int SwDtmfDecoder::findMaxIndex(const float f[])
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
    
} /* SwDtmfDecoder::findMaxIndex */

/*- End of file ------------------------------------------------------------*/
