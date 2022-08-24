/**
@file	 SquelchVox.cpp
@brief   Implementes a voice activated squelch
@author  Tobias Blomberg
@date	 2004-02-15

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2010 Tobias Blomberg / SM0SVX

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

#include <stdio.h>
#include <cstdlib>
#include <iostream>
#include <cmath>


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

#include "SquelchVox.h"



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


SquelchVox::SquelchVox(void)
  : buf(0), buf_size(0), head(0), sum(0), up_thresh(0), down_thresh(0)
{
} /* SquelchVox::SquelchVox */


SquelchVox::~SquelchVox(void)
{
  delete [] buf;
  buf = 0;
} /* SquelchVox::~SquelchVox */


bool SquelchVox::initialize(Config& cfg, const string& rx_name)
{
  if (!Squelch::initialize(cfg, rx_name))
  {
    return false;
  }

  string value;
  if (!cfg.getValue(rx_name, "VOX_FILTER_DEPTH", value))
  {
    cerr << "*** ERROR: Config variable " << rx_name
      	 << "/VOX_FILTER_DEPTH not set\n";
    return false;
  }
  buf_size = INTERNAL_SAMPLE_RATE * atoi(value.c_str()) / 1000;
  buf = new float[buf_size];
  for (int i=0; i<buf_size; ++i)
  {
    buf[i] = 0;
  }

  short vox_thresh = 0;
  if (!cfg.getValue(rx_name, "VOX_THRESH", vox_thresh))
  {
    if (cfg.getValue(rx_name, "VOX_LIMIT", vox_thresh))
    {
      cerr << "*** WARNING: Configuration variable " << rx_name
	   << "/VOX_LIMIT is deprecated. Use VOX_THRESH instead.\n";
    }
    else
    {
      cerr << "*** ERROR: Config variable " << rx_name
	   << "/VOX_THRESH not set\n";
      return false;
    }
  }
  setVoxThreshold(vox_thresh);

  if (cfg.getValue(rx_name, "VOX_START_DELAY", value))
  {
    cerr << "*** ERROR: VOX_START_DELAY changed its name to SQL_START_DELAY in "
            "configuration of " << rx_name << "\n";
    return false;
  }

  return true;

} /* SquelchVox::initialize */


void SquelchVox::setVoxThreshold(short thresh)
{
  up_thresh = pow(thresh / 10000.0, 2) * buf_size;
  down_thresh = pow(thresh / 10000.0, 2) * buf_size;
} /* SquelchVox::setVoxThreshold */


void SquelchVox::reset(void)
{
  for (int i=0; i<buf_size; ++i)
  {
    buf[i] = 0;
  }
  sum = 0;
  head = 0;
  Squelch::reset();
} /* SquelchVox::reset */



/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/

int SquelchVox::processSamples(const float *samples, int count)
{
  int start_pos = 0;
  if (buf_size < count)
  {
    start_pos = count - buf_size;
  }

  for (int i=start_pos; i<count; ++i)
  {
    sum -= buf[head];
    buf[head] = samples[i] * samples[i];
    sum += buf[head];
    head = (head >= buf_size-1) ? 0 : head + 1;
  }

  bool opened = !signalDetected() && (sum >= up_thresh);
  bool closed = signalDetected() && (sum < down_thresh);
  if (opened || closed)
  {
    std::ostringstream ss;
    ss << static_cast<int>(std::round(10000 * std::sqrt(sum / buf_size)));
    setSignalDetected(opened, ss.str());
  }

  return count;

} /* SquelchVox::processSamples */



/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/



/*
 * This file has not been truncated
 */
