/**
@file	 SigLevDetSim.cpp
@brief   A simulated signal level detector
@author  Tobias Blomberg / SM0SVX
@date	 2015-10-03

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

#include "SigLevDetSim.h"


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

unsigned int SigLevDetSim::next_seed = 0;


/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/

SigLevDetSim::SigLevDetSim(void)
  : sample_rate(0), block_idx(0), last_siglev(0), integration_time(1),
    update_interval(0), update_counter(0), siglev_toggle_interval(0),
    siglev_toggle_counter(0), siglev_rand_interval(0), siglev_rand_counter(0),
    block_size(0), siglev_min(0.0f), siglev_max(100.0f), siglev_default(0.0f),
    seed(next_seed++)
{
} /* SigLevDetSim::SigLevDetSim */


SigLevDetSim::~SigLevDetSim(void)
{
} /* SigLevDetSim::~SigLevDetSim */


bool SigLevDetSim::initialize(Config &cfg, const string& name, int sample_rate)
{
  this->sample_rate = sample_rate;
  block_size = BLOCK_LENGTH * sample_rate / 1000;

  cfg.getValue(name, "SIGLEV_MIN", siglev_min);
  cfg.getValue(name, "SIGLEV_MAX", siglev_max);
  cfg.getValue(name, "SIGLEV_DEFAULT", siglev_default);

  unsigned siglev_toggle_interval_ms = 0;
  cfg.getValue(name, "SIGLEV_TOGGLE_INTERVAL", siglev_toggle_interval_ms);
  siglev_toggle_interval = siglev_toggle_interval_ms * sample_rate / 1000;

  int siglev_rand_interval_ms = 0;
  cfg.getValue(name, "SIGLEV_RAND_INTERVAL", siglev_rand_interval_ms);
  siglev_rand_interval = siglev_rand_interval_ms * sample_rate / 1000;

  reset();

  return SigLevDet::initialize(cfg, name, sample_rate);
  
} /* SigLevDetSim::initialize */


void SigLevDetSim::reset(void)
{
  block_idx = 0;
  last_siglev = siglev_default;
  update_counter = 0;
  siglev_toggle_counter = 0;
  siglev_rand_counter = 0;
  siglev_values.clear();
} /* SigLevDetSim::reset */


void SigLevDetSim::setContinuousUpdateInterval(int interval_ms)
{
  update_interval = interval_ms * sample_rate / 1000;
  update_counter = 0;  
} /* SigLevDetSim::setContinuousUpdateInterval */


void SigLevDetSim::setIntegrationTime(int time_ms)
{
    // Calculate the integration time expressed as the
    // number of processing blocks.
  integration_time = time_ms * sample_rate / 1000 / block_size;
  if (integration_time <= 0)
  {
    integration_time = 1;
  }
} /* SigLevDetSim::setIntegrationTime */


float SigLevDetSim::siglevIntegrated(void) const
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
} /* SigLevDetSim::siglevIntegrated */



/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/

int SigLevDetSim::writeSamples(const float *samples, int count)
{
  for (int i=0; i<count; ++i)
  {
    if (siglev_rand_interval > 0)
    {
      if (++siglev_rand_counter >= siglev_rand_interval)
      {
        siglev_rand_counter = 0;
        randNewSiglev();
      }
    }

    if (siglev_toggle_interval > 0)
    {
      if (++siglev_toggle_counter >= siglev_toggle_interval)
      {
        siglev_toggle_counter = 0;
        toggleSiglev();
      }
    }

    if (++block_idx == block_size)
    {
      block_idx = 0;
      siglev_values.push_back(last_siglev);
      if (siglev_values.size() > integration_time)
      {
	siglev_values.erase(siglev_values.begin(),
		siglev_values.begin()+siglev_values.size()-integration_time);
      }
    }

    if (update_interval > 0)
    {
      if (++update_counter >= update_interval)
      {
        update_counter = 0;
        signalLevelUpdated(siglevIntegrated());
      }
    }
  }

  return count;
} /* SigLevDetSim::writeSamples */


/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/

void SigLevDetSim::randNewSiglev(void)
{
  if ((rand_r(&seed) > RAND_MAX/2) && (last_siglev < siglev_max))
  {
    last_siglev += 1.0f;
  }
  else if (last_siglev > siglev_min)
  {
    last_siglev -= 1.0f;
  }
} /* SigLevDetSim::randNewSiglev */


void SigLevDetSim::toggleSiglev(void)
{
  if (last_siglev == siglev_min)
  {
    last_siglev = siglev_max;
  }
  else
  {
    last_siglev = siglev_min;
  }
} /* SigLevDetSim::toggleSiglev */


/*
 * This file has not been truncated
 */
