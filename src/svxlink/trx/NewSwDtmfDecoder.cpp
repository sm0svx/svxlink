/**
@file	 NewSwDtmfDecoder.cpp
@brief   This file contains a class that implements a sw DTMF decoder
@author  Tobias Blomberg / SM0SVX
@date	 2015-02-22

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2004-2015  Tobias Blomberg / SM0SVX

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
#include <iomanip>
#include <cmath>
#include <cstring>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncSigCAudioSink.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "NewSwDtmfDecoder.h"



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

namespace {
  static const char digit_map[4][4] =
  {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
  };
};


/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/

NewSwDtmfDecoder::NewSwDtmfDecoder(Config &cfg, const string &name)
  : DtmfDecoder(cfg, name), twist_nrm_thresh(0), twist_rev_thresh(0), row(4),
    col(4), block_pos(0), det_cnt(0), undet_cnt(0), last_digit_active(0),
    min_det_cnt(DEFAULT_MIN_DET_CNT), min_undet_cnt(DEFAULT_MIN_UNDET_CNT),
    det_state(STATE_IDLE), det_cnt_weight(0)
{
  twist_nrm_thresh = powf(10.0f, DEFAULT_MAX_NORMAL_TWIST_DB / 10.0f);
  twist_rev_thresh = powf(10.0f, -DEFAULT_MAX_REV_TWIST_DB / 10.0f);

    // Row detectors
  row[0].initialize(697);
  row[1].initialize(770);
  row[2].initialize(852);
  row[3].initialize(941);

    // Column detectors
  col[0].initialize(1209);
  col[1].initialize(1336);
  col[2].initialize(1477);
  col[3].initialize(1633);
} /* NewSwDtmfDecoder::NewSwDtmfDecoder */


bool NewSwDtmfDecoder::initialize(void)
{
  if (!DtmfDecoder::initialize())
  {
    return false;
  }
  
  float cfg_max_normal_twist = -1.0f;
  if (cfg().getValue(name(), "DTMF_MAX_FWD_TWIST", cfg_max_normal_twist))
  {
    if (cfg_max_normal_twist > 0.0f)
    {
      twist_nrm_thresh = powf(10.0f, cfg_max_normal_twist / 10.0f);
    }
  }
  
  float cfg_max_rev_twist = -1.0f;
  if (cfg().getValue(name(), "DTMF_MAX_REV_TWIST", cfg_max_rev_twist))
  {
    if (cfg_max_rev_twist >= 0.0f)
    {
      twist_rev_thresh = powf(10.0f, -cfg_max_rev_twist / 10.0f);
    }
  }
  
  return true;
  
} /* NewSwDtmfDecoder::initialize */


int NewSwDtmfDecoder::writeSamples(const float *buf, int len)
{
  for (int i = 0; i < len; i++)
  {
    block[block_pos] = buf[i];
    if (++block_pos >= BLOCK_SIZE)
    {
      processBlock();
      if (OVERLAP > 0)
      {
        memmove(block, block + (BLOCK_SIZE - OVERLAP), OVERLAP * sizeof(*buf));
      }
      block_pos = OVERLAP;
    }
  }

  return len;
} /* NewSwDtmfDecoder::writeSamples */


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

void NewSwDtmfDecoder::processBlock(void)
{
  bool debug = false;

    // Reset all Goertzel objects
  row[0].reset();
  row[1].reset();
  row[2].reset();
  row[3].reset();
  col[0].reset();
  col[1].reset();
  col[2].reset();
  col[3].reset();

    // Calculate total block energy and energy for all Goertzel detectors
  double block_energy = 0.0;
  for (size_t i=0; i<BLOCK_SIZE; ++i)
  {
    row[0].calc(block[i]);
    row[1].calc(block[i]);
    row[2].calc(block[i]);
    row[3].calc(block[i]);
    col[0].calc(block[i]);
    col[1].calc(block[i]);
    col[2].calc(block[i]);
    col[3].calc(block[i]);
    block_energy += block[i] * block[i];
  }
  if (debug)
  {
    cout << setprecision(2) << fixed;
    cout << "### block_energy=" << setw(6) << block_energy;
  }

  bool digit_active = false;
  size_t max_row_idx = 0;
  size_t max_col_idx = 0;
  if (block_energy > ENERGY_THRESH)
  {
    float rel_energy = 0.0f;
    float max_row_ms = 0.0f;
    float max_col_ms = 0.0f;
    for (size_t i = 0; i < 4; ++i)
    {
      const float row_ms = row[i].magnitudeSquared();
      if (row_ms > max_row_ms)
      {
        max_row_ms = row_ms;
        max_row_idx = i;
      }

      const float col_ms = col[i].magnitudeSquared();
      if (col_ms > max_col_ms)
      {
        max_col_ms = col_ms;
        max_col_idx = i;
      }
    }

    rel_energy = 2 * (max_row_ms + max_col_ms) / (BLOCK_SIZE * block_energy);
    //cout << " row=" << max_row_idx << " col=" << max_col_idx;
    const float twist = max_row_ms / max_col_ms;
    if (debug)
    {
      cout << " rel_energy=" << setw(4) << rel_energy;
      cout << " twist=" << setw(5) << 10.0 * log10(twist) << "dB";
    }
    if (rel_energy > REL_THRESH_HI)
    {
      det_cnt_weight = DET_CNT_HI_WEIGHT;
      digit_active = (twist > twist_rev_thresh) && (twist < twist_nrm_thresh);
    }
    else if (rel_energy > REL_THRESH_LO)
    {
      det_cnt_weight = 1;
      digit_active = (twist > twist_rev_thresh) && (twist < twist_nrm_thresh);
    }
    else
    {
      digit_active = false;
    }
  }

  char digit = 0;
  if (digit_active)
  {
    digit = digit_map[max_row_idx][max_col_idx];
    if (debug)
    {
      cout << " digit=" << digit;
    }
    digit_active = ((det_state == STATE_IDLE) || (digit == last_digit_active));
    last_digit_active = digit;
  }

  if (digit_active)
  {
    DtmfGoertzel &max_row = row[max_row_idx];
    DtmfGoertzel &max_col = col[max_col_idx];
    max_row.reset();
    max_col.reset();
    max_row.calc(block[0]);
    max_col.calc(block[0]);
    complex<double> prev_row_result = max_row.result();
    complex<double> prev_col_result = max_col.result();
    complex<double> row_sum = 0;
    complex<double> col_sum = 0;
    size_t samp_cnt = 0;
    for (size_t i=1; i<BLOCK_SIZE; ++i)
    {
      max_row.calc(block[i]);
      max_col.calc(block[i]);

      if (++samp_cnt >= 4)
      {
        samp_cnt = 0;

          // Caclulate row phase differense and accumulate
        complex<double> row_result = max_row.result();
        row_sum += row_result * conj(prev_row_result);
        prev_row_result = row_result;

          // Caclulate column phase differense and accumulate
        complex<double> col_result = max_col.result();
        col_sum += col_result * conj(prev_col_result);
        prev_col_result = col_result;
      }
    }
    float row_fq = INTERNAL_SAMPLE_RATE * arg(row_sum) / (8.0 * M_PI);
    float col_fq = INTERNAL_SAMPLE_RATE * arg(col_sum) / (8.0 * M_PI);
    float row_fqdiff = 2.0 * (row_fq - max_row.m_freq);
    float col_fqdiff = 2.0 * (col_fq - max_col.m_freq);
    if (debug)
    {
      cout << " row_fqdiff=" << row_fqdiff
           << " (" << (100.0 * row_fqdiff / max_row.m_freq) << "%)";
      cout << " col_fqdiff=" << col_fqdiff
           << " (" << (100.0 * col_fqdiff / max_col.m_freq) << "%)";

      digit_active = (abs(row_fqdiff) < max_row.m_max_fqdiff) &&
                     (abs(col_fqdiff) < max_col.m_max_fqdiff);
    }
  }

  switch (det_state)
  {
    case STATE_IDLE:
      if (digit_active)
      {
        det_cnt = det_cnt_weight;
        undet_cnt = 0;
        duration = 1;
        det_state = STATE_DET_DELAY;
      }
      break;

    case STATE_DET_DELAY:
      duration += 1;
      if (digit_active)
      {
        undet_cnt = 0;
        det_cnt += det_cnt_weight;
        if (det_cnt >= min_det_cnt)
        {
          if (debug)
          {
            cout << " activated";
          }
          digitActivated(last_digit_active);
          det_state = STATE_DETECTED;
        }
      }
      else
      {
        if ((det_cnt_weight > 1) || (++undet_cnt >= 2))
        {
          undet_cnt = 0;
          det_state = STATE_IDLE;
        }
      }
      break;

    case STATE_DETECTED:
      if (digit_active)
      {
        if (undet_cnt > 0)
        {
          duration += undet_cnt;
          undet_cnt = 0;
        }
        else
        {
          duration += 1;
        }
      }
      else
      {
        if (++undet_cnt >= min_undet_cnt)
        {
          const int first_block_time = 1000 * BLOCK_SIZE / INTERNAL_SAMPLE_RATE;
          const int block_time =
              1000 * (BLOCK_SIZE-OVERLAP) / INTERNAL_SAMPLE_RATE;
          const int dur_ms = first_block_time + block_time * (duration - 1);
          if (debug)
          {
            cout << " deactivated=" << last_digit_active;
            cout << " duration=" << dur_ms;
          }
          digitDeactivated(last_digit_active, dur_ms);
          det_state = STATE_IDLE;
        }
      }
      break;
  }

  if (debug)
  {
    cout << " " << (digit_active ? "*" : "");
    cout << endl;
  }
} /* NewSwDtmfDecoder::processBlock */


float NewSwDtmfDecoder::phaseDiffToFq(float phase, float prev_phase)
{
  float diff = phase - prev_phase;
  if (diff > M_PI)
  {
    diff -= 2.0f * M_PI;
  }
  else if (diff < -M_PI)
  {
    diff += 2.0f * M_PI;
  }
  return INTERNAL_SAMPLE_RATE * diff / (2.0f * M_PI);
} /* NewSwDtmfDecoder::phaseDiffToFq */


void NewSwDtmfDecoder::DtmfGoertzel::initialize(float freq)
{
  Goertzel::initialize(freq, INTERNAL_SAMPLE_RATE);
  m_freq = freq;
  m_max_fqdiff = m_freq * MAX_FQ_ERROR;
}


/*
 * This file has not been truncated
 */
