/**
@file	  AsyncAudioDelayLine.cpp
@brief	  An audio pipe component to create a delay used for muting
@author	  Tobias Blomberg / SM0SVX
@date	  2006-07-08

\verbatim
Async - A library for programming event driven applications
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

#include <cstring>
#include <cmath>
#include <algorithm>


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

#include "AsyncAudioDelayLine.h"



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

AudioDelayLine::AudioDelayLine(int length_ms)
  : size(length_ms * INTERNAL_SAMPLE_RATE / 1000), ptr(0), flush_cnt(0),
    is_muted(false), mute_cnt(0), last_clear(0), fade_gain(0), fade_len(0),
    fade_pos(0), fade_dir(0)
{
  buf = new float[size];
  memset(buf, 0, size * sizeof(*buf));
  clear();
  setFadeTime(DEFAULT_FADE_TIME);
} /* AudioDelayLine::AudioDelayLine */


AudioDelayLine::~AudioDelayLine(void)
{
  delete [] fade_gain;
  delete [] buf;
} /* AudioDelayLine::~AudioDelayLine */


void AudioDelayLine::setFadeTime(int time_ms)
{
  delete [] fade_gain;
  fade_gain = 0;
  
  if (time_ms <= 0)
  {
    fade_len = 0;
    fade_pos = 0;
    fade_dir = 0;
    return;
  }

  fade_len = time_ms * INTERNAL_SAMPLE_RATE / 1000;
  fade_pos = min(fade_pos, fade_len-1);
  fade_gain = new float[fade_len];
  for (int i=0; i<fade_len-1; ++i)
  {
    fade_gain[i] = pow(2.0f, -15.0f * (static_cast<float>(i) / fade_len));
  }
  fade_gain[fade_len-1] = 0;
} /* AudioDelayLine::setFadeTime  */


void AudioDelayLine::mute(bool do_mute, int time_ms)
{
  int mute_ext = 0;
  if (time_ms > 0)
  {
    mute_ext = min(size, time_ms * INTERNAL_SAMPLE_RATE / 1000);
  }

  if (do_mute)
  {
    fade_pos = 0; // Reset fade gain
    fade_dir = 1; // Fade out
    ptr = (ptr + size - mute_ext) % size;
    for (int i=0; i<mute_ext; ++i)
    {
      ptr = (ptr < size-1) ? ptr+1 : 0;
      buf[ptr] *= currentFadeGain();
    }
    is_muted = true;
    mute_cnt = 0;
  }
  else
  {
    if (mute_ext == 0)
    {
      fade_dir = -1; // Fade in
      is_muted = false;
    }
    else
    {
      mute_cnt = mute_ext;
    }
  }
} /* AudioDelayLine::mute */


void AudioDelayLine::clear(int time_ms)
{
  int count;
  if (time_ms < 0)
  {
    count = size;
  }
  else
  {
    count = min(size, time_ms * INTERNAL_SAMPLE_RATE / 1000);
  }

  //fade_pos = 0; // Reset fade gain
  fade_dir = 1; // Fade out
  ptr = (ptr + size - count) % size;
  for (int i=0; i<count; ++i)
  {
    ptr = (ptr < size-1) ? ptr+1 : 0;
    buf[ptr] *= currentFadeGain();
  }

  if (!is_muted)
  {
    fade_dir = -1; // Fade in again when new samples arrive
  }

  last_clear = max(0, count - fade_len);
  
} /* AudioDelayLine::clear */


int AudioDelayLine::writeSamples(const float *samples, int count)
{
  flush_cnt = 0;
  last_clear = 0;
  
  count = min(count, size);
  float output[count];
  int out_ptr = ptr;
  for (int i=0; i<count; ++i)
  {
    output[i] = buf[out_ptr];
    out_ptr = (out_ptr < size-1) ? out_ptr+1 : 0;
  }

  int written = sinkWriteSamples(output, count);

  for (int i=0; i<written; ++i)
  {
    buf[ptr] = samples[i] * currentFadeGain();
    if (is_muted && (mute_cnt > 0) && (--mute_cnt == 0))
    {
      fade_dir = -1; // Fade in
      is_muted = false;
    }
    ptr = (ptr < size-1) ? ptr+1 : 0;
  }
  
  return written;
  
} /* AudioDelayLine::writeSamples */


void AudioDelayLine::flushSamples(void)
{
  flush_cnt = size - last_clear;
  
  if (flush_cnt > 0)
  {
    writeRemainingSamples();
  }
  else
  {
    sinkFlushSamples();
  }
} /* AudioDelayLine::flushSamples */


void AudioDelayLine::resumeOutput(void)
{
  if (flush_cnt > 0)
  {
    writeRemainingSamples();
  }
  else
  {
    sourceResumeOutput();
  }
} /* AudioDelayLine::resumeOutput */


void AudioDelayLine::allSamplesFlushed(void)
{
  sourceAllSamplesFlushed();
} /* AudioDelayLine::allSamplesFlushed */



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

void AudioDelayLine::writeRemainingSamples(void)
{
  float output[512];
  int written = 1; // Set to 1 so that we enter the loop the first time around

  while ((written > 0) && (flush_cnt > 0))
  {
    int count = min(512, flush_cnt);
    
    int out_ptr = ptr;
    for (int i=0; i<count; ++i)
    {
      output[i] = buf[out_ptr];
      out_ptr = (out_ptr < size-1) ? out_ptr+1 : 0;
    }

    written = sinkWriteSamples(output, count);

    for (int i=0; i<written; ++i)
    {
      buf[ptr] = 0;
      ptr = (ptr < size-1) ? ptr+1 : 0;
    }

    flush_cnt -= written;
  }
  
  if (flush_cnt == 0)
  {
    sinkFlushSamples();
  }
} /* AudioDelayLine::writeRemainingSamples */



/*
 * This file has not been truncated
 */

