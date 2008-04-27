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
    buf_pos(0), do_flush(false), input_stopped(false)
{
  buf = new float[buf_size];
  prebuf_samples = prebuf_time * sample_rate / 1000;
} /* AudioPacer::AudioPacer */


AudioPacer::~AudioPacer(void)
{
  delete [] buf;
} /* AudioPacer::~AudioPacer */


int AudioPacer::writeSamples(const float *samples, int count)
{
  //printf("AudioPacer::writeSamples: count=%d\n", count);

  if (count <= 0)
  {
    return 0;
  }

  int samples_written = 0;  
  
  if (do_flush)
  {
    //printf("AudioPacer::writeSamples: do_flush=false\n");
    do_flush = false;
  }
  
  if (prebuf_samples > 0)
  {
    prebuf_samples -= count;
    if (prebuf_samples <= 0)
    {
      samples_written = count;
      samples_written += prebuf_samples;
      samples_written = sinkWriteSamples(samples, samples_written);
      samples_written += writeSamples(samples + samples_written,
      	      	      	      	      count - samples_written);
      pace_timer = new Timer(buf_size * 1000 / sample_rate,
      	      	      	     Timer::TYPE_PERIODIC);
      pace_timer->expired.connect(slot(*this, &AudioPacer::outputNextBlock));
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
  }
  
  //printf("%d samples of %d written into paser\n", samples_written, count);
  
  if (samples_written == 0)
  {
    input_stopped = true;
  }
  
  return samples_written;
  
} /* AudioPacer::writeSamples */


void AudioPacer::flushSamples(void)
{
  //printf("AudioPacer::flushSamples: buf_pos=%d\n", buf_pos);
  
  input_stopped = false;

  if (buf_pos == 0)
  {
    //printf("AudioPacer::flushSamples: Calling sinkFlushSamples()\n");
    sinkFlushSamples();
  }
  else
  {
    //printf("AudioPacer::flushSamples: do_flush=true\n");
    do_flush = true;
  }
} /* AudioPacer::flushSamples */


void AudioPacer::resumeOutput(void)
{

} /* AudioPacer::resumeOutput */
    


/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/

void AudioPacer::allSamplesFlushed(void)
{
  sourceAllSamplesFlushed();
} /* AudioPacer::allSamplesFlushed */



/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/


/*
 *----------------------------------------------------------------------------
 * Method:    
 * Purpose:   
 * Input:     
 * Output:    
 * Author:    
 * Created:   
 * Remarks:   
 * Bugs:      
 *----------------------------------------------------------------------------
 */
void AudioPacer::outputNextBlock(Timer *t)
{
  //printf("Timer expired\n");
  if (buf_pos < buf_size)
  {
    delete pace_timer;
    pace_timer = 0;
    prebuf_samples = prebuf_time * sample_rate / 1000;
    //printf("AudioPacer::outputNextBlock: Turning on prebuffering...\n");
  }
  
  if (buf_pos == 0)
  {
    return;
  }
  
  //printf("AudioPacer::outputNextBlock: Calling sinkWriteSamples()\n");
  int samples_written = sinkWriteSamples(buf, buf_pos);
  if (samples_written < buf_pos)
  {
    memmove(buf, buf+samples_written, (buf_pos - samples_written) * sizeof(*buf));
    buf_pos -= samples_written;
  }
  else
  {
    buf_pos = 0;
  }

  if (input_stopped && (buf_pos < buf_size))
  {
    input_stopped = false;
    sourceResumeOutput();
  }
  
  if (do_flush && (buf_pos == 0))
  {
    do_flush = false;
    //printf("AudioPacer::outputNextBlock: Calling sinkFlushSamples()\n");
    sinkFlushSamples();
  }
  
} /* AudioPacer::outputNextBlock */




/*
 * This file has not been truncated
 */

