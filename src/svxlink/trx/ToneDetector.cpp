/**
@file	 ToneDetector.cpp
@brief   A tone detector that use the Goertzel algorithm
@author  Tobias Blomberg / SM0SVX
@date	 2003-04-15

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2004-2022  Tobias Blomberg / SM0SVX

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
#include <iomanip>


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
  float               bw                      = 0.0f;
  int                 detect_delay_ms         = -1;
  int                 stable_count_thresh     = DEFAULT_STABLE_COUNT_THRESH;
  size_t              block_len               = 0;
  float               overlap_percent         = DEFAULT_OVERLAP_PERCENT;
  std::vector<float>  overlap_buf;
  size_t              overlap_buf_size        = 0;
  float               freq_tol_hz             = DEFAULT_FREQ_TOL_HZ;
  float               block_len_radians       = 0.0f;
  std::complex<float> prev_res_cmplx          = 0;
  float               prev_phase              = 0.0f;
  float               peak_thresh             = DEFAULT_PEAK_THRESH;
  int                 period_block_len        = 0;
  float               phase_offset            = 0.0f;
  float               phase_mean_thresh       = DEFAULT_PHASE_MEAN_THRESH;
  float               phase_var_thresh        = DEFAULT_PHASE_VAR_THRESH;
  float               phase_actual_fq         = 0.0f;
  Goertzel            center;
  Goertzel            lower;
  Goertzel            upper;
  std::vector<float>  window_table;
  bool                use_windowing           = DEFAULT_USE_WINDOWING;
  float               peak_to_tot_pwr_thresh  = DEFAULT_PEAK_TO_TOT_PWR_THRESH;
  float               snr_thresh              = DEFAULT_SNR_THRESH;
  float               passband_bw             = 0.0f;
}; /* struct ToneDetector::DetectorParams */


/****************************************************************************
 *
 * Prototypes and local functions
 *
 ****************************************************************************/

namespace
{
  inline double wrapToPi(double x)
  {
    if (x > M_PI)
    {
      x -= 2*M_PI * std::trunc((x+M_PI)/(2*M_PI));
    }
    else if (x < -M_PI)
    {
      x -= 2*M_PI * std::trunc((x-M_PI)/(2*M_PI));
    }
    return x;
  } /* wrapToPi */
}; /* Anonymous namespace */



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
  : tone_fq(tone_hz), buf_pos(0), is_activated(false),
    last_active(false), stable_count(0), phase_check_left(-1),
    par(nullptr), last_snr(0.0f), tone_fq_est(0.0f)
{
  det_par = new DetectorParams;
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


void ToneDetector::setDetectToneFrequencyTolerancePercent(
    float freq_tol_percent)
{
  setToneFrequencyTolerancePercent(det_par, freq_tol_percent);
} /* ToneDetector::setDetectToneFrequencyTolerance */


void ToneDetector::setUndetectToneFrequencyTolerancePercent(
    float freq_tol_percent)
{
  setToneFrequencyTolerancePercent(undet_par, freq_tol_percent);
} /* ToneDetector::setUndetectToneFrequencyTolerance */


void ToneDetector::setDetectOverlapPercent(float overlap_percent)
{
  det_par->overlap_percent = overlap_percent;
  setOverlapLength(det_par, det_par->block_len * overlap_percent / 100.0f);
} /* ToneDetector::setDetectOverlapPercent */


void ToneDetector::setUndetectOverlapPercent(float overlap_percent)
{
  undet_par->overlap_percent = overlap_percent;
  setOverlapLength(undet_par, undet_par->block_len * overlap_percent / 100.0f);
} /* ToneDetector::setUndetectOverlapPercent */


void ToneDetector::setDetectOverlapLength(size_t overlap)
{
  det_par->overlap_percent = 0.0f;
  setOverlapLength(det_par, overlap);
} /* ToneDetector::setDetectOverlapLength */


void ToneDetector::setUndetectOverlapLength(size_t overlap)
{
  undet_par->overlap_percent = 0.0f;
  setOverlapLength(undet_par, overlap);
} /* ToneDetector::setUndetectOverlapLength */


void ToneDetector::reset(void)
{
  setActivated(false);
  last_active = false;
  stable_count = 0;
  buf_pos = 0;
  tone_fq_est = 0.0f;
  passband_energy = 0.0f;
  win = par->window_table.begin();
  par->center.reset();
  par->lower.reset();
  par->upper.reset();
  par->overlap_buf.clear();
  par->prev_res_cmplx = 0;
  phaseCheckReset();
} /* ToneDetector::reset */


void ToneDetector::setDetectDelay(int delay_ms)
{
  setDelay(det_par, delay_ms);
} /* ToneDetector::setDetectDelay */


void ToneDetector::setDetectStableCountThresh(int count)
{
  det_par->stable_count_thresh = count;
} /* ToneDetector::setDetectStableCountThresh */


int ToneDetector::detectDelay(void) const
{
  return delay(det_par);
} /* ToneDetector::detectDelay */


void ToneDetector::setUndetectDelay(int delay_ms)
{
  setDelay(undet_par, delay_ms);
} /* ToneDetector::setUndetectDelay */


void ToneDetector::setUndetectStableCountThresh(int count)
{
  undet_par->stable_count_thresh = count;
} /* ToneDetector::setUndetectStableCountThresh */


int ToneDetector::undetectDelay(void) const
{
  return delay(undet_par);
} /* ToneDetector::undetectDelay */


void ToneDetector::setDetectBw(float bw_hz)
{
  setBw(det_par, bw_hz);
} /* ToneDetector::setDetectBw */


void ToneDetector::setUndetectBw(float bw_hz)
{
  setBw(undet_par, bw_hz);
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
  const float *end = buf + len;
  while (buf != end)
  {
    float famp;
    if (buf_pos < par->overlap_buf.size())
    {
      famp = par->overlap_buf.at(buf_pos);
    }
    else
    {
      famp = *buf++;
    }
    const size_t non_overlap_len = par->block_len - par->overlap_buf_size;
    if (buf_pos >= non_overlap_len)
    {
      const size_t insert_pos = buf_pos - non_overlap_len;
      if (insert_pos < par->overlap_buf.size())
      {
        par->overlap_buf[insert_pos] = famp;
      }
      else
      {
        par->overlap_buf.push_back(famp);
      }
    }

    passband_energy += static_cast<double>(famp) * famp;

      // First apply the Hamming window, if enabled
    if (par->use_windowing)
    {
      famp *= *(win++);
    }

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

    if (++buf_pos >= par->block_len)
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
  float bw = static_cast<float>(INTERNAL_SAMPLE_RATE) / par->block_len;
  float det_bw = bw;
  float win_comp_energy = 1.0f;

    // Compensate for power loss due to windowing if necessary
  if (par->use_windowing)
  {
    det_bw *= 1.3631;
    win_comp_energy = 1.835f * 1.835f;
  }

    // Calculate the magnitude for the center bin
  const std::complex<float> res_cmplx = par->center.result();
  float res_center = win_comp_energy * Goertzel::magnitudeSquared(res_cmplx);

    // Now determine if the tone is active or not. We start by checking
    // if the tone energy exceed the energy threshold. This check
    // is necessary to not give false detections on silent input, like when
    // the hardware squelch is closed on the receiver.
  active = active && (res_center > DEFAULT_TONE_ENERGY_THRESH);

  if (par->peak_thresh > 0.0f)
  {
      // Check if the center fq is above the lower fq bin by the peak threshold.
      // This is part of the "neighbour bin SNR" check.
    float res_lower = win_comp_energy * par->lower.magnitudeSquared();
    active = active && (res_center > (res_lower * par->peak_thresh));

      // Check if the center fq is above the upper fq bin by the peak threshold.
      // This is part of the "neighbour bin SNR" check.
    float res_upper = win_comp_energy * par->upper.magnitudeSquared();
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
    float Pnoise = (Ppassband - Ptone) / ((par->passband_bw-det_bw) / det_bw);
    
    //std::cout << "### Ppasspand=" << Ppassband
    //          << " Ptone=" << Ptone
    //          << " Pnoise=" << Pnoise
    //          << " rel=" << (Ptone / Ppassband)
    //          << " rel2=" << (res_center / passband_energy)
    //          << std::endl;

      // Calculate the signal to noise ratio.
      // Pnoise may turn negative, probably due to rounding errors, so we
      // arbitrarily set the SNR to a constant in that case. This typically
      // happens on signals with extremely little noise in them. Probably we
      // will only see this in constructed scenarious like in simulation.
    last_snr = 70.0f;
    if (Pnoise > 0.0f)
    {
      last_snr = 10.0f * log10f(Ptone / Pnoise);
    }
    
      // Check if the SNR is over the threshold
    active = active && (last_snr > par->snr_thresh);

      // Tell subscribers that the SNR changed
    snrUpdated(last_snr);
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

  if (par->freq_tol_hz > 0.0f)
  {
    const double phase_err =
      wrapToPi(std::arg(res_cmplx * std::conj(par->prev_res_cmplx)) -
          par->block_len_radians);
    par->prev_res_cmplx = res_cmplx;
    const double freq_err =
      INTERNAL_SAMPLE_RATE * phase_err /
      (2*M_PI * (par->block_len - par->overlap_buf_size));
    tone_fq_est = tone_fq + freq_err;
    active = active && (abs(freq_err) < par->freq_tol_hz);
    //std::cout << "### freq_tol[" << toneFq() << "]:"
    //          << std::showpos << std::fixed << std::setprecision(3)
    //          << " phase_err=" << phase_err
    //          << " freq_err=" << freq_err
    //          << " est=" << tone_fq_est
    //          << std::endl;
    //active = false;
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
    tone_fq_est = 0.0f;
  }

    // Point to the first windowing table entry
  win = par->window_table.begin();

    // Reset sample counter
  buf_pos = 0;

  par->center.reset();
  par->lower.reset();
  par->upper.reset();
  phaseCheckReset();
  passband_energy = 0.0f;

} /* ToneDetector::postProcess */


void ToneDetector::setActivated(bool activated)
{
  //std::cout << "### activate[" << toneFq() << "]=" << activated << std::endl;
  is_activated = activated;
  if (activated)
  {
    undet_par->prev_phase = det_par->prev_phase;
    undet_par->prev_res_cmplx = det_par->prev_res_cmplx;
    par = undet_par;
  }
  else
  {
    det_par->prev_phase = undet_par->prev_phase;
    det_par->prev_res_cmplx = undet_par->prev_res_cmplx;
    par = det_par;
  }
  par->overlap_buf.clear();
} /* ToneDetector::setActivated */


void ToneDetector::setToneFrequencyTolerancePercent(
    ToneDetector::DetectorParams* par,
    float freq_tol_percent)
{
  par->freq_tol_hz = toneFq() * freq_tol_percent / 100.0f;

    // Calculate the theoretical angle difference in radians between two DFT
    // blocks
  par->block_len_radians =
    par->block_len * 2*M_PI * toneFq() / INTERNAL_SAMPLE_RATE;
  if (par->overlap_buf_size > 0)
  {
    const float olap_ratio =
      static_cast<float>(par->block_len) / par->overlap_buf_size;
    const float bw =
      static_cast<float>(INTERNAL_SAMPLE_RATE) / par->block_len;
    par->block_len_radians +=
      2*M_PI / olap_ratio * (olap_ratio - fmod(toneFq() / bw, olap_ratio));
  }
  par->block_len_radians = wrapToPi(par->block_len_radians);
} /* ToneDetector::setToneFrequencyTolerance */


void ToneDetector::setDelay(ToneDetector::DetectorParams* par, int delay_ms)
{
  //std::cout << "### ToneDetector::setDelay: delay_ms=" << delay_ms
  //          << std::endl;
  par->detect_delay_ms = delay_ms;
  if (delay_ms > 0)
  {
    size_t block_cnt = 1;
    size_t delay_cnt = delay_ms * INTERNAL_SAMPLE_RATE / 1000;
    if (delay_cnt > par->block_len)
    {
      block_cnt += 1 + (delay_cnt - par->block_len) /
                   (par->block_len - par->overlap_buf_size);
    }
    par->stable_count_thresh = block_cnt;
    //std::cout << "### ToneDetector::setDelay:"
    //          << " fq=" << toneFq()
    //          << " delay_cnt=" << delay_cnt
    //          << " delay_ms=" << delay_ms
    //          << " block_len=" << par->block_len
    //          << " block_cnt=" << block_cnt
    //          << " delay()=" << delay(par)
    //          << std::endl;
  }
  else if (delay_ms == 0)
  {
    par->stable_count_thresh = DEFAULT_STABLE_COUNT_THRESH;
  }
} /* ToneDetector::setDelay */


int ToneDetector::delay(ToneDetector::DetectorParams* par) const
{
  size_t samp_cnt = par->block_len;
  samp_cnt += (par->stable_count_thresh - 1) *
              (par->block_len - par->overlap_buf_size);
  return samp_cnt * 1000 / INTERNAL_SAMPLE_RATE;
} /* ToneDetector::delay */


void ToneDetector::setOverlapPercent(ToneDetector::DetectorParams* par,
                                     float overlap_percent)
{
  setOverlapLength(par, par->block_len * overlap_percent / 100.0f);
} /* ToneDetector::setOverlapPercent */


void ToneDetector::setOverlapLength(ToneDetector::DetectorParams* par,
                                    size_t overlap)
{
  par->overlap_buf_size = overlap;
  if (overlap >= par->overlap_buf.size())
  {
    par->overlap_buf.reserve(overlap);
  }
  else
  {
    par->overlap_buf.resize(overlap);
  }
  setDelay(par, par->detect_delay_ms);
} /* ToneDetector::setOverlapLength */


void ToneDetector::setBw(ToneDetector::DetectorParams* par, float bw_hz)
{
  par->bw = bw_hz;

    // Adjust block length to minimize the DFT error
  par->block_len = lrintf(INTERNAL_SAMPLE_RATE *
                          ceilf(tone_fq / bw_hz) / tone_fq);

  par->window_table.clear();
  if (par->use_windowing)
  {
      // Set up Hamming window coefficients
    for (size_t i = 0; i < par->block_len; i++)
    {
      //float a0 = 0.54;
      float a0 = 25.0 / 46.0;
      par->window_table.push_back(
          a0 - (1.0f - a0) * cosf(2.0f * M_PI * i / (par->block_len - 1)));
    }
  }

  par->center.initialize(tone_fq, INTERNAL_SAMPLE_RATE);
  par->lower.initialize(tone_fq - 2 * bw_hz, INTERNAL_SAMPLE_RATE);
  par->upper.initialize(tone_fq + 2 * bw_hz, INTERNAL_SAMPLE_RATE);

  setOverlapPercent(par, par->overlap_percent);
} /* ToneDetector::setBw */


/*
 * This file has not been truncated
 */
