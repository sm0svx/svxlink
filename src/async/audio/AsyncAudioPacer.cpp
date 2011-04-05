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
#include <cstring>
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


AudioPacer::AudioPacer(unsigned sample_rate, unsigned block_size)
  : sample_rate(sample_rate), block_size(block_size),
    output_samples(0), pace_timer(0), stream_state(STREAM_IDLE)
{
  assert(sample_rate > 0);
  assert((sample_rate % 1000) == 0);
  assert(block_size > 0);
  
  buf = new float[block_size];
  
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


void AudioPacer::availSamples(void)
{
  if (stream_state != STREAM_ACTIVE)
  {
    stream_state = STREAM_ACTIVE;
    gettimeofday(&output_start, NULL);
    output_samples = 0;
    pace_timer->setEnable(true);
  }
} /* AudioPacer::availSamples */


void AudioPacer::flushSamples(void)
{
  stream_state = STREAM_FLUSHING;
  if (!isReading())
  {
    sinkFlushSamples();
  }
} /* AudioPacer::flushSamples */


void AudioPacer::requestSamples(int count)
{
  cerr << "Error: The AudioPacer does not output samples on request" << endl;
  pace_timer->setEnable(true);
} /* AudioPacer::requestSamples */
    


/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/

void AudioPacer::allSamplesFlushed(void)
{
  if (stream_state == STREAM_FLUSHING)
  {
    stream_state = STREAM_IDLE;
    sourceAllSamplesFlushed();
  }
    
  pace_timer->setEnable(false);
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
  unsigned count = sample_rate * diff_ms / 1000 - output_samples;

  // cerr << " block: " << count
  //      << " buffer: " << samplesInBuffer()
  //      << endl;

  while ((stream_state == STREAM_ACTIVE) && (count > block_size))
  {
    memset(buf, 0, block_size * sizeof(float));
    /* read the block */
    readSamples(buf, block_size);
    /* output the block */
    sinkWriteSamples(buf, block_size);
    /* update counters */
    count -= block_size;
    output_samples += block_size;
  }
  
  if (stream_state == STREAM_FLUSHING)
  {
    sinkFlushSamples();
  }

} /* AudioPacer::outputNextBlock */


/*
 * This file has not been truncated
 */

