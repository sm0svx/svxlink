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

#include <iostream>
#include <cassert>


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


AudioPacer::AudioPacer(unsigned sample_rate, unsigned block_size, unsigned prebuf_time)
  : sample_rate(sample_rate), buf_size(0), head(0), tail(0),
    output_samples(0), pace_timer(0), is_flushing(false), is_full(false), 
    prebuf(true)
{
  assert(sample_rate > 0);
  assert((sample_rate % 1000) == 0);
  assert(block_size > 0);
  
  prebuf_samples = prebuf_time * sample_rate / 1000;
  buf_size = 2 * block_size + prebuf_samples;
  
  buf = new float[buf_size];
  
  pace_timer = new Timer(block_size * 1000 / sample_rate, Timer::TYPE_PERIODIC);
  pace_timer->expired.connect(slot(*this, &AudioPacer::outputNextBlock));
  pace_timer->setEnable(false);
  
  timerclear(&output_start);
    
} /* AudioPacer::AudioPacer */


AudioPacer::~AudioPacer(void)
{
  delete pace_timer;
  delete [] buf;
} /* AudioPacer::~AudioPacer */


int AudioPacer::writeSamples(const float *samples, int count)
{
  if (is_flushing)
  {
    is_flushing = false;
    prebuf = true;
  }

  int samples_written = 0;
  while (!is_full && (samples_written < count))
  {
    buf[head] = samples[samples_written++];
    head = (head + 1) % buf_size;
    /* buffer is full */
    if (head == tail)
    {
      is_full = true;
    }
  }
  
  if ((samplesInBuffer() > 0) && prebuf)
  {
    gettimeofday(&output_start, NULL);

    prebuf = false;
    output_samples = 0;

    pace_timer->setEnable(true);
  }

  return samples_written;
 
} /* AudioPacer::writeSamples */


void AudioPacer::flushSamples(void)
{
  is_flushing = true;
  
  if (empty())
  {
    sinkFlushSamples();
  }
} /* AudioPacer::flushSamples */


void AudioPacer::requestSamples(int count)
{
  if (!is_flushing)
  {
    sourceRequestSamples(buf_size - samplesInBuffer(true));
  }
  pace_timer->setEnable(true);
  outputSamplesFromBuffer(count);
} /* AudioPacer::requestSamples */
    


/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/

void AudioPacer::allSamplesFlushed(void)
{
  if (empty())
  {
    if (is_flushing)
    {
      is_flushing = false;
      sourceAllSamplesFlushed();
    }
    prebuf = true;
    pace_timer->setEnable(false);
  }
} /* AudioPacer::allSamplesFlushed */



/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/


void AudioPacer::outputNextBlock(Timer *t)
{
  //printf("AudioPacer::outputNextBlock\n");
  struct timeval now, diff;
  
  gettimeofday(&now, NULL);
  timersub(&now, &output_start, &diff);

  /* elapsed time (ms) since output has been started */  
  uint64_t diff_ms = diff.tv_sec * 1000 + diff.tv_usec / 1000;
  
  /* the output is modeled as a constant rate sample source */
  int count = sample_rate * diff_ms / 1000 - output_samples;

  // cerr << " block: " << count
  //      << " buffer: " << samplesInBuffer()
  //      << endl;

  /* output the block */
  outputSamplesFromBuffer(count);

  /* check amount of available samples */
  sourceRequestSamples(buf_size - samplesInBuffer());

} /* AudioPacer::outputNextBlock */


void AudioPacer::outputSamplesFromBuffer(int count)
{
  //printf("AudioPacer::outputSamplesFromBuffer %d\n", count);
  unsigned samples_from_buffer = min(count, samplesInBuffer());

  while (samples_from_buffer > 0)
  {
    unsigned to_end_of_buffer = min(samples_from_buffer, buf_size - tail);
    unsigned ret = sinkWriteSamples(buf + tail, to_end_of_buffer);
    
    tail += ret;
    tail %= buf_size;
    output_samples += ret;
    samples_from_buffer -= ret;
    is_full &= (ret == 0);
    
    if (ret < to_end_of_buffer)
    {
      pace_timer->setEnable(false);
      break;
    }
  }

  if (empty())
  {
    pace_timer->setEnable(false);

    if (is_flushing)
    {
      sinkFlushSamples();
    }

    prebuf = true;
  }

} /* AudioPacer::outputSamplesFromBuffer */


int AudioPacer::samplesInBuffer(bool ignore_prebuf)
{
  unsigned samples_in_buffer = is_full ? buf_size :
    (head - tail + buf_size) % buf_size;

  if (!ignore_prebuf && prebuf && !is_flushing)
  {
    if (samples_in_buffer < prebuf_samples)
    {
      return 0;
    }
  }

  return samples_in_buffer;

} /* AudioPacer::samplesInBuffer */


/*
 * This file has not been truncated
 */

