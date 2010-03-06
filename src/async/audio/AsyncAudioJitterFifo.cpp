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

#include <cstdio>
#include <cstring>
#include <algorithm>
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

static const unsigned  MAX_WRITE_SIZE = 800;


/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/


AudioJitterFifo::AudioJitterFifo(unsigned fifo_size)
  : fifo_size(fifo_size), head(0), tail(0),
    output_stopped(false), prebuf(true), is_flushing(false)
{
  assert(fifo_size > 0);
  fifo = new float[fifo_size];
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


unsigned AudioJitterFifo::samplesInFifo(void) const
{
  unsigned samples_in_buffer = (head - tail + fifo_size) % fifo_size;

  if (prebuf && !is_flushing)
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
  output_stopped = false;
  
  if (is_flushing)
  {
    is_flushing = false;
    if (!was_empty)
    {
      sinkFlushSamples();
    }
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

  if (samplesInFifo() > 0)
  {
    prebuf = false;
  }
  
  writeSamplesFromFifo();

  return samples_written;
  
} /* writeSamples */


void AudioJitterFifo::flushSamples(void)
{
  is_flushing = true;
  if (empty())
  {
    sinkFlushSamples();
  }
} /* AudioJitterFifo::flushSamples */


void AudioJitterFifo::resumeOutput(void)
{
  if (output_stopped)
  {
    output_stopped = false;
    writeSamplesFromFifo();
  }
} /* resumeOutput */



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


void AudioJitterFifo::writeSamplesFromFifo(void)
{
  if (output_stopped)
  {
    return;
  }

  int samples_written;
  if (prebuf && !empty())
  {
    float silence[MAX_WRITE_SIZE];
    for (unsigned i=0; i<MAX_WRITE_SIZE; i++)
    {
      silence[i] = 0;
    }
    unsigned timeout = (fifo_size << 4) / MAX_WRITE_SIZE;
    do
    {
      samples_written = sinkWriteSamples(silence, MAX_WRITE_SIZE);
    } while ((samples_written > 0) && (--timeout));
  }
  else
  {
    do
    {
      int samples_to_write = min(MAX_WRITE_SIZE, samplesInFifo());
      int to_end_of_fifo = fifo_size - tail;
      samples_to_write = min(samples_to_write, to_end_of_fifo);
      samples_written = sinkWriteSamples(fifo+tail, samples_to_write);
      tail = (tail + samples_written) % fifo_size;
    } while((samples_written > 0) && !empty());
  }
  
  if (samples_written == 0)
  {
    output_stopped = true;
  }
  
  if (empty())
  {
    if (is_flushing)
    {
      sinkFlushSamples();
    }
    else
    {
      prebuf = true;
    }
  }
  
} /* writeSamplesFromFifo */



/*
 * This file has not been truncated
 */
