/**
@file	 SigLevDet.cpp
@brief   A simple signal level detector
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

#include "SigLevDet.h"



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

SigLevDet::SigLevDet(int sample_rate)
  : slope(1.0), offset(0.0)
{
  if (sample_rate >= 16000)
  {
    filter = new AudioFilter("HpBu4/6000", sample_rate);
  }
  else
  {
    filter = new AudioFilter("HpBu4/3500", sample_rate);
  }
  setHandler(filter);
  sigc_sink = new SigCAudioSink;
  sigc_sink->sigWriteSamples.connect(slot(*this, &SigLevDet::processSamples));
  sigc_sink->sigFlushSamples.connect(
      slot(*sigc_sink, &SigCAudioSink::allSamplesFlushed));
  sigc_sink->registerSource(filter);
} /* SigLevDet::SigLevDet */


SigLevDet::~SigLevDet(void)
{
  clearHandler();
  delete filter;
  delete sigc_sink;
} /* SigLevDet::~SigLevDet */


void SigLevDet::reset(void)
{
  filter->reset();
} /* SigLevDet::reset */



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

int SigLevDet::processSamples(float *samples, int count)
{
  //cout << "SigLevDet::processSamples: count=" << count << "\n";
  double rms = 0.0;
  for (int i=0; i<count; ++i)
  {
    float sample = samples[i];
    rms += sample * sample;
  }
  last_siglev = sqrt(rms / count);
  
  return count;
  
} /* SigLevDet::processSamples */






/*
 * This file has not been truncated
 */

