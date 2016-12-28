/**
@file	 AsyncAudioEncoderGsm.cpp
@brief   An audio encoder that encodes to GSM
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

#include "AsyncAudioEncoderGsm.h"



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

AudioEncoderGsm::AudioEncoderGsm(void)
  : gsmh(0), gsm_buf_len(0)
{
  gsmh = gsm_create();
} /* AsyncAudioEncoderGsm::AsyncAudioEncoderGsm */


AudioEncoderGsm::~AudioEncoderGsm(void)
{
  gsm_destroy(gsmh);
  gsmh = 0;
} /* AsyncAudioEncoderGsm::~AsyncAudioEncoderGsm */


int AudioEncoderGsm::writeSamples(const float *samples, int count)
{
  for (int i=0; i<count; ++i)
  {
    float sample = samples[i];
    if (sample > 1.0)
    {
      gsm_buf[gsm_buf_len++] = 32767;
    }
    else if (sample < -1.0)
    {
      gsm_buf[gsm_buf_len++] = -32767;
    }
    else
    {
      gsm_buf[gsm_buf_len++] = static_cast<gsm_signal>(sample * 32767.0);
    }
    
    if (gsm_buf_len == GSM_BUF_SIZE)
    {
      gsm_buf_len = 0;

      gsm_frame frame[FRAME_COUNT];
      for (int frameno=0; frameno<FRAME_COUNT; ++frameno)
      {
        gsm_encode(gsmh, gsm_buf + frameno * FRAME_SAMPLE_CNT, frame[frameno]);
      }
      
      writeEncodedSamples(frame, FRAME_COUNT * sizeof(gsm_frame));
    }
  }
  
  
  return count;
  
} /* AudioEncoderGsm::writeSamples */




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

