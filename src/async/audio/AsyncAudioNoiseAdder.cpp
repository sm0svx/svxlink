/**
@file	 AsyncAudioNoiseAdder.cpp
@brief   A class to add white noise to an audio stream
@author  Tobias Blomberg / SM0SVX
@date	 2015-03-08

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2003-2015 Tobias Blomberg / SM0SVX

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

#include <cstring>
#include <cstdlib>
#include <cmath>
#include <locale>

#include <cstdlib>


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

#include "AsyncAudioNoiseAdder.h"


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

AudioNoiseAdder::AudioNoiseAdder(float max_ampl)
  : seed(0), max_ampl(max_ampl)
{
} /* AudioNoiseAdder::AudioNoiseAdder */


AudioNoiseAdder::~AudioNoiseAdder(void)
{
} /* AudioNoiseAdder::~AudioNoiseAdder */



/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/

void AudioNoiseAdder::processSamples(float *dest, const float *src, int count)
{
  //cout << "AudioNoiseAdder::processSamples: len=" << len << endl;
  
  for (int i=0; i<count; ++i)
  {
    float noise = 2.0 * rand_r(&seed) / RAND_MAX - 1.0;
    dest[i] = src[i] + max_ampl * noise;
  }
} /* AudioNoiseAdder::writeSamples */



/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/


/*
 * This file has not been truncated
 */
