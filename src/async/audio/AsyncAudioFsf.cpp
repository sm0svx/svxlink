/**
@file	 AsyncAudioFsf.cpp
@brief   Frequency Sampling Filter
@author  Tobias Blomberg / SM0SVX
@date	 2018-01-03

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2003-2018 Tobias Blomberg / SM0SVX

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
#include <cstring>
#include <cassert>
#include <iostream>


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

#include "AsyncAudioFsf.h"



/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/

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

namespace Async
{
  class AudioFsf::CombFilter
  {
    public:
      CombFilter(size_t N, float r)
        : N(N), r_fact(-std::pow(r, N)), pos(0)
      {
        delay = new float[N];
        std::memset(delay, 0, sizeof(*delay)*N);
        //std::cout << "### CombFilter: N=" << N << " r_fact=" << r_fact
        //          << std::endl;
      }
  
      ~CombFilter(void)
      {
        delete [] delay;
        delay = 0;
      }
  
      inline float processSample(const float& src)
      {
        float dest = src + delay[pos] * r_fact;
        delay[pos] = src;
        pos = (pos == N-1) ? 0 : pos + 1;
        return dest;
      }
  
    private:
      const size_t  N;
      const float   r_fact;
      float *       delay;
      size_t        pos;
  
      CombFilter(const CombFilter&);
      CombFilter& operator=(const CombFilter&);
  
  }; /* AudioFsf::CombFilter */


  class AudioFsf::Resonator
  {
    public:
      Resonator(const size_t N, const size_t k, const float r, const float H)
        : gain(H), coeff1(2.0*r*cos(2.0*M_PI*k/N)), coeff2(-r*r),
          z1(0.0), z2(0.0)
      {
        gain /= N;
        if ((k == 0) || (k == N/2))
        {
          gain /= 2.0;
        }
        if (k % 2 == 1)
        {
          gain = -gain;
        }
        //std::cout << "### Resonator: N=" << N << " k=" << k << " r=" << r
        //          << " H=" << H << " gain=" << gain
        //          << " coeff1=" << coeff1 << " coeff2=" << coeff2 << std::endl;
      }

      inline float processSample(const float& src)
      {
        float dest = src + z1*coeff1 + z2*coeff2;
        z2 = z1;
        z1 = dest;
        return dest * gain;
      }

    private:
      float       gain;
      const float coeff1;
      const float coeff2;
      float       z1;
      float       z2;
  };
};


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

AudioFsf::AudioFsf(const size_t N, const float *coeff, const float r)
{
  assert(N % 2 == 0);
  assert((r >= 0.0) && (r <= 1.0));
  m_combN = new CombFilter(N, r);
  m_comb2 = new CombFilter(2, r);
  for (size_t k=0; k<=N/2; ++k)
  {
    float H = coeff[k];
    if (H > 0.0f)
    {
      Resonator *res = new Resonator(N, k, r, H);
      m_resonators.push_back(res);
    }
  }
} /* AudioFsf::AudioFsf */


AudioFsf::~AudioFsf(void)
{
  for (std::vector<Resonator*>::iterator it=m_resonators.begin();
       it!=m_resonators.end();
       ++it)
  {
    delete *it;
  }
  m_resonators.clear();
  delete m_comb2;
  m_comb2 = 0;
  delete m_combN;
  m_combN = 0;
} /* AudioFsf::~AudioFsf */


/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/

void AudioFsf::processSamples(float *dest, const float *src, int count)
{
  for (int i=0; i<count; ++i)
  {
    float destN = m_combN->processSample(src[i]);
    float dest2 = m_comb2->processSample(destN);
    dest[i] = 0.0f;
    for (std::vector<Resonator*>::iterator it=m_resonators.begin();
         it!=m_resonators.end();
         ++it)
    {
      dest[i] += (*it)->processSample(dest2);
    }
  }
} /* AudioFsf::processSamples */


/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/



/*
 * This file has not been truncated
 */

