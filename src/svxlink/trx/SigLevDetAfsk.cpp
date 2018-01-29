/**
@file	 SigLevDetAfsk.cpp
@brief   Signal level "detector" reading signal strengths from the data stream
@author  Tobias Blomberg / SM0SVX
@date	 2009-05-23

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2013 Tobias Blomberg / SM0SVX

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


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncConfig.h>
#include <AsyncAudioSource.h>
#include <Synchronizer.h>
#include <HdlcDeframer.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "SigLevDetAfsk.h"
#include "Rx.h"
#include "Tx.h"


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

SigLevDetAfsk::SigLevDetAfsk(void)
  : last_siglev(0), timeout_timer(3500)
{
  timeout_timer.setEnable(false);
  timeout_timer.expired.connect(mem_fun(*this, &SigLevDetAfsk::timeout));
} /* SigLevDetAfsk::SigLevDetAfsk */


SigLevDetAfsk::~SigLevDetAfsk(void)
{

} /* SigLevDetAfsk::~SigLevDetAfsk */


void SigLevDetAfsk::reset(void)
{
  last_siglev = 0;
} /* SigLevDetAfsk::reset */


void SigLevDetAfsk::setContinuousUpdateInterval(int interval_ms)
{
  /*
  update_interval = interval_ms * sample_rate / 1000;
  update_counter = 0;
  */
} /* SigLevDetAfsk::setContinuousUpdateInterval */


void SigLevDetAfsk::setIntegrationTime(int time_ms)
{
    // Calculate the integration time expressed as the
    // number of processing blocks.
  /*
  integration_time = time_ms * 16000 / 1000 / BLOCK_SIZE;
  if (integration_time <= 0)
  {
    integration_time = 1;
  }
  */
} /* SigLevDetAfsk::setIntegrationTime */


float SigLevDetAfsk::siglevIntegrated(void) const
{
  /*
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
  */
  return last_siglev;
} /* SigLevDetAfsk::siglevIntegrated */


int SigLevDetAfsk::writeSamples(const float *samples, int len)
{
  return len;
} /* SigLevDetAfsk::writeSamples */


void SigLevDetAfsk::flushSamples(void)
{
  sourceAllSamplesFlushed();
} /* SigLevDetAfsk::flushSamples */


void SigLevDetAfsk::frameReceived(vector<uint8_t> frame)
{
  uint8_t cmd = frame[0];
  if (cmd != Tx::DATA_CMD_SIGLEV)
  {
    return;
  }

  if (frame.size() < 3)
  {
    cerr << "*** WARNING: Illegal Siglev data frame received\n";
    return;
  }

  char rxid = frame[1] & 0x7f;
  if ((rxid < '!') || (rxid > '~'))
  {
    rxid = Rx::ID_UNKNOWN;
  }

  uint8_t siglev = frame[2];
  cout << "### SigLevDetAfsk::frameReceived: len=" << frame.size()
       << " cmd=" << (int)cmd
       << " rxid=" << rxid
       << " siglev=" << (int)siglev
       << endl;
  last_siglev = siglev;
  updateRxId(rxid);
  signalLevelUpdated(siglev);

  if (timeout_timer.isEnabled())
  {
    if (siglev == 0)
    {
      timeout_timer.setEnable(false);
    }
    else
    {
      timeout_timer.reset();
    }
  }
  else if (siglev > 0)
  {
    timeout_timer.setEnable(true);
  }
} /* SigLevDetAfsk::frameReceived */



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

void SigLevDetAfsk::timeout(Timer *t)
{
  last_siglev = 0;
  signalLevelUpdated(0);
} /* SigLevDetAfsk::timeout */



/*
 * This file has not been truncated
 */
