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
#include "Goertzel.h"



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

struct ToneDetector::DetectorParams
{
  float		      bw;
  int		      stable_count_thresh;
  int		      block_len;
  float		      peak_thresh;
  int		      period_block_len;
  float		      phase_offset;
  float		      phase_mean_thresh;
  float		      phase_var_thresh;
  float		      phase_actual_fq;
  Goertzel	      center;
  Goertzel	      lower;
  Goertzel	      upper;
  std::vector<float>  window_table;
  bool		      use_windowing;
  float		      peak_to_tot_pwr_thresh;
  float		      snr_thresh;
  float		      passband_bw;
};


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

ToneDetector::ToneDetector(float tone_hz, float width_hz, int det_delay_ms)
  : tone_fq(tone_hz), samples_left(0), is_activated(false),
    last_active(false), stable_count(0), phase_check_left(-1),
    par(0) 
{
  det_par = new DetectorParams;
  det_par->bw = width_hz;
  det_par->stable_count_thresh = DEFAULT_STABLE_COUNT_THRESH;
  det_par->block_len = 0;
  det_par->peak_thresh = DEFAULT_PEAK_THRESH;
  det_par->period_block_len = 0;
  det_par->phase_offset = 0.0f;
  det_par->phase_mean_thresh = DEFAULT_PHASE_MEAN_THRESH;
  det_par->phase_var_thresh = DEFAULT_PHASE_VAR_THRESH;
  det_par->phase_actual_fq = 0.0f;
  det_par->use_windowing = DEFAULT_USE_WINDOWING;
  det_par->peak_to_tot_pwr_thresh = 0.0f;
  det_par->snr_thresh = 0.0f;
  det_par->passband_bw = 0.0f;

  setDetectBw(width_hz);

    // Calculate the block length of one period of the tone to detect.
    // This is used in the phase detector. Make sure at least one period
    // of the tone we want to detect fit in the block. This is because we
    // want the actual_fq to be lower or equal to the tone we want to
    // detect since if it is the other way around, the phase/fq relation
    // is not linear.
  det_par->period_block_len =
	static_cast<int>(ceilf(INTERNAL_SAMPLE_RATE / tone_hz));

    // Calculate the actual frequency for the phase detector. This is used as
    // a reference but it's not the center frequency for the phase detector.
  det_par->phase_actual_fq =
	static_cast<float>(INTERNAL_SAMPLE_RATE) / det_par->period_block_len;

    // Calculate the phase offset due to the difference between the requested
    // fq and the actual fq.
    // We need to multiply this offset by two to compensate for both the offset
    // of the actual_fq from the Goertzel recursive stage fq and also to
    // compensate for the difference between the actual_fq and the tone_fq
    // in the phase detector.
  det_par->phase_offset = 2.0f * (tone_hz - det_par->phase_actual_fq) *
		         (M_PI / det_par->phase_actual_fq);
  
  setDetectDelay(det_delay_ms);

    // Copy the just setup detection parameters to the undetection
    // parameters to start out with.
  undet_par = new DetectorParams(*det_par);

  reset();

} /* ToneDetector::ToneDetector */


ToneDetector::~ToneDetector(void)
{
  delete det_par;
  det_par = 0;
  delete undet_par;
  undet_par = 0;
} /* ToneDetector::~ToneDetector */


void ToneDetector::reset(void)
{
  setActivated(false);
  last_active = false;
  stable_count = 0;
  samples_left = par->block_len;

    // Point to the first windowing table entry
  win = par->window_table.begin();

    // Reset Goertzel filters
  par->center.reset();
  par->lower.reset();
  par->upper.reset();

    // Reset phase check state variables
  phaseCheckReset();

    // Reset the passband energy
  passband_energy = 0.0f;
} /* ToneDetector::reset */


void ToneDetector::setDetectDelay(int delay_ms)
{
  if (delay_ms > 0)
  {
    det_par->stable_count_thresh = static_cast<int>(
	ceil(delay_ms * INTERNAL_SAMPLE_RATE / (det_par->block_len * 1000.0)));
  }
  else
  {
    det_par->stable_count_thresh = DEFAULT_STABLE_COUNT_THRESH;
  }
} /* ToneDetector::setDetectDelay */


void ToneDetector::setDetectStableCountThresh(int count)
{
  det_par->stable_count_thresh = count;
} /* ToneDetector::setDetectStableCountThresh */


int ToneDetector::detectDelay(void) const
{
  return det_par->stable_count_thresh * det_par->block_len *
         1000 / INTERNAL_SAMPLE_RATE;
} /* ToneDetector::detectDelay */


void ToneDetector::setUndetectDelay(int delay_ms)
{
  if (delay_ms > 0)
  {
    undet_par->stable_count_thresh = static_cast<int>(
	ceil(delay_ms * INTERNAL_SAMPLE_RATE /
	     (undet_par->block_len * 1000.0)));
  }
  else
  {
    undet_par->stable_count_thresh = DEFAULT_STABLE_COUNT_THRESH;
  }
} /* ToneDetector::setUndetectDelay */


void ToneDetector::setUndetectStableCountThresh(int count)
{
  undet_par->stable_count_thresh = count;
} /* ToneDetector::setUndetectStableCountThresh */


int ToneDetector::undetectDelay(void) const
{
  return undet_par->stable_count_thresh * undet_par->block_len *
         1000 / INTERNAL_SAMPLE_RATE;
} /* ToneDetector::undetectDelay */


void ToneDetector::setDetectBw(float bw_hz)
{
  det_par->bw = bw_hz;

    // Adjust block length to minimize the DFT error
  det_par->block_len = lrintf(INTERNAL_SAMPLE_RATE * 
		              ceilf(tone_fq / bw_hz) / tone_fq);

  det_par->window_table.clear();
  if (det_par->use_windowing)
  {
      // Set up Hamming window coefficients
    for (int i = 0; i < det_par->block_len; i++)
    {
      det_par->window_table.push_back(
          0.54 - 0.46 * cosf(2.0f * M_PI * i / (det_par->block_len - 1)));
    }
  }

  det_par->center.initialize(tone_fq, INTERNAL_SAMPLE_RATE);
  det_par->lower.initialize(tone_fq - 2 * bw_hz, INTERNAL_SAMPLE_RATE);
  det_par->upper.initialize(tone_fq + 2 * bw_hz, INTERNAL_SAMPLE_RATE);
  
} /* ToneDetector::setDetectBw */


void ToneDetector::setUndetectBw(float bw_hz)
{
  undet_par->bw = bw_hz;

    // Adjust block length to minimize the DFT error
  undet_par->block_len = lrintf(INTERNAL_SAMPLE_RATE * 
		               ceilf(tone_fq / bw_hz) / tone_fq);

  undet_par->window_table.clear();
  if (undet_par->use_windowing)
  {
      // Set up Hamming window coefficients
    for (int i = 0; i < undet_par->block_len; i++)
    {
      undet_par->window_table.push_back(
          0.54 - 0.46 * cosf(2.0f * M_PI * i / (undet_par->block_len - 1)));
    }
  }

  undet_par->center.initialize(tone_fq, INTERNAL_SAMPLE_RATE);
  undet_par->lower.initialize(tone_fq - 2 * bw_hz, INTERNAL_SAMPLE_RATE);
  undet_par->upper.initialize(tone_fq + 2 * bw_hz, INTERNAL_SAMPLE_RATE);

} /* ToneDetector::setUndetectBw */


void ToneDetector::setPeakThresh(float thresh)
{
  setDetectPeakThresh(thresh);
  setUndetectPeakThresh(thresh);
} /* ToneDetector::setPeakThresh */


void ToneDetector::setDetectPeakThresh(float thresh)
{
  if (thresh > 0.0f)
  {
    det_par->peak_thresh = powf(10, thresh/10.0f);
  }
  else
  {
    det_par->peak_thresh = 0.0f;
  }
} /* ToneDetector::setDetectPeakThresh */


void ToneDetector::setUndetectPeakThresh(float thresh)
{
  if (thresh > 0.0f)
  {
    undet_par->peak_thresh = powf(10, thresh/10.0f);
  }
  else
  {
    undet_par->peak_thresh = 0.0f;
  }
} /* ToneDetector::setUndetectPeakThresh */


void ToneDetector::setDetectPeakToTotPwrThresh(float thresh)
{
  det_par->peak_to_tot_pwr_thresh = thresh;
} /* ToneDetector::setDetectPeakToTotPwrThresh */


void ToneDetector::setUndetectPeakToTotPwrThresh(float thresh)
{
  undet_par->peak_to_tot_pwr_thresh = thresh;
} /* ToneDetector::setUndetectPeakToTotPwrThresh */


void ToneDetector::setDetectSnrThresh(float thresh_db, float passband_bw_hz)
{
  det_par->snr_thresh = thresh_db;
  det_par->passband_bw = passband_bw_hz;
} /* ToneDetector::setDetectSnrThresh */


void ToneDetector::setUndetectSnrThresh(float thresh_db, float passband_bw_hz)
{
  undet_par->snr_thresh = thresh_db;  
  undet_par->passband_bw = passband_bw_hz;
} /* ToneDetector::setUndetectSnrThresh */


void ToneDetector::setDetectPhaseBwThresh(float bw_hz, float stddev_hz)
{
  if ((bw_hz > 0.0f) && (stddev_hz > 0.0f))
  {
    det_par->phase_mean_thresh =
	bw_hz * (M_PI / det_par->phase_actual_fq) / 2.0f;
    det_par->phase_var_thresh = stddev_hz * (M_PI / det_par->phase_actual_fq);
    det_par->phase_var_thresh *= det_par->phase_var_thresh;

    setDetectUseWindowing(false);
  }
  else
  {
    det_par->phase_mean_thresh = 0.0f;
    det_par->phase_var_thresh = 0.0f;
  }
} /* ToneDetector::setDetectPhaseBwThresh */


void ToneDetector::setUndetectPhaseBwThresh(float bw_hz, float stddev_hz)
{
  if ((bw_hz > 0.0f) && (stddev_hz > 0.0f))
  {
    undet_par->phase_mean_thresh =
	bw_hz * (M_PI / undet_par->phase_actual_fq) / 2.0f;
    undet_par->phase_var_thresh =
	stddev_hz * (M_PI / undet_par->phase_actual_fq);
    undet_par->phase_var_thresh *= undet_par->phase_var_thresh;

    setUndetectUseWindowing(false);
  }
  else
  {
    undet_par->phase_mean_thresh = 0.0f;
    undet_par->phase_var_thresh = 0.0f;
  }
} /* ToneDetector::setUndetectPhaseBwThresh */


void ToneDetector::setDetectUseWindowing(bool enable)
{
  if (enable == det_par->use_windowing)
  {
    return;
  }
  det_par->use_windowing = enable;
  setDetectBw(det_par->bw);
} /* ToneDetector::setDetectUseWindowing */


void ToneDetector::setUndetectUseWindowing(bool enable)
{
  if (enable == undet_par->use_windowing)
  {
    return;
  }
  undet_par->use_windowing = enable;
  setUndetectBw(undet_par->bw);
} /* ToneDetector::setUndetectUseWindowing */


int ToneDetector::writeSamples(const float *buf, int len)
{
  for (int i = 0;  i < len;  i++)
  {
    float famp = *(buf++);

      // First apply the Hamming window, if enabled
    if (par->use_windowing)
    {
      famp *= *(win++);
    }

    passband_energy += famp * famp;

      // Run the recursive Goertzel stage for the center frequency
    par->center.calc(famp);

    if (par->peak_thresh > 0.0f)
    {
        // Run the recursive Goertzel stage for the lower and upper frequency
      par->lower.calc(famp);
      par->upper.calc(famp);
    }

    if ((phase_check_left > 0) && (--phase_check_left == 0))
    {
      phaseCheck();
      phase_check_left = par->period_block_len;
    }

    if (--samples_left == 0)
    {
      postProcess();
    }
  }
    
  return len;
  
} /* ToneDetector::writeSamples */



/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/

void ToneDetector::phaseCheckReset(void)
{
  if (par->phase_mean_thresh > 0.0f)
  {
    phase_check_left = par->period_block_len;
    prev_phase = 2.0f * M_PI;
    phase_diffs.clear();
  }
  else
  {
    phase_check_left = -1;
  }
} /* ToneDetector::phaseCheckReset */


void ToneDetector::phaseCheck(void)
{
  float phase = par->center.phase();
  if (prev_phase < 2.0f * M_PI)
  {
    float diff = phase - prev_phase;
    if (diff > M_PI)
    {
      diff = 2.0f * M_PI - diff;
    }
    else if (diff < -M_PI)
    {
      diff = 2.0f * M_PI + diff;
    }
    diff -= par->phase_offset;
    phase_diffs.push_back(diff);
  }
  prev_phase = phase;
} /* ToneDetector::phaseCheck */


void ToneDetector::postProcess(void)
{
  bool active = true;

    // Calculate the magnitude for the center bin
  float res_center = par->center.magnitudeSquared();

    // Now determine if the tone is active or not. We start by checking
    // if the tone energy exceed the energy threshold. This check
    // is necessary to not give false detections on silent input, like when
    // the hardware squelch is closed on the receiver.
  active = active && (res_center > DEFAULT_TONE_ENERGY_THRESH);

  if (par->peak_thresh > 0.0f)
  {
      // Check if the center fq is above the lower fq bin by the peak threshold.
      // This is part of the "neighbour bin SNR" check.
    float res_lower = par->lower.magnitudeSquared();
    active = active && (res_center > (res_lower * par->peak_thresh));

      // Check if the center fq is above the upper fq bin by the peak threshold.
      // This is part of the "neighbour bin SNR" check.
    float res_upper = par->upper.magnitudeSquared();
    active = active && (res_center > (res_upper * par->peak_thresh));
  }

  if (par->peak_to_tot_pwr_thresh > 0.0f)
  {
      // Calculate the relation between the center bin power and the total
      // passband power. If all power within the passband is in the tone
      // we will get a value of 1.0. If none of the passband power is in
      // the tone we will get a value of 0.0.
      // The calculation below is a bit optimized but this is what we
      // are doing:
      //
      //    float Ptone = 2.0f * res_center / (par->block_len*par->block_len);
      //    float Ppassband = passband_energy / par->block_len;
      //    float peak_to_tot_pwr = Ptone / Ppassband;
    float peak_to_tot_pwr =
	2.0f * res_center / (par->block_len * passband_energy);
    active = active && (peak_to_tot_pwr < 1.5f) &&
	(peak_to_tot_pwr > par->peak_to_tot_pwr_thresh);
  }

  if (par->passband_bw > 0.0f)
  {
      // Calculate mean tone power
    float Ptone = 2.0f * res_center / (par->block_len*par->block_len);
    
      // Calculate mean passband power
    float Ppassband = passband_energy / par->block_len;
    
      // Estimate the mean noise floor over the whole passband
    float Pnoise = (Ppassband - Ptone) / ((par->passband_bw-par->bw) / par->bw);
    
      // Calculate the signal to noise ratio.
      // Pnoise may turn negative, probably due to rounding errors, so we
      // arbitrarily set the SNR to a constant in that case. This typically
      // happens on signals with extremely little noise in them. Probably we
      // will only see this in constructed scenarious like in simulation.
    float snr = 70.0f;
    if (Pnoise > 0.0f)
    {
      snr = 10.0f * log10f(Ptone / Pnoise);
    }
    
      // Check if the SNR is over the threshold
    active = active && (snr > par->snr_thresh);

      // Tell subscribers that the SNR changed
    snrUpdated(snr);
  }
  
    // If phase checking is active, check the phase
  if (phase_check_left > 0)
  {
      // Calculate the mean value of the phase difference measurements.
    float phase_mean = 0.0f;
    for (unsigned i=0; i<phase_diffs.size(); ++i)
    {
      phase_mean += phase_diffs[i];
    }
    phase_mean /= phase_diffs.size();

      // Calculate the variance of the phase difference measurements.
    float phase_variance = 0.0f;
    for (unsigned i=0; i<phase_diffs.size(); ++i)
    {
      float diff = phase_diffs[i] - phase_mean;
      phase_variance += diff * diff;
    }
    phase_variance /= phase_diffs.size();

      // Make sure that the phase is indicating that we have acquired the
      // correct tone. If the acquired tone match exactly,
      // the phase_mean should be around 0. The variance is also checked to
      // make sure we have a stable, not noisy, tone.
    active = active && ((fabs(phase_mean) < par->phase_mean_thresh) &&
                        (phase_variance < par->phase_var_thresh));
  }

    // If the detector is stable in the active or inactive state, the
    // stable counter is increaed. Otherwise it is reset.
  if (active == last_active)
  {
    stable_count += 1;
  }
  else
  {
    stable_count = 1;
  }
  last_active = active;
  
    // Now check if the detector should change state.
  if ((is_activated != active) && (stable_count >= par->stable_count_thresh))
  {
    setActivated(active);
    activated(active);
    if (active)
    {
      detected(tone_fq);
    }
  }

    // Point to the first windowing table entry
  win = par->window_table.begin();
    // Reload sample counter
  samples_left = par->block_len;

  par->center.reset();
  par->lower.reset();
  par->upper.reset();
  phaseCheckReset();
  passband_energy = 0.0f;

} /* ToneDetector::postProcess */


void ToneDetector::setActivated(bool activated)
{
  is_activated = activated;
  par = is_activated ? undet_par : det_par;
} /* ToneDetector::setActivated */


/*
 * This file has not been truncated
 */
