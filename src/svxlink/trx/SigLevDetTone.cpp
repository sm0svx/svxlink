/**
@file	 SigLevDetTone.cpp
@brief   A signal level detector using tone in the 5.5 to 6.5kHz band
@author  Tobias Blomberg / SM0SVX
@date	 2009-05-23

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2009 Tobias Blomberg / SM0SVX

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
#include <algorithm>
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

#include "SigLevDetTone.h"
#include "Goertzel.h"



/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/

using namespace std;



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

static int tone_siglev_map[10] =
{
  100, 90, 80, 70, 60, 50, 40, 30, 20, 10
};


/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/

SigLevDetTone::SigLevDetTone(void)
  : block_idx(0), last_siglev(0)
{
  for (int i=0; i<10; ++i)
  {
    det[i] = new Goertzel(5500 + i * 100, 16000);
  }
} /* SigLevDetTone::SigLevDetTone */


SigLevDetTone::~SigLevDetTone(void)
{
  for (int i=0; i<10; ++i)
  {
    delete det[i];
  }
} /* SigLevDetTone::~SigLevDetTone */


bool SigLevDetTone::initialize(Async::Config &cfg, const std::string& name)
{
  string mapstr;
  if (cfg.getValue(name, "TONE_SIGLEV_MAP", mapstr))
  {
    string::iterator comma;
    string::iterator begin = mapstr.begin();
    int i = 0;
    do
    {
      comma = find(begin, mapstr.end(), ',');
      string value;
      if (comma == mapstr.end())
      {
	value = string(begin, mapstr.end());
      }
      else
      {
	value = string(begin, comma);
	begin = comma + 1;
      }
      
      tone_siglev_map[i++] = atoi(value.c_str());
    } while ((comma != mapstr.end()) && (i < 10));
  }
  
  return true;
  
} /* SigLevDetTone::initialize */


void SigLevDetTone::reset(void)
{
  for (int i=0; i<10; ++i)
  {
    det[i]->reset();
    block_idx = 0;
  }
  last_siglev = 0;
} /* SigLevDetTone::reset */


int SigLevDetTone::writeSamples(const float *samples, int count)
{
  for (int i=0; i<count; ++i)
  {
    for (int detno=0; detno < 10; ++detno)
    {
      det[detno]->calc(samples[i]);
    }
    
    if (++block_idx == 160)
    {
      block_idx = 0;
      
      float max = 0.0f;
      float prev_max = 0.0f;
      int max_idx = -1;
      for (int detno=0; detno < 10; ++detno)
      {
        float res = det[detno]->result();
        det[detno]->reset();
        if (res >= max)
        {
          prev_max = max;
          max = res;
          max_idx = detno;
        }
        else if (res > prev_max)
        {
          prev_max = res;
        }
      }
      
      last_siglev = 0;
      if (max > 0.1f)
      {
        float snr = 10.0f * log10f(max / prev_max);
        if (snr > 10.0f)
        {
          last_siglev = tone_siglev_map[max_idx];
          printf("fq=%d  max=%.2f  prev_max=%.2f  snr=%.2f  siglev=%d\n",
                5500 + max_idx * 100, max, prev_max, snr, last_siglev);
        }
      }
    }
  }
  
  return count;
  
} /* SigLevDetTone::writeSamples */


void SigLevDetTone::flushSamples(void)
{
  sourceAllSamplesFlushed();
} /* SigLevDetTone::flushSamples */



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



/*
 * This file has not been truncated
 */

