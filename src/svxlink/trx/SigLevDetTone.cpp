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

#include <iostream>
//#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cmath>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncConfig.h>
#include <common.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "SigLevDetTone.h"
//#include "Goertzel.h"



/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/

using namespace std;
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

/**
@brief  An implementation of the Goertzel single bin DFT algorithm
@author Tobias Blomberg / SM0SVX
@date   2009-05-23
*/
class SigLevDetTone::MyGoertzel
{
  public:
    /**
     * @brief   Default constuctor
     * @param   freq        The frequency of interest, in Hz
     * @param   sample_rate The sample rate used
     */
    MyGoertzel(float freq, unsigned sample_rate)
    {
      coeff = 2.0f * cosf(2.0f * M_PI * (freq / (float)sample_rate));
      reset();
    }

    /**
     * @brief   Destructor
     */
    ~MyGoertzel(void) {}

    /**
     * @brief   Reset the state variables
     */
    void reset(void)
    {
      q1 = q2 = 0.0f;
    }

    /**
     * @brief   Call this function for each sample in a block
     * @param   sample A sample
     */
    void calc(float sample)
    {
      float q0 = coeff * q1 - q2 + sample;
      q2 = q1;
      q1 = q0;
    }

    /**
     * @brief   Read back the result after calling "calc" for a whole block
     * @return  Returns the relative magnitude squared
     */
    float magnitudeSquared(void)
    {
      return q1*q1 + q2*q2 - q1*q2*coeff;
    }


  protected:

  private:
    float q1;
    float q2;
    float coeff;

    MyGoertzel(const MyGoertzel&);
    MyGoertzel& operator=(const MyGoertzel&);

};  /* class MyGoertzel */


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
  : tone_siglev_map(10), block_idx(0), last_siglev(0) 
{
  hwin = new HammingWindow(BLOCK_SIZE);
  for (int i=0; i<10; ++i)
  {
    det[i] = new MyGoertzel(5500 + i * 100, 16000);
    tone_siglev_map[i] = 100 - i * 10;
  }
} /* SigLevDetTone::SigLevDetTone */


SigLevDetTone::~SigLevDetTone(void)
{
  for (int i=0; i<10; ++i)
  {
    delete det[i];
  }
  delete hwin;
} /* SigLevDetTone::~SigLevDetTone */


bool SigLevDetTone::initialize(Async::Config &cfg, const std::string& name)
{
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
  
  return true;
  
} /* SigLevDetTone::initialize */


void SigLevDetTone::reset(void)
{
  for (int i=0; i<10; ++i)
  {
    det[i]->reset();
  }
  hwin->reset();
  block_idx = 0;
  last_siglev = 0;
} /* SigLevDetTone::reset */


int SigLevDetTone::writeSamples(const float *samples, int count)
{
  for (int i=0; i<count; ++i)
  {
    float sample = hwin->calc(samples[i]);
    for (int detno=0; detno < 10; ++detno)
    {
      det[detno]->calc(sample);
    }
    
    if (++block_idx == BLOCK_SIZE)
    {
      block_idx = 0;
      hwin->reset();
      
      float max = 0.0f;
      float sum = 0.0f;
      int max_idx = -1;
      for (int detno=0; detno < 10; ++detno)
      {
        float res = det[detno]->magnitudeSquared();
        det[detno]->reset();
        if (res >= max)
        {
          max = res;
          max_idx = detno;
        }
        sum += res;
      }
      float mean = (sum - max) / 9.0f;

      last_siglev = 0;
      if (max > 0.1f)
      {
        float snr = 5.0f * log10f(max / mean);
        if (snr > 8.0f)
        {
          last_siglev = tone_siglev_map[max_idx];
          //printf("fq=%d  max=%.2f  mean=%.2f  snr=%.2f  siglev=%d\n",
          //      5500 + max_idx * 100, max, mean, snr, last_siglev);
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

