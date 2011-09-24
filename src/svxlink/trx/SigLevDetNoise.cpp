/**
@file	 SigLevDetNoise.cpp
@brief   A simple signal level detector based on noise measurements
@author  Tobias Blomberg / SM0SVX
@date	 2006-05-07

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2008 Tobias Blomberg / SM0SVX

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
//#include <iostream>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncAudioFilter.h>
#include <SigCAudioSink.h>


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

SigLevDetNoise::SigLevDetNoise(int sample_rate)
  : sample_rate(sample_rate), slope(1.0), offset(0.0), update_interval(0),
    update_counter(0), integration_time(0)
{
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
      slot(*this, &SigLevDetNoise::processSamples));
  sigc_sink->sigFlushSamples.connect(
      slot(*sigc_sink, &SigCAudioSink::allSamplesFlushed));
  sigc_sink->registerSource(filter);
  setIntegrationTime(0);
  reset();
} /* SigLevDetNoise::SigLevDetNoise */


SigLevDetNoise::~SigLevDetNoise(void)
{
  clearHandler();
  delete filter;
  delete sigc_sink;
} /* SigLevDetNoise::~SigLevDetNoise */


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


void SigLevDetNoise::setContinuousUpdateInterval(int interval_ms)
{
  update_interval = interval_ms * sample_rate / 1000;
  update_counter = 0;
} /* SigLevDetNoise::setContinuousUpdateInterval */


void SigLevDetNoise::setIntegrationTime(int time_ms)
{
  if (time_ms < 20)
  {
    time_ms = 20;
  }
  integration_time = time_ms * sample_rate / 1000;
} /* SigLevDetNoise::setIntegrationTime */


float SigLevDetNoise::lastSiglev(void) const
{
  double ss = 0.0;
  deque<double>::const_iterator it;
  for (it=ss_values.begin(); it!=ss_values.end(); ++it)
  {
    ss += *it;
  }
  return offset - slope * log10(sqrt(ss / ss_values.size()));
} /* SigLevDetNoise::lastSiglev */


void SigLevDetNoise::reset(void)
{
  filter->reset();
  last_siglev = pow10f(-offset / slope);
  update_counter = 0;
  ss_values.clear();
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
    float sample = samples[i];
    ss_values.push_back(sample * sample);
  }
  
  if (ss_values.size() > integration_time)
  {
    ss_values.erase(ss_values.begin(),
		    ss_values.begin()+ss_values.size()-integration_time);
  }

  if (update_interval > 0)
  {
    update_counter += count;
    if (update_counter >= update_interval)
    {
      signalLevelUpdated(lastSiglev());
      update_counter = 0;
    }
  }
  
  return count;
  
} /* SigLevDetNoise::processSamples */



/*
 * This file has not been truncated
 */

