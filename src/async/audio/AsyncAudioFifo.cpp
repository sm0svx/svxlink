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



/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/


AudioFifo::AudioFifo(unsigned fifo_size)
  : fifo_size(fifo_size), head(0), tail(0), stream_state(STREAM_IDLE),
    is_full(false), buffering_when_empty(true), buffering_enabled(true)
{
  fifo = new float[fifo_size];
  AudioSource::setHandler(&valve);
  sigsrc.registerSink(&valve);
  sigsrc.sigRequestSamples.connect(mem_fun(*this, &AudioFifo::onRequestSamples));
  sigsrc.sigAllSamplesFlushed.connect(mem_fun(*this, &AudioFifo::onAllSamplesFlushed));
} /* AudioFifo */


AudioFifo::~AudioFifo(void)
{
  AudioSource::clearHandler();
  delete [] fifo;
} /* ~AudioFifo */


void AudioFifo::setSize(unsigned new_size)
{
  if (new_size != fifo_size)
  {
    delete [] fifo;
    fifo_size = new_size;
    fifo = new float[fifo_size];
  }
  clear();
} /* AudioFifo::setSize */


unsigned AudioFifo::samplesInFifo() const
{
  if (fifo_size == 0) return 0;
  return is_full ? fifo_size : (head - tail + fifo_size) % fifo_size;

} /* AudioFifo::samplesInFifo */


unsigned AudioFifo::spaceAvail() const
{
  return fifo_size - samplesInFifo();

} /* AudioFifo::spaceAvail */


void AudioFifo::clear(void)
{
  bool was_empty = empty();
  
  tail = head = 0;
  is_full = false;
  
  if ((stream_state == STREAM_FLUSHING) && !was_empty)
  {
    sigsrc.flushSamples();
  }

} /* AudioFifo::clear */


void AudioFifo::enableBuffering(bool enable)
{
  buffering_when_empty = enable;
  
  if (enable)
  {
    buffering_enabled = true;
  }  
  else if (empty())
  {
    buffering_enabled = false;
  }

} /* AudioFifo::enableBuffering */


int AudioFifo::writeSamples(const float *samples, int count)
{
  stream_state = STREAM_ACTIVE;

  int written = 0;
  if (empty())
  {
    if (valve.isOpen())
    {
      written = sigsrc.writeSamples(samples, count);
    }
    else if (!buffering_enabled || (fifo_size == 0))
    {
      return count;
    }
  }

  if (buffering_enabled && (fifo_size > 0))
  {  
    if (valve.isOpen())
    {
      writeSamplesFromFifo(count - written);
    }

    while (written < count)
    {
      fifo[head] = samples[written++];
      head = (head + 1) % fifo_size;
        
      /* FIFO is full */
      if (head == tail)
      {
        is_full = true;
        
        if (valve.isOpen())
        {
          /* If output is enabled, we try to write */
          /* the FIFO content to the sink. */
          writeSamplesFromFifo(count - written);
        }
        else
        {
          /* If output is disabled, we try to overwrite. */
          tail = (tail + 1) % fifo_size;
          is_full = false;
        }
      }
    }
  }

  return written;
  
} /* writeSamples */


void AudioFifo::availSamples(void)
{
  stream_state = STREAM_ACTIVE;
  sigsrc.availSamples();
} /* AudioFifo::availSamples */


void AudioFifo::flushSamples(void)
{
  //printf("AudioFifo::flushSamples\n");
  stream_state = STREAM_FLUSHING;
  if (empty())
  {
    sigsrc.flushSamples();
  }
  else if (valve.isOpen())
  {
    flushSamplesFromFifo();
  }
  
} /* AudioFifo::flushSamples */


/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/

void AudioFifo::onRequestSamples(int count)
{
  int written = writeSamplesFromFifo(count);
  if ((stream_state == STREAM_ACTIVE) && (written < count))
  {
    sourceRequestSamples(count - written);
  }

} /* AudioFifo::onRequestSamples */


void AudioFifo::onAllSamplesFlushed(void)
{
  if (stream_state == STREAM_FLUSHING)
  {
    stream_state = STREAM_IDLE;
    sourceAllSamplesFlushed();
  }

} /* AudioFifo::onAllSamplesFlushed */



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
    unsigned ret = sigsrc.writeSamples(fifo + tail, to_end_of_fifo);
    
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

  if (empty())
  {
    buffering_enabled = buffering_when_empty;
    if (stream_state == STREAM_FLUSHING)
    {
      sigsrc.flushSamples();
    }
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
    unsigned ret = sigsrc.writeSamples(fifo + tail, to_end_of_fifo);
    
    tail += ret;
    tail %= fifo_size;
    samples_from_fifo -= ret;
    is_full &= (ret == 0);

    if (ret < to_end_of_fifo)
    {
      break;
    }
  }

  if (empty())
  {
    buffering_enabled = buffering_when_empty;
    if (stream_state == STREAM_FLUSHING)
    {
      sigsrc.flushSamples();
    }
  }

} /* flushSamplesFromFifo */


/*
 * This file has not been truncated
 */
