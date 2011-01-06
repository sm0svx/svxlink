/**
@file	 AsyncAudioJitterFifo.cpp
@brief   A FIFO for handling audio samples
@author  Tobias Blomberg / SM0SVX
@date	 2007-10-06

Implements a jitter-tolerant FIFO for storing samples.

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2003-2009  Tobias Blomberg / SM0SVX

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

#include "AsyncAudioJitterFifo.h"



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

AudioJitterFifo::AudioJitterFifo(unsigned sample_rate, unsigned fifo_size)
  : sample_rate(sample_rate), fifo(0), fifo_size(fifo_size), head(0),
    tail(0), prebuf(true), is_flushing(false), output_samples(0)
{
  assert(sample_rate > 0);
  assert((sample_rate % 1000) == 0);
  assert(fifo_size > 0);
  fifo = new float[fifo_size];
  timerclear(&output_start);
} /* AudioJitterFifo */


AudioJitterFifo::~AudioJitterFifo(void)
{
  delete [] fifo;
} /* ~AudioJitterFifo */


void AudioJitterFifo::setSize(unsigned new_size)
{
  assert(fifo_size > 0);
  if (new_size != fifo_size)
  {
    delete [] fifo;
    fifo_size = new_size;
    fifo = new float[fifo_size];
  }
  clear();
} /* AudioJitterFifo::setSize */


unsigned AudioJitterFifo::samplesInFifo(bool ignore_prebuf) const
{
  unsigned samples_in_buffer = (head - tail + fifo_size) % fifo_size;

  if (!ignore_prebuf && prebuf && !is_flushing)
  {
    if (samples_in_buffer < (fifo_size >> 1))
    {
      return 0;
    }
  }

  return samples_in_buffer;

} /* AudioJitterFifo::samplesInFifo */


void AudioJitterFifo::clear(void)
{
  bool was_empty = empty();
  
  tail = head = 0;
  prebuf = true;
  
  if (is_flushing && !was_empty)
  {
    sinkFlushSamples();
  }
} /* AudioJitterFifo::clear */


int AudioJitterFifo::writeSamples(const float *samples, int count)
{
  assert(count > 0);
  
  if (is_flushing)
  {
    is_flushing = false;
    prebuf = true;
  }

  int samples_written = 0;
  while (samples_written < count)
  {
    fifo[head] = samples[samples_written++];
    head = (head + 1) % fifo_size;
    if (head == tail)
    {
        // Throw away the first half of the buffer.
      tail = (tail + (fifo_size >> 1)) % fifo_size;
    }
  }

  if (samplesInFifo() > 0 && prebuf)
  {
    gettimeofday(&output_start, NULL);
    output_samples = 0;
    prebuf = false;
  }

  /* When we push samples into the pipe, we have to limit the pushed */
  /* sample rate. Otherwise, the FIFO space is used inefficiently. */
  if (!prebuf)
  {  
    struct timeval now, diff;
  
    gettimeofday(&now, NULL);
    timersub(&now, &output_start, &diff);

    /* elapsed time (ms) since output has been started */  
    uint64_t diff_ms = diff.tv_sec * 1000 + diff.tv_usec / 1000;
  
    /* the output is modeled as a constant rate sample source */
    writeSamplesFromFifo(sample_rate * diff_ms / 1000 - output_samples);
  }

  return samples_written;
  
} /* writeSamples */


void AudioJitterFifo::flushSamples(void)
{
  is_flushing = true;
  if (empty())
  {
    sinkFlushSamples();
  }
  else
  {
    flushSamplesFromFifo();
  }
} /* AudioJitterFifo::flushSamples */


void AudioJitterFifo::requestSamples(int count)
{
  int avail_samples = samplesInFifo(true);
  if (!is_flushing && (count > avail_samples))
  {
    sourceRequestSamples(prebuf ? (fifo_size >> 1) + count - avail_samples :
      count - avail_samples);
  }
  writeSamplesFromFifo(count);
} /* requestSamples */



/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/


void AudioJitterFifo::allSamplesFlushed(void)
{
  if (empty())
  {
    if (is_flushing)
    {
      is_flushing = false;
      sourceAllSamplesFlushed();
    }
    prebuf = true;
  }
} /* AudioJitterFifo::allSamplesFlushed */



/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/


void AudioJitterFifo::writeSamplesFromFifo(int count)
{
  unsigned samples_from_fifo = min((unsigned)count, samplesInFifo());

  //cerr << "samples_from_fifo=" << samples_from_fifo << " " << samplesInFifo() << endl;

  while (samples_from_fifo > 0)
  {
    unsigned to_end_of_fifo = min(samples_from_fifo, fifo_size - tail);
    unsigned ret = sinkWriteSamples(fifo + tail, to_end_of_fifo);
      
    tail += ret;
    tail %= fifo_size;
    output_samples += ret;
    samples_from_fifo -= ret;

    if (ret < to_end_of_fifo)
    {
      break;
    }
  }
  
  if (empty())
  {
    if (is_flushing)
    {
      sinkFlushSamples();
    }
    prebuf = true;
  }
  
} /* writeSamplesFromFifo */


void AudioJitterFifo::flushSamplesFromFifo(void)
{
  unsigned samples_from_fifo = samplesInFifo();

  //cerr << "samples_from_fifo=" << samples_from_fifo << endl;

  while (samples_from_fifo > 0)
  {
    unsigned to_end_of_fifo = min(samples_from_fifo, fifo_size - tail);
    unsigned ret = sinkWriteSamples(fifo + tail, to_end_of_fifo);
    
    tail += ret;
    tail %= fifo_size;
    samples_from_fifo -= ret;

    if (ret < to_end_of_fifo)
    {
      break;
    }
  }

  if (is_flushing && empty())
  {
    sinkFlushSamples();
  }

} /* flushSamplesFromFifo */

/*
 * This file has not been truncated
 */
