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


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncAudioFilter.h>
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
    min_det_cnt(DEFAULT_MIN_DET_CNT)
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

  AudioFilter *bpf = new AudioFilter("BpBu6/350-3500");
  setHandler(bpf);

  SigCAudioSink *sigc_sink = new SigCAudioSink;
  sigc_sink->sigWriteSamples.connect(
      mem_fun(*this, &NewSwDtmfDecoder::handleSamples));
  sigc_sink->sigFlushSamples.connect(
      mem_fun(sigc_sink, &SigCAudioSink::allSamplesFlushed));
  bpf->registerSink(sigc_sink, true);

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


int NewSwDtmfDecoder::handleSamples(const float *buf, int len)
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
} /* NewSwDtmfDecoder::handleSamples */


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
  bool debug = true;

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
  if (debug)
  {
    cout << setprecision(2) << fixed;
    cout << "### block_energy=" << setw(6) << block_energy;
  }

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
    float twist = max_row_ms / max_col_ms;
    if (debug)
    {
      cout << " rel_energy=" << setw(4) << rel_energy;
      cout << " twist=" << setw(5) << 10.0 * log10(twist) << "dB";
    }
    digit_active = (rel_energy > REL_THRESH) &&
                   (twist > twist_rev_thresh) &&
                   (twist < twist_nrm_thresh);
  }

  if (digit_active)
  {
    row[max_row].reset();
    col[max_col].reset();
    row[max_row].calc(block[0]);
    col[max_col].calc(block[0]);
    complex<double> prev_row_result = row[max_row].result();
    complex<double> prev_col_result = col[max_col].result();
    complex<double> row_sum = 0;
    complex<double> col_sum = 0;
    for (size_t i=1; i<BLOCK_SIZE; ++i)
    {
      row[max_row].calc(block[i]);
      col[max_col].calc(block[i]);

        // Caclulate row phase differense and accumulate
      complex<double> row_result = row[max_row].result();
      row_sum += row_result * conj(prev_row_result);
      prev_row_result = row_result;

        // Caclulate column phase differense and accumulate
      complex<double> col_result = col[max_col].result();
      col_sum += col_result * conj(prev_col_result);
      prev_col_result = col_result;
    }
    float row_fq = INTERNAL_SAMPLE_RATE * arg(row_sum) / (2 * M_PI);
    float col_fq = INTERNAL_SAMPLE_RATE * arg(col_sum) / (2 * M_PI);
    float row_fqdiff = 2.0 * (row_fq - row[max_row].m_freq);
    float col_fqdiff = 2.0 * (col_fq - col[max_col].m_freq);
    if (debug)
    {
      cout << " row_fqdiff=" << row_fqdiff
           << " (" << (100.0 * row_fqdiff / row[max_row].m_freq) << "%)";
      cout << " col_fqdiff=" << col_fqdiff
           << " (" << (100.0 * col_fqdiff / col[max_col].m_freq) << "%)";

      digit_active = (abs(row_fqdiff) < row[max_row].m_max_fqdiff) &&
                     (abs(col_fqdiff) < col[max_col].m_max_fqdiff);
    }
  }

  if (digit_active)
  {
    char digit = digit_map[max_row][max_col];
    if (debug)
    {
      cout << " digit=" << digit;
    }
    if (digit != last_digit_active)
    {
      det_cnt = 0;
    }
    last_digit_active = digit;
    if (++det_cnt == min_det_cnt)
    {
      if (debug)
      {
        cout << " activated";
      }
      digitActivated(last_digit_active);
    }
    else if (det_cnt > min_det_cnt)
    {
      det_cnt += undet_cnt;
    }
    undet_cnt = 0;
  }
  else if (det_cnt > 0)
  {
    if (++undet_cnt > 1)
    {
      if (det_cnt >= min_det_cnt)
      {
        const int block_time = 1000 * BLOCK_SIZE / INTERNAL_SAMPLE_RATE;
        const int duration = block_time * det_cnt;
        if (debug)
        {
          cout << " deactivated=" << last_digit_active;
          cout << " duration=" << duration;
        }
        digitDeactivated(last_digit_active, duration);
      }
      det_cnt = 0;
      undet_cnt = 0;
      last_digit_active = 0;
    }
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
