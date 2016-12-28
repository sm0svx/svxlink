/**
@file	 AsyncAudioEncoderS16.cpp
@brief   An audio encoder that encodes samples to signed 16 bit samples
@author  Tobias Blomberg / SM0SVX
@date	 2008-10-06

\verbatim
Async - A library for programming event driven applications
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

#include <stdint.h>


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

#include "AsyncAudioEncoderS16.h"



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

#if 0
AudioEncoderS16::AudioEncoderS16(void)
{
  
} /* AsyncAudioEncoderS16::AsyncAudioEncoderS16 */


AudioEncoderS16::~AudioEncoderS16(void)
{
  
} /* AsyncAudioEncoderS16::~AsyncAudioEncoderS16 */
#endif


int AudioEncoderS16::writeSamples(const float *samples, int count)
{
  int16_t s16_samples[count];
  
  for (int i=0; i<count; ++i)
  {
    float sample = samples[i];
    if (sample > 1.0)
    {
      s16_samples[i] = 32767;
    }
    else if (sample < -1.0)
    {
      s16_samples[i] = -32767;
    }
    else
    {
      s16_samples[i] = static_cast<int16_t>(sample * 32767.0);
    }
  }
  
  writeEncodedSamples(s16_samples, count * sizeof(*s16_samples));
  
  return count;
  
} /* AudioEncoderS16::writeSamples */




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

