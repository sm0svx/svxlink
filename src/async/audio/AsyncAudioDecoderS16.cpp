/**
@file	 AudioDecoderS16.cpp
@brief   An audio decoder for signed 16 bit samples
@author  Tobias Blomberg / SM0SVX
@date	 2008-10-06

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2003-2026 Tobias Blomberg / SM0SVX

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

#include "AsyncAudioDecoderS16.h"



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

AudioDecoderS16::AudioDecoderS16(void)
{
  
} /* AudioDecoderS16::AudioDecoderS16 */


AudioDecoderS16::~AudioDecoderS16(void)
{
  
} /* AudioDecoderS16::~AudioDecoderS16 */


void AudioDecoderS16::writeEncodedSamples(void *buf, int size)
{
  if (size < 0)
  {
    std::cerr << "*** WARNING: Async::AudioDecoderS16 received an encoded "
                 "frame with negative size (" << size << " bytes). "
                 "Discarding it."
              << std::endl;
    return;
  }

    // Decode through a fixed-size stack buffer in chunks to avoid a VLA whose
    // size is controlled by the input.
  static const size_t CHUNK_SAMPLES = 512;
  float samples[CHUNK_SAMPLES];
  size_t pos = 0;
  const size_t count = static_cast<size_t>(size) / sizeof(int16_t);
  const int16_t* const s16_samples = static_cast<int16_t*>(buf);
  while (pos < count)
  {
    size_t n = std::min(count - pos, CHUNK_SAMPLES);
    for (size_t i=0; i<n; ++i)
    {
      samples[i] = static_cast<float>(s16_samples[pos+i]) / 32768.0f;
    }
    sinkWriteSamples(samples, n);
    pos += n;
  }
} /* AudioDecoderS16::writeEncodedSamples */



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

