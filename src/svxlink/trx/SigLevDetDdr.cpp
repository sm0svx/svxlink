/**
@file	 SigLevDetDdr.cpp
@brief   A signal level detector measuring power levels using a DDR
@author  Tobias Blomberg / SM0SVX
@date	 2014-07-17

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


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "SigLevDetDdr.h"
#include "Ddr.h"



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

SigLevDetDdr::SigLevDetDdr(void)
  : sample_rate(0), block_idx(0), last_siglev(0.0f), integration_time(1),
    update_interval(0), update_counter(0), pwr_sum(0.0), slope(1.0),
    offset(0.0), block_size(0)
{
} /* SigLevDetDdr::SigLevDetDdr */


SigLevDetDdr::~SigLevDetDdr(void)
{
} /* SigLevDetDdr::~SigLevDetDdr */


bool SigLevDetDdr::initialize(Config &cfg, const string& name, int sample_rate)
{
  Ddr *ddr = Ddr::find(name);
  if (ddr == 0)
  {
    cout << "*** ERROR: Could not find a DDR named \"" << name << "\". "
         << "Cannot use DDR signal level detector.\n";
    return false;
  }
  ddr->preDemod.connect(mem_fun(*this, &SigLevDetDdr::processSamples));

  this->sample_rate = sample_rate;

  cfg.getValue(name, "SIGLEV_OFFSET", offset);
  cfg.getValue(name, "SIGLEV_SLOPE", slope);

  block_size = BLOCK_LENGTH * sample_rate / 1000;

  reset();

  return SigLevDet::initialize(cfg, name, sample_rate);
  
} /* SigLevDetDdr::initialize */


void SigLevDetDdr::reset(void)
{
  block_idx = 0;
  last_siglev = 0.0f;
  update_counter = 0;
  siglev_values.clear();
  pwr_sum = 0.0;
} /* SigLevDetDdr::reset */


void SigLevDetDdr::setContinuousUpdateInterval(int interval_ms)
{
  update_interval = interval_ms * sample_rate / 1000;
  update_counter = 0;  
} /* SigLevDetDdr::setContinuousUpdateInterval */


void SigLevDetDdr::setIntegrationTime(int time_ms)
{
    // Calculate the integration time expressed as the
    // number of processing blocks.
  integration_time = time_ms * 16000 / 1000 / block_size;
  if (integration_time <= 0)
  {
    integration_time = 1;
  }
} /* SigLevDetDdr::setIntegrationTime */


float SigLevDetDdr::siglevIntegrated(void) const
{
  if (siglev_values.size() > 0)
  {
    float sum = 0;
    deque<float>::const_iterator it;
    for (it=siglev_values.begin(); it!=siglev_values.end(); ++it)
    {
      sum += *it;
    }
    return sum / siglev_values.size();
  }
  return 0;
} /* SigLevDetDdr::siglevIntegrated */



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

void SigLevDetDdr::processSamples(const vector<RtlTcp::Sample> &samples)
{
  for (vector<RtlTcp::Sample>::const_iterator it = samples.begin();
       it != samples.end();
       ++it)
  {
    pwr_sum += it->real() * it->real() + it->imag() * it->imag();
    if (++block_idx == block_size)
    {
      last_siglev = offset + slope * 10.0 * log10(pwr_sum / block_size);
      siglev_values.push_back(last_siglev);
      if (siglev_values.size() > integration_time)
      {
	siglev_values.erase(siglev_values.begin(),
		siglev_values.begin()+siglev_values.size()-integration_time);
      }
      
      if (update_interval > 0)
      {
	update_counter += block_size;
	if (update_counter >= update_interval)
	{
	  signalLevelUpdated(siglevIntegrated());
	  update_counter = 0;
	}
      }

      block_idx = 0;
      pwr_sum = 0.0;
    }
  }
} /* SigLevDetDdr::processSamples */



/*
 * This file has not been truncated
 */

