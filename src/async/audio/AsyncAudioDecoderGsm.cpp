/**
@file	 AudioDecoderGsm.cpp
@brief   An audio decoder for GSM
@author  Tobias Blomberg / SM0SVX
@date	 2008-10-15

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

#include "AsyncAudioDecoderGsm.h"



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

AudioDecoderGsm::AudioDecoderGsm(void)
  : gsmh(0), frame_len(0)
{
  gsmh = gsm_create();
} /* AudioDecoderGsm::AudioDecoderGsm */


AudioDecoderGsm::~AudioDecoderGsm(void)
{
  gsm_destroy(gsmh);
  gsmh = 0;
} /* AudioDecoderGsm::~AudioDecoderGsm */


void AudioDecoderGsm::writeEncodedSamples(void *buf, int size)
{
  unsigned char *ptr = (unsigned char *)buf;
  
  for (int i=0; i<size; ++i)
  {
    frame[frame_len++] = ptr[i];
    if (frame_len == sizeof(frame))
    {
      gsm_signal s16_samples[FRAME_SAMPLE_CNT];
      gsm_decode(gsmh, frame, s16_samples);
    
      float samples[FRAME_SAMPLE_CNT];
      for (int j=0; j<FRAME_SAMPLE_CNT; ++j)
      {
        samples[j] = static_cast<float>(s16_samples[j]) / 32768.0;
      }
      sinkWriteSamples(samples, FRAME_SAMPLE_CNT);
      frame_len = 0;
    }
  }
} /* AudioDecoderGsm::writeEncodedSamples */



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

