/**
@file	 SigLevDetTone.cpp
@brief   A signal level detector using tone in the 5.5 to 6.4kHz band
@author  Tobias Blomberg / SM0SVX
@date	 2009-05-23

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

#include <iostream>
#include <cstdio>
#include <cstdlib>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncConfig.h>
#include <AsyncSigCAudioSink.h>
#include <AsyncAudioFilter.h>
#include <common.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "SigLevDetTone.h"
#include "Goertzel.h"



/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/

using namespace std;
using namespace Async;
using namespace SvxLink;



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

#if 0
class SigLevDetTone::HammingWindow
{
  public:
    HammingWindow(unsigned wsize)
      : wsize(wsize)
    {
      window = new float[wsize];
      for (unsigned i=0; i<wsize; ++i)
      {
        window[i] = 0.54f - 0.46f * cosf(2*M_PI*(float)i/(float)wsize);
      }
      reset();
    }

    ~HammingWindow(void)
    {
      delete [] window;
    }

    void reset(void)
    {
      wpos = 0;
    }

    float calc(float sample)
    {
      float windowed = sample * window[wpos];
      wpos = wpos < wsize-1 ? wpos+1 : 0;
      return windowed;
    }

  private:
    const unsigned  wsize;
    float           *window;
    unsigned        wpos;
    
    HammingWindow(const HammingWindow&);
    HammingWindow& operator=(const HammingWindow&);
    
};  /* HammingWindow */
#endif


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

SigLevDetTone::SigLevDetTone(void)
  : sample_rate(0), tone_siglev_map(10), block_idx(0), last_siglev(0),
    passband_energy(0.0f), filter(0), prev_peak_to_tot_pwr(0.0f),
    integration_time(1), update_interval(0), update_counter(0)
{
  for (int i=0; i<10; ++i)
  {
    det[i] = 0;
  }
} /* SigLevDetTone::SigLevDetTone */


SigLevDetTone::~SigLevDetTone(void)
{
  delete filter;

  for (int i=0; i<10; ++i)
  {
    delete det[i];
  }
} /* SigLevDetTone::~SigLevDetTone */


bool SigLevDetTone::initialize(Config &cfg, const string& name, int sample_rate)
{
  if (!SigLevDet::initialize(cfg, name, sample_rate))
  {
    return false;
  }

  if (sample_rate != 16000)
  {
    cerr << "*** ERROR: The tone signal level detector only works at 16kHz "
            "internal sampling rate\n";
    return false;
  }

  this->sample_rate = sample_rate;

  filter = new AudioFilter("BpBu8/5400-6500", sample_rate);
  setHandler(filter);

  SigCAudioSink *sigc_sink = new SigCAudioSink;
  sigc_sink->sigWriteSamples.connect(
      mem_fun(*this, &SigLevDetTone::processSamples));
  sigc_sink->sigFlushSamples.connect(
      mem_fun(*sigc_sink, &SigCAudioSink::allSamplesFlushed));
  filter->registerSink(sigc_sink, true);

  for (int i=0; i<10; ++i)
  {
    det[i] = new Goertzel(5500 + i * 100, sample_rate);
    tone_siglev_map[i] = 100 - i * 10;
  }
  reset();

  string mapstr;
  if (cfg.getValue(name, "TONE_SIGLEV_MAP", mapstr))
  {
    size_t list_len = splitStr(tone_siglev_map, mapstr, ", ");
    if (list_len != 10)
    {
      cerr << "*** ERROR: Config variable " << name << "/TONE_SIGLEV_MAP must "
           << "contain exactly ten comma separated siglev values.\n";
      return false;
    }
  }
  
  return SigLevDet::initialize(cfg, name, sample_rate);
  
} /* SigLevDetTone::initialize */


void SigLevDetTone::reset(void)
{
  for (int i=0; i<10; ++i)
  {
    det[i]->reset();
  }
  block_idx = 0;
  last_siglev = 0;
  passband_energy = 0.0f;
  prev_peak_to_tot_pwr = 0.0f;
  update_counter = 0;
  siglev_values.clear();
} /* SigLevDetTone::reset */


void SigLevDetTone::setContinuousUpdateInterval(int interval_ms)
{
  update_interval = interval_ms * sample_rate / 1000;
  update_counter = 0;  
} /* SigLevDetTone::setContinuousUpdateInterval */


void SigLevDetTone::setIntegrationTime(int time_ms)
{
    // Calculate the integration time expressed as the
    // number of processing blocks.
  integration_time = time_ms * 16000 / 1000 / BLOCK_SIZE;
  if (integration_time <= 0)
  {
    integration_time = 1;
  }
} /* SigLevDetTone::setIntegrationTime */


float SigLevDetTone::siglevIntegrated(void) const
{
  if (siglev_values.size() > 0)
  {
    int sum = 0;
    deque<int>::const_iterator it;
    for (it=siglev_values.begin(); it!=siglev_values.end(); ++it)
    {
      sum += *it;
    }
    return sum / siglev_values.size();
  }
  return 0;
} /* SigLevDetTone::siglevIntegrated */



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

int SigLevDetTone::processSamples(const float *samples, int count)
{
  for (int i=0; i<count; ++i)
  {
    const float &sample = samples[i];

    passband_energy += sample * sample;

    for (int detno=0; detno < 10; ++detno)
    {
      det[detno]->calc(sample);
    }
    
    if (++block_idx == BLOCK_SIZE)
    {
      float max = 0.0f;
      int max_idx = -1;
      for (int detno=0; detno < 10; ++detno)
      {
        float res = det[detno]->magnitudeSquared();
        det[detno]->reset();
        if (res > max)
        {
          max = res;
          max_idx = detno;
        }
      }

      last_siglev = 0;

        // Check that we have enough energy in the tone to do a
        // proper detection
      if (max > ENERGY_THRESH)
      {
          // Calculate the coefficient used to get from relative magnitude
          // squared to the "peak to total power" relation.
        float coeff = 2.0f / (BLOCK_SIZE * passband_energy);

          // Calculate the peak to total bandpass power relation
        float peak_to_tot_pwr = coeff * max;
        
          // Filter it using a first order IIR filter.
        peak_to_tot_pwr = prev_peak_to_tot_pwr
                          + ALPHA * (peak_to_tot_pwr - prev_peak_to_tot_pwr);
        prev_peak_to_tot_pwr = peak_to_tot_pwr;

          // If the relation value is larger than 1.5 it's probably a bogus
          // value. In theory, the relation value should never exceed 1.0.
        if (peak_to_tot_pwr < 1.5f)
        {
#if 0
            // The tone frequency may be offset so that energy spill into
            // neighbouring bins. Check the bin above and below to see if
            // adding the energy from either bin will get us above the
            // threshold. Subtract the other bin to compensate a bit.
          float lo_peak_to_tot_pwr = 0.0f, hi_peak_to_tot_pwr = 0.0f;
          if (max_idx > 0)
          {
            lo_peak_to_tot_pwr = coeff * det[max_idx-1];
          }
          if (max_idx < 9)
          {
            hi_peak_to_tot_pwr = coeff * det[max_idx+1];
          }
          peak_to_tot_pwr += max(lo_peak_to_tot_pwr - hi_peak_to_tot_pwr,
                                 hi_peak_to_tot_pwr - lo_peak_to_tot_pwr);
#endif
          if (peak_to_tot_pwr > DET_THRESH)
          {
            last_siglev = tone_siglev_map[max_idx];
            //printf("fq=%d  max=%f  siglev=%d  quality=%.1f\n",
            //       5500 + max_idx * 100, max, last_siglev, peak_to_tot_pwr);
          }
        }
      }

      siglev_values.push_back(last_siglev);
      if (siglev_values.size() > integration_time)
      {
	siglev_values.erase(siglev_values.begin(),
		siglev_values.begin()+siglev_values.size()-integration_time);
      }
      
      if (update_interval > 0)
      {
	update_counter += BLOCK_SIZE;
	if (update_counter >= update_interval)
	{
	  signalLevelUpdated(siglevIntegrated());
	  update_counter = 0;
	}
      }

      passband_energy = 0.0f;
      block_idx = 0;
    }
  }
  
  return count;
  
} /* SigLevDetTone::processSamples */



/*
 * This file has not been truncated
 */

