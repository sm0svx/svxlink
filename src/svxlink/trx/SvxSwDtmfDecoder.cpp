/**
@file	 SvxSwDtmfDecoder.cpp
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

#include "SvxSwDtmfDecoder.h"



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

  static const float row_fqs[] = { 697, 770, 852, 941 };
  static const float col_fqs[] = { 1209, 1336, 1477, 1633 };
};


/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/

SvxSwDtmfDecoder::SvxSwDtmfDecoder(Config &cfg, const string &name)
  : DtmfDecoder(cfg, name), twist_nrm_thresh(0), twist_rev_thresh(0),
    row(8), col(8), block_size(0), block_pos(0), det_cnt(0), undet_cnt(0),
    last_digit_active(0), min_det_cnt(DEFAULT_MIN_DET_CNT),
    min_undet_cnt(DEFAULT_MIN_UNDET_CNT), det_state(STATE_IDLE),
    det_cnt_weight(0), duration(0), undet_thresh(0), debug(false),
    win_pwr_comp(0.0f)
{
  twist_nrm_thresh = powf(10.0f, DEFAULT_MAX_NORMAL_TWIST_DB / 10.0f);
  twist_rev_thresh = powf(10.0f, -(DEFAULT_MAX_REV_TWIST_DB / 10.0f));

    // Row detectors
  for (size_t i=0; i<4; ++i)
  {
    row[i].initialize(row_fqs[i]);
    row[i+4].initialize(3.0f * row_fqs[i]); // Third overtone
  }

    // Column detectors
  for (size_t i=0; i<4; ++i)
  {
    col[i].initialize(col_fqs[i]);
    col[i+4].initialize(3.0f * col_fqs[i]); // Third overtone
  }

    // Initialize window function
  for (size_t n=0; n<BLOCK_SIZE; ++n)
  {
      // Hamming window
    win[n] = 0.53836 - 0.46164 * cosf(2.0f * M_PI * n / (BLOCK_SIZE - 1));
      // Hann window
    //win[n] = 0.5 - 0.5 * cosf(2.0f * M_PI * n / (BLOCK_SIZE - 1));
      // Rectangular window
    //win[n] = 1.0f;
    win_pwr_comp += win[n] * win[n];
  }
  win_pwr_comp /= BLOCK_SIZE;
  win_pwr_comp = 1.0f / win_pwr_comp;
} /* SvxSwDtmfDecoder::SvxSwDtmfDecoder */


bool SvxSwDtmfDecoder::initialize(void)
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

  if (hangtime() > 0)
  {
    const size_t block_size_ms = 1000 * BLOCK_SIZE / INTERNAL_SAMPLE_RATE;
    const size_t step_size_ms = 1000 * STEP_SIZE / INTERNAL_SAMPLE_RATE;
    min_undet_cnt = 1;
    if (hangtime() > block_size_ms)
    {
      min_undet_cnt = 1 + (hangtime() - block_size_ms) / step_size_ms;
    }
  }
  
  cfg().getValue(name(), "DTMF_DEBUG", debug);
  
  return true;
  
} /* SvxSwDtmfDecoder::initialize */


int SvxSwDtmfDecoder::writeSamples(const float *buf, int len)
{
  for (int i = 0; i < len; i++)
  {
    block[block_pos] = buf[i];
    if (++block_pos >= BLOCK_SIZE)
    {
      processBlock();
      if (STEP_SIZE < BLOCK_SIZE)
      {
        memmove(block, block + STEP_SIZE,
                (BLOCK_SIZE - STEP_SIZE) * sizeof(*buf));
      }
      block_pos = BLOCK_SIZE - STEP_SIZE;
    }
  }

  return len;
} /* SvxSwDtmfDecoder::writeSamples */


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

void SvxSwDtmfDecoder::processBlock(void)
{
    // Reset all Goertzel objects
  row[0].reset();
  row[1].reset();
  row[2].reset();
  row[3].reset();
  col[0].reset();
  col[1].reset();
  col[2].reset();
  col[3].reset();

    // Calculate the total block energy and energy for all individual
    // Goertzel detectors over the block
  double block_energy = 0.0;
  for (size_t i=0; i<BLOCK_SIZE; ++i)
  {
    float sample = block[i] * win[i];
    block_energy += static_cast<double>(sample) * sample;
    row[0].calc(sample);
    row[1].calc(sample);
    row[2].calc(sample);
    row[3].calc(sample);
    col[0].calc(sample);
    col[1].calc(sample);
    col[2].calc(sample);
    col[3].calc(sample);
  }
  ios_base::fmtflags orig_cout_flags(cout.flags());
  if (debug)
  {
    cout << setprecision(2) << fixed;
    cout << "### pwr=" << setw(6) 
         << 10.0f * log10f(2 * win_pwr_comp * block_energy / BLOCK_SIZE)
         << "dB";
  }

    // If the total block energy is over the threshold, find the strongest
    // tone in the low group (row) and the high group (column).
    // Calculate the relation between the energy in the two tones to the
    // energy in the whole passband. If more than 85% of the energy is in the
    // tones, we class this as a strong signal. If more than 50% of the energy
    // is in the tones, we consider this a weak signal and the required time
    // for detection is increased so as to suppress false positives. Below
    // 50% is considered as not active.
    // Also test the two strongest tones for twist, the amplitude difference
    // between the two tones must not exceed the limits (configurable) which
    // typically are +/- 8dB.
  bool digit_active = false;
  size_t max_row_idx = 0;
  size_t max_col_idx = 0;
  float max_row_ms = 0.0f;
  float max_col_ms = 0.0f;
  if (block_energy > ENERGY_THRESH)
  {
    float rel_energy = 0.0f;
    float row_sum = 0.0f;
    float col_sum = 0.0f;
    for (size_t i = 0; i < 4; ++i)
    {
      const float row_ms = WIN_ENB * row[i].magnitudeSquared();
      if (row_ms > max_row_ms)
      {
        max_row_ms = row_ms;
        max_row_idx = i;
      }
      row_sum += row_ms;

      const float col_ms = WIN_ENB * col[i].magnitudeSquared();
      if (col_ms > max_col_ms)
      {
        max_col_ms = col_ms;
        max_col_idx = i;
      }
      col_sum += col_ms;
    }

    rel_energy = 2 * (max_row_ms + max_col_ms) / (BLOCK_SIZE * block_energy);
    //cout << " row=" << max_row_idx << " col=" << max_col_idx;
    const float twist = max_row_ms / max_col_ms;
    const float row_group_rel = max_row_ms / row_sum;
    const float col_group_rel = max_col_ms / col_sum;
    if (debug)
    {
      cout << " q=" << setw(4) << rel_energy;
      cout << " twist=" << setw(5) << 10.0 * log10(twist) << "dB";
      cout << " rowq=" << row_group_rel;
      cout << " colq=" << col_group_rel;
    }
    digit_active = false;
    if (rel_energy > REL_THRESH_LO)
    {
      if (rel_energy > REL_THRESH_HI)
      {
        det_cnt_weight = DET_CNT_HI_WEIGHT;
      }
      else if (rel_energy > REL_THRESH_MED)
      {
        det_cnt_weight = DET_CNT_MED_WEIGHT;
      }
      else
      {
        det_cnt_weight = DET_CNT_LO_WEIGHT;
      }
      digit_active = (twist > twist_rev_thresh) &&
                     (twist < twist_nrm_thresh) &&
                     (row_group_rel > 0.80) &&
                     (col_group_rel > 0.80);
    }
  }
  DtmfGoertzel &max_row = row[max_row_idx];
  DtmfGoertzel &max_col = col[max_col_idx];

    // Find out what digit corresponds to the two strongest tones.
    // If the digit changed from the previous detection without a proper pause
    // we consider this detection bogus.
  char digit = 0;
  if (digit_active)
  {
    digit = digit_map[max_row_idx][max_col_idx];
    if (debug)
    {
      cout << " digit=" << digit;
    }
    if ((det_state != STATE_IDLE) && (digit != last_digit_active))
    {
      digit_active = false;
    }
    else
    {
      last_digit_active = digit;
    }
  }

    // Calculate overtone DFT:s and intermodulation DFT.
    // The third overtone for the selected tones in each tone group is
    // calculated and compared to a threshold. If the overtone is too high,
    // the received tone is not a pure sine.
    // Intermodulation between the two selected tones can also be a sign of
    // that this is not a DTMF digit.
  if (digit_active)
  {
    Goertzel im(max_col.m_freq + max_col.m_freq - max_row.m_freq,
                INTERNAL_SAMPLE_RATE);
    row[max_row_idx+4].reset();
    col[max_col_idx+4].reset();
    for (size_t i=0; i<BLOCK_SIZE; ++i)
    {
      float sample = block[i] * win[i];
      im.calc(sample);
      row[max_row_idx+4].calc(sample);
      col[max_col_idx+4].calc(sample);
    }

    float row_ot_rel = row[max_row_idx+4].magnitudeSquared() / max_row_ms;
    float col_ot_rel = col[max_col_idx+4].magnitudeSquared() / max_col_ms;
    float im_rel = im.magnitudeSquared() / (max_row_ms + max_col_ms);
    if (debug)
    {
      cout << " row3rd=" << row_ot_rel;
      cout << " col3rd=" << col_ot_rel;
      cout << " im=" << im_rel;
    }

    digit_active = (row_ot_rel < MAX_OT_REL) && (col_ot_rel < MAX_OT_REL) &&
                   (im_rel < MAX_IM_REL);
  }

#if 0
    // Since we use the same detection bandwidth for all eight tones, most
    // detectors actually allow a tone frequency deviation that is outside of
    // the DTMF spec. To find out what the frequency error is, we one more time
    // run through the Goertzel for the two strongest tones. The difference is
    // that this time we also calculate the phase for multiple points during
    // the block. We can then use the mean value of the phase differences to
    // estimate the frequency errors for the tones.
    // After the frequency error have been calculated, it's easy to make sure
    // that both tones lie within the tolerance limit which is about a maximum
    // of 3% frequency deviation.
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
#endif

    // Using the result from the calculations above, either digit active or
    // not active for this block, we use a state machine to determine if a DTMF
    // digit detection should be reported or not.
    // STATE_IDLE: We wait for the first active block. When found, reset all
    // variables and move to the next state.
    // STATE_DET_DELAY: There need to be a number of blocks in a row where the
    // same digit is considered active. If the signal is weak, more blocks are
    // required for a successful detection. For strong signals, we go straight
    // back to the IDLE state if one non-active block is found. For weaker
    // signals we allow two non-active blocks in a row before going to IDLE.
    // STATE_DETECTED: Now the digit is considered a real detection and so the
    // start of digit is reported (digitActivated). There now must be a number
    // of non-active blocks before we consider the digit as ended.
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
          float det_quality = static_cast<float>(det_cnt)
                            / (duration * DET_CNT_HI_WEIGHT);
          if (det_quality > 0.5)
          {
            undet_thresh = min_undet_cnt;
          }
          else if (det_quality > 0.2)
          {
            undet_thresh = 2 * min_undet_cnt;
          }
          else
          {
            undet_thresh = 3 * min_undet_cnt;
          }
          if (debug)
          {
            //cout << " det_q=" << det_quality;
            cout << " activated";
          }
          digitActivated(last_digit_active);
          det_state = STATE_DETECTED;
        }
      }
      else
      {
        //if ((det_cnt_weight > 1) || (++undet_cnt >= 2))
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
        if (++undet_cnt >= undet_thresh)
        {
          const int first_block_time = 1000 * BLOCK_SIZE / INTERNAL_SAMPLE_RATE;
          const int block_time = 1000 * STEP_SIZE / INTERNAL_SAMPLE_RATE;
          const int dur_ms = first_block_time + block_time * (duration - 1);
          if (debug)
          {
            cout << " digit=" << last_digit_active;
            cout << " deactivated";
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
    cout << " " << (digit_active ? "^" : "");
    cout << endl;
    cout.flags(orig_cout_flags);
  }
} /* SvxSwDtmfDecoder::processBlock */


void SvxSwDtmfDecoder::DtmfGoertzel::initialize(float freq)
{
  Goertzel::initialize(freq, INTERNAL_SAMPLE_RATE);
  m_freq = freq;
  //m_max_fqdiff = m_freq * MAX_FQ_ERROR;
} /* SvxSwDtmfDecoder::DtmfGoertzel::initialize */


/*
 * This file has not been truncated
 */
