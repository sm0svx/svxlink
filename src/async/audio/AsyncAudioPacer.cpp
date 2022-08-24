/**
@file	 AudioPacer.cpp
@brief   An audio pipe class for pacing audio output
@author  Tobias Blomberg / SM0SVX
@date	 2004-04-03

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

#include <stdio.h>

#include <algorithm>
#include <cstring>
#include <cassert>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncTimer.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "AsyncAudioPacer.h"



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


AudioPacer::AudioPacer(int sample_rate, int block_size, int prebuf_time)
  : sample_rate(sample_rate), buf_size(block_size), prebuf_time(prebuf_time),
    buf_pos(0), pace_timer(0), do_flush(false), input_stopped(false)
{
  assert(sample_rate > 0);
  assert(block_size > 0);
  assert(prebuf_time >= 0);
  
  buf = new float[buf_size];
  prebuf_samples = prebuf_time * sample_rate / 1000;
  
  pace_timer = new Timer(buf_size * 1000 / sample_rate,
       	      	      	 Timer::TYPE_PERIODIC);
  pace_timer->expired.connect(mem_fun(*this, &AudioPacer::outputNextBlock));
  
  if (prebuf_samples > 0)
  {
    pace_timer->setEnable(false);
  }
  
} /* AudioPacer::AudioPacer */


AudioPacer::~AudioPacer(void)
{
  delete pace_timer;
  delete [] buf;
} /* AudioPacer::~AudioPacer */


int AudioPacer::writeSamples(const float *samples, int count)
{
  assert(count > 0);
  
  if (do_flush)
  {
    do_flush = false;
  }
  
  int samples_written = 0;
  
  if (prebuf_samples > 0)
  {
    prebuf_samples -= count;
    if (prebuf_samples <= 0)
    {
      	// Prebuffering done.
      	// Write incoming samples to sink, excluding samples that go
	// beyond prebuffering (Note: prebuf_samples is zero or negative here).
      samples_written = sinkWriteSamples(samples, count + prebuf_samples);
      
      int samples_left = count - samples_written;
      if (samples_left > 0)
      {
      	  // Make a recursive call to write remaining samples into the buffer.
	samples_written += writeSamples(samples + samples_written,
	      	      	      	      	samples_left);
      }
      pace_timer->setEnable(true);
    }
    else
    {
      samples_written = sinkWriteSamples(samples, count);
      if (samples_written < count)
      {
        prebuf_samples += count - samples_written;
      }
    }
  }
  else
  {
    samples_written = min(count, buf_size - buf_pos);
    memcpy(buf + buf_pos, samples, samples_written * sizeof(*buf));
    buf_pos += samples_written;
    
    if (!pace_timer->isEnabled())
    {
      pace_timer->setEnable(true);
    }
  }
  
  if (samples_written == 0)
  {
    input_stopped = true;
  }
  
  return samples_written;
  
} /* AudioPacer::writeSamples */


void AudioPacer::flushSamples(void)
{
  input_stopped = false;
  do_flush = true;
  
  if (buf_pos == 0)
  {
    sinkFlushSamples();
  }
} /* AudioPacer::flushSamples */


void AudioPacer::resumeOutput(void)
{
  if (prebuf_samples <= 0)
  {
    pace_timer->setEnable(true);
    outputNextBlock();
  }
} /* AudioPacer::resumeOutput */
    


/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/

void AudioPacer::allSamplesFlushed(void)
{
  if (do_flush)
  {
    do_flush = false;
    sourceAllSamplesFlushed();
  }
} /* AudioPacer::allSamplesFlushed */



/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/


void AudioPacer::outputNextBlock(Timer *t)
{
  if (buf_pos < buf_size)
  {
    pace_timer->setEnable(false);
    prebuf_samples = prebuf_time * sample_rate / 1000;
  }
  
  if (buf_pos == 0)
  {
    return;
  }
  
  int samples_to_write = buf_pos;
  int tot_samples_written = 0;
  int samples_written;
  do {
    samples_written = sinkWriteSamples(buf + tot_samples_written,
      	      	      	      	       samples_to_write);
    tot_samples_written += samples_written;
    samples_to_write -= samples_written;
  } while ((samples_written > 0) && (samples_to_write > 0));

  if (tot_samples_written < buf_pos)
  {
    memmove(buf, buf + tot_samples_written,
      	    (buf_pos - tot_samples_written) * sizeof(*buf));
    buf_pos -= tot_samples_written;
  }
  else
  {
    buf_pos = 0;
  }
  
  if (samples_written == 0)
  {
    pace_timer->setEnable(false);
  }
  
  if (input_stopped && (buf_pos < buf_size))
  {
    input_stopped = false;
    sourceResumeOutput();
  }
  
  if (do_flush && (buf_pos == 0))
  {
    sinkFlushSamples();
  }
  
} /* AudioPacer::outputNextBlock */




/*
 * This file has not been truncated
 */

