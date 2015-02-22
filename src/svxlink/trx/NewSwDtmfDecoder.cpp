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
#include <cmath>


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
  : DtmfDecoder(cfg, name), max_normal_twist(DEFAULT_MAX_NORMAL_TWIST),
    max_reverse_twist(DEFAULT_MAX_REV_TWIST), row(4), col(4), block_pos(0),
    det_cnt(0), undet_cnt(0), last_digit_detected(0)
{
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
  
  int cfg_max_normal_twist;
  if (cfg().getValue(name(), "DTMF_MAX_FWD_TWIST", cfg_max_normal_twist))
  {
    if (cfg_max_normal_twist >= 0)
    {
      max_normal_twist = powf(10, cfg_max_normal_twist / 10.0f);
    }
  }
  
  int cfg_max_rev_twist;
  if (cfg().getValue(name(), "DTMF_MAX_REV_TWIST", cfg_max_rev_twist))
  {
    if (cfg_max_rev_twist >= 0)
      max_reverse_twist = powf(10, cfg_max_rev_twist / 10.0f);
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
        block_pos = 0;
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
  for (int i=0; i<4; ++i)
  {
    row[i].reset();
    col[i].reset();
  }

  double block_energy = 0.0;
  for (size_t i=0; i<BLOCK_SIZE; ++i)
  {
    for (int di=0; di<4; ++di)
    {
      row[di].calc(block[i]);
      col[di].calc(block[i]);
    }
    block_energy += block[i] * block[i];
  }
  cout << "### block_energy=" << block_energy;

  bool digit_active = false;
  float rel_energy = 0.0f;
  size_t max_row = 0;
  size_t max_col = 0;
  if (block_energy > ENERGY_THRESH)
  {
    float max_row_ms = 0.0f;
    float max_col_ms = 0.0f;
    for (size_t i = 0; i < 4; ++i)
    {
      float row_ms = row[i].magnitudeSquared();
      if (row_ms > max_row_ms)
      {
        max_row_ms = row_ms;
        max_row = i;
      }

      float col_ms = col[i].magnitudeSquared();
      if (col_ms > max_col_ms)
      {
        max_col_ms = col_ms;
        max_col = i;
      }
    }

    rel_energy = 2 * (max_row_ms + max_col_ms) / (BLOCK_SIZE * block_energy);
    //cout << " row=" << max_row << " col=" << max_col;
    digit_active = (rel_energy > REL_THRESH);
  }

  //cout << " rel_energy=" << rel_energy;

  if (digit_active)
  {
    row[max_row].reset();
    col[max_col].reset();
    double row_fq_sum = 0.0;
    double col_fq_sum = 0.0;
    row[max_row].calc(block[0]);
    col[max_col].calc(block[0]);
    float prev_row_phase = row[max_row].phase();
    float prev_col_phase = col[max_col].phase();
    int row_period_block_pos = 0;
    int col_period_block_pos = 0;
    int row_period_cnt = 0;
    int col_period_cnt = 0;
    for (size_t i=1; i<BLOCK_SIZE; ++i)
    {
      row[max_row].calc(block[i]);
      col[max_col].calc(block[i]);

      if (++row_period_block_pos >= row[max_row].m_period_block_len)
      {
        row_period_block_pos = 0;
        float row_phase = row[max_row].phase();
        row_fq_sum += phaseDiffToFq(row_phase, prev_row_phase);
        prev_row_phase = row_phase;
        ++row_period_cnt;
      }

      if (++col_period_block_pos >= col[max_col].m_period_block_len)
      {
        col_period_block_pos = 0;
        float col_phase = col[max_col].phase();
        col_fq_sum += phaseDiffToFq(col_phase, prev_col_phase);
        prev_col_phase = col_phase;
        ++col_period_cnt;
      }
    }
    //float row_fqdiff = (row_fq_sum / (BLOCK_SIZE-1)) - row[max_row].m_freq;
    //float col_fqdiff = (col_fq_sum / (BLOCK_SIZE-1)) - col[max_col].m_freq;
    float row_fqdiff = (row_fq_sum / row_period_cnt);
    float col_fqdiff = (col_fq_sum / col_period_cnt);
    cout << " row_fqdiff=" << row_fqdiff;
    cout << " col_fqdiff=" << col_fqdiff;
  }

  if (digit_active)
  {
    if (++det_cnt == 3)
    {
      last_digit_detected = digit_map[max_row][max_col];
      //cout << " digit=" << last_digit_detected;
      digitActivated(last_digit_detected);
    }
  }
  else if (det_cnt > 0)
  {
    if (++undet_cnt > 1)
    {
      if (det_cnt > 2)
      {
        digitDeactivated(last_digit_detected, 10 * det_cnt);
      }
      det_cnt = 0;
      undet_cnt = 0;
      last_digit_detected = 0;
    }
  }

  cout << endl;

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
  m_period_block_len = static_cast<int>(ceilf(INTERNAL_SAMPLE_RATE / freq));
  float actual_fq = INTERNAL_SAMPLE_RATE / m_period_block_len;
  cout << "### actual_fq=" << actual_fq << endl;
  m_freq = actual_fq;
  m_phase_offset = 2.0f * (freq - actual_fq) * (M_PI / actual_fq);
}

