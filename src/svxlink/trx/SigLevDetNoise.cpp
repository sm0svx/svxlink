/**
@file	 SigLevDetNoise.cpp
@brief   A simple signal level detector based on noise measurements
@author  Tobias Blomberg / SM0SVX
@date	 2006-05-07

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2014 Tobias Blomberg / SM0SVX

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

#include <cmath>
#include <limits>
//#include <iostream>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncAudioFilter.h>
#include <AsyncSigCAudioSink.h>
#include <AsyncConfig.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "SigLevDetNoise.h"



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



/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/

SigLevDetNoise::SigLevDetNoise(void)
  : sample_rate(0), block_len(0), filter(0), sigc_sink(0),
    slope(10.0), offset(0.0), update_interval(0), update_counter(0),
    integration_time(0), ss(0.0), ss_cnt(0),
    bogus_thresh(numeric_limits<float>::max())
{
} /* SigLevDetNoise::SigLevDetNoise */


SigLevDetNoise::~SigLevDetNoise(void)
{
  clearHandler();
  delete filter;
  delete sigc_sink;
} /* SigLevDetNoise::~SigLevDetNoise */


bool SigLevDetNoise::initialize(Config &cfg, const string& name,
                                int sample_rate)
{
  this->sample_rate = sample_rate;
  block_len = BLOCK_TIME * sample_rate / 1000;
  if (sample_rate >= 16000)
  {
    filter = new AudioFilter("BpBu4/5000-5500", sample_rate);
  }
  else
  {
    filter = new AudioFilter("HpBu4/3500", sample_rate);
  }
  setHandler(filter);
  sigc_sink = new SigCAudioSink;
  sigc_sink->sigWriteSamples.connect(
      mem_fun(*this, &SigLevDetNoise::processSamples));
  sigc_sink->sigFlushSamples.connect(
      mem_fun(*sigc_sink, &SigCAudioSink::allSamplesFlushed));
  sigc_sink->registerSource(filter);
  setIntegrationTime(0);

  cfg.getValue(name, "SIGLEV_OFFSET", offset);
  cfg.getValue(name, "SIGLEV_SLOPE", slope);
  cfg.getValue(name, "SIGLEV_BOGUS_THRESH", bogus_thresh);

  reset();

  return SigLevDet::initialize(cfg, name, sample_rate);

} /* SigLevDetNoise::initialize */


void SigLevDetNoise::setDetectorSlope(float slope)
{
  this->slope = slope;
  reset();
} /* SigLevDetNoise::setDetectorSlope  */


void SigLevDetNoise::setDetectorOffset(float offset)
{
  this->offset = offset;
  reset();
} /* SigLevDetNoise::setDetectorOffset  */


void SigLevDetNoise::setBogusThresh(float thresh)
{
  bogus_thresh = thresh;
} /* SigLevDetNoise::setBogusThresh */


void SigLevDetNoise::setContinuousUpdateInterval(int interval_ms)
{
  update_interval = interval_ms * sample_rate / 1000;
  update_counter = 0;
} /* SigLevDetNoise::setContinuousUpdateInterval */


void SigLevDetNoise::setIntegrationTime(int time_ms)
{
  if (time_ms < static_cast<int>(BLOCK_TIME))
  {
    time_ms = BLOCK_TIME;
  }
  integration_time = time_ms * sample_rate / 1000;

  while (ss_idx.size() > integration_time / block_len)
  {
    ss_values.erase(*ss_idx.begin());
    ss_idx.pop_front();
  }
} /* SigLevDetNoise::setIntegrationTime */


float SigLevDetNoise::lastSiglev(void) const
{
  if (ss_idx.empty())
  {
    return 0.0f;
  }

    // Calculate the siglev value
  float siglev = offset - slope * log10(*ss_idx.back());

    // If the siglev value is way above 100 (like 120), it's probably bogus.
    // It's likely that this is caused by a closed squelch on the receiver or
    // that it has been turned off.
  if (siglev > bogus_thresh)
  {
    return 0.0f;
  }

  return offset - slope * log10(*ss_idx.back());

} /* SigLevDetNoise::lastSiglev */


float SigLevDetNoise::siglevIntegrated(void) const
{
  if (ss_values.empty())
  {
    return 0.0f;
  }

    // Calculate the siglev value.
    // Compensate for the over estimation of the siglev value caused by
    // using the minimum value over the "integration time".
    // The over estimation have been determined by a small number of
    // experiments so it may be wrong for some receivers.
    // The compensation may have to be determined for each receiver using
    // calibration but we'll try to have it hard coded for now.
    // If the BLOCK_TIME is changed, the compensation probably will have to
    // be changed too.
  float siglev = offset - slope * (log10(*ss_values.begin()) + 0.25);

    // If the siglev value is way above 100 (like 120), it's probably bogus.
    // It's likely that this is caused by a closed squelch on the receiver or
    // that it has been turned off.
  if (siglev > bogus_thresh)
  {
    return 0.0f;
  }

  return siglev;

} /* SigLevDetNoise::siglevIntegrated */


void SigLevDetNoise::reset(void)
{
  filter->reset();
  update_counter = 0;
  ss_values.clear();
  ss_idx.clear();
  ss_cnt = 0;
  ss = 0.0;
} /* SigLevDetNoise::reset */



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

int SigLevDetNoise::processSamples(float *samples, int count)
{
  for (int i=0; i<count; ++i)
  {
    const float &sample = samples[i];
    ss += static_cast<double>(sample) * sample;
    if (++ss_cnt >= block_len)
    {
      SsSetIter it = ss_values.insert(ss);
      ss_idx.push_back(it);
      if (ss_idx.size() > integration_time / block_len)
      {
        ss_values.erase(*ss_idx.begin());
        ss_idx.pop_front();
      }

      ss = 0.0;
      ss_cnt = 0;
    }
  }

  if (update_interval > 0)
  {
    update_counter += count;
    if (update_counter >= update_interval)
    {
      signalLevelUpdated(siglevIntegrated());
      update_counter = 0;
    }
  }
  
  return count;
  
} /* SigLevDetNoise::processSamples */



/*
 * This file has not been truncated
 */

