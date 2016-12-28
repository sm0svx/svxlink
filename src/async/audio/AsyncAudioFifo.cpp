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
    do_overwrite(false), output_stopped(false), prebuf_samples(0),
    prebuf(false), is_flushing(false), is_full(false), buffering_enabled(true),
    disable_buffering_when_flushed(false), is_idle(true), input_stopped(false)
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


void AudioFifo::clear(void)
{
  bool was_empty = empty();
  
  is_full = false;
  tail = head = 0;
  prebuf = (prebuf_samples > 0);
  output_stopped = false;
  
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
      if (input_stopped)
      {
      	sourceResumeOutput();
      }
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
  
  assert(count > 0);
  
  is_idle = false;
  is_flushing = false;
  
  if (is_full)
  {
    input_stopped = true;
    return 0;
  }
  
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
      while (!is_full && (samples_written < count))
      {
	fifo[head] = samples[samples_written++];
	head = (head < fifo_size-1) ? head + 1 : 0;
	if (head == tail)
	{
	  if (do_overwrite)
	  {
      	    tail = (tail < fifo_size-1) ? tail + 1 : 0;
	  }
	  else
	  {
	    is_full = true;
	  }
	}
      }
      
      if (prebuf && (samplesInFifo() > 0))
      {
      	prebuf = false;
      }

      writeSamplesFromFifo();
    }
  }
  else
  {
    output_stopped = (samples_written == 0);
  }

  input_stopped = (samples_written == 0);
  
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
    writeSamplesFromFifo();
  }
} /* AudioFifo::flushSamples */


void AudioFifo::resumeOutput(void)
{
  if (output_stopped)
  {
    output_stopped = false;
    if (buffering_enabled)
    {
      writeSamplesFromFifo();
    }
    else if (input_stopped)
    {
      sourceResumeOutput();
    }
  }
} /* resumeOutput */



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


void AudioFifo::writeSamplesFromFifo(void)
{
  if (output_stopped || (samplesInFifo() == 0))
  {
    return;
  }
  
  bool was_full = full();
  
  int samples_written;
  do
  {
    int samples_to_write = min(MAX_WRITE_SIZE, samplesInFifo(true));
    int to_end_of_fifo = fifo_size - tail;
    samples_to_write = min(samples_to_write, to_end_of_fifo);
    samples_written = sinkWriteSamples(fifo+tail, samples_to_write);
    //printf("AudioFifo::writeSamplesFromFifo(%s): samples_to_write=%d "
    //  	   "samples_written=%d\n", debug_name.c_str(), samples_to_write,
	//   samples_written);
    if (was_full && (samples_written > 0))
    {
      is_full = false;
      was_full = false;
    }
    tail = (tail + samples_written) % fifo_size;
  } while((samples_written > 0) && !empty());
  
  if (samples_written == 0)
  {
    output_stopped = true;
  }
  
  if (input_stopped && !full())
  {
    input_stopped = false;
    sourceResumeOutput();
  }
  
  if (is_flushing && empty())
  {
    sinkFlushSamples();
  }
  
} /* writeSamplesFromFifo */



/*
 * This file has not been truncated
 */
