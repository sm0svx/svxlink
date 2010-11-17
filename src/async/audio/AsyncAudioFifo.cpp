/**
@file	 AsyncAudioFifo.cpp
@brief   A FIFO for handling audio samples
@author  Tobias Blomberg / SM0SVX
@date	 2007-10-06

Implements a FIFO (with some extra functionality) for storing samples.

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
#include <iostream>
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

#include "AsyncAudioFifo.h"



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


AudioFifo::AudioFifo(unsigned fifo_size)
  : fifo_size(fifo_size), head(0), tail(0),
    do_overwrite(false), prebuf_samples(0), prebuf(false),
    is_flushing(false), is_full(false), buffering_enabled(true),
    disable_buffering_when_flushed(false), is_idle(true)
{
  assert(fifo_size > 0);
  fifo = new float[fifo_size];
} /* AudioFifo */


AudioFifo::~AudioFifo(void)
{
  delete [] fifo;
} /* ~AudioFifo */


void AudioFifo::setSize(unsigned new_size)
{
  assert(fifo_size > 0);
  if (new_size != fifo_size)
  {
    delete [] fifo;
    fifo_size = new_size;
    fifo = new float[fifo_size];
  }
  clear();
} /* AudioFifo::setSize */


unsigned AudioFifo::samplesInFifo(bool ignore_prebuf) const
{
  unsigned samples_in_buffer =
      	  is_full ? fifo_size : (head - tail + fifo_size) % fifo_size;

  if (!ignore_prebuf && prebuf && !is_flushing)
  {
    if (samples_in_buffer < prebuf_samples)
    {
      return 0;
    }
  }

  return samples_in_buffer;

} /* AudioFifo::samplesInFifo */


unsigned AudioFifo::spaceAvail() const
{
  return is_full ? 0 : fifo_size - ((head - tail + fifo_size) % fifo_size);
} /* AudioFifo::spaceAvail */


void AudioFifo::clear(void)
{
  bool was_empty = empty();
  
  is_full = false;
  tail = head = 0;
  prebuf = (prebuf_samples > 0);
  
  if (is_flushing && !was_empty)
  {
    sinkFlushSamples();
  }
} /* AudioFifo::clear */


void AudioFifo::setPrebufSamples(unsigned prebuf_samples)
{
  this->prebuf_samples = min(prebuf_samples, fifo_size-1);
  if (empty())
  {
    prebuf = (prebuf_samples > 0);
  }
} /* AudioFifo::setPrebufSamples */


void AudioFifo::enableBuffering(bool enable)
{
  if (enable)
  {
    disable_buffering_when_flushed = false;
    if (!buffering_enabled)
    {
      buffering_enabled = true;
    }
  }
  else
  {
    if (buffering_enabled)
    {
      if (empty())
      {
      	buffering_enabled = false;
      }
      else
      {
      	disable_buffering_when_flushed = true;
      }
    }
  }
} /* AudioFifo::enableBuffering */


int AudioFifo::writeSamples(const float *samples, int count)
{
  /*
  printf("AudioFifo::writeSamples: count=%d empty=%s  prebuf=%s\n",
      	  count, empty() ? "true" : "false", prebuf ? "true" : "false");
  */
  is_idle = false;
  is_flushing = false;

  flushSamplesFromFifo();
  
  int samples_written = 0;
  if (empty() && !prebuf)
  {
    samples_written = sinkWriteSamples(samples, count);
    /*
    printf("AudioFifo::writeSamples: count=%d "
      	   "samples_written=%d\n", count, samples_written);
    */
  }

  if (buffering_enabled)
  {
    while (!is_full && (samples_written < count))
    {
      fifo[head] = samples[samples_written++];
      head = (head + 1) % fifo_size;
        
      /* FIFO is full */
      if (head == tail)
      {
        is_full = true;
        
        /* First we try to write the FIFO content to the sink. */
        flushSamplesFromFifo();
        
        /* If the FIFO is still full, we try to overwrite. */
        if (is_full && do_overwrite)
        {
      	  tail = (tail + 1) % fifo_size;
      	  is_full = false;
        }
      }

      /* End of pre-buffering phase */
      if (prebuf && (samplesInFifo() > 0))
      {
        prebuf = false;
      }
      
      flushSamplesFromFifo();
    }
  }

  return samples_written;
  
} /* writeSamples */


void AudioFifo::flushSamples(void)
{
  //printf("AudioFifo::flushSamples\n");
  is_flushing = true;
  prebuf = (prebuf_samples > 0);
  if (empty())
  {
    sinkFlushSamples();
  }
  else
  {
    writeSamplesFromFifo(fifo_size);
  }
} /* AudioFifo::flushSamples */


void AudioFifo::requestSamples(int count)
{
  int written = writeSamplesFromFifo(count);
  if (!is_flushing && (written < count))
  {
    sourceRequestSamples(count - written);
  }

} /* requestSamples */



/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/


void AudioFifo::allSamplesFlushed(void)
{
  if (empty())
  {
    if (disable_buffering_when_flushed)
    {
      disable_buffering_when_flushed = false;
      buffering_enabled = false;
    }

    if (is_flushing)
    {
      is_flushing = false;
      sourceAllSamplesFlushed();
    }
  }
} /* AudioFifo::allSamplesFlushed */



/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/


int AudioFifo::writeSamplesFromFifo(int count)
{
  unsigned samples_from_fifo = min((unsigned)count, samplesInFifo());

  //cerr << "samples_from_fifo=" << samples_from_fifo << endl;

  int written = 0;
  while (samples_from_fifo > 0)
  {
    unsigned to_end_of_fifo = min(samples_from_fifo, fifo_size - tail);
    unsigned ret = sinkWriteSamples(fifo + tail, to_end_of_fifo);
    
    tail += ret;
    tail %= fifo_size;
    written += ret;
    samples_from_fifo -= ret;
    is_full &= (ret == 0);

    if (ret < to_end_of_fifo)
    {
      break;
    }
  }

  if (is_flushing && empty())
  {
    sinkFlushSamples();
  }

  return written;
} /* writeSamplesFromFifo */


void AudioFifo::flushSamplesFromFifo(void)
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
    is_full &= (ret == 0);

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
