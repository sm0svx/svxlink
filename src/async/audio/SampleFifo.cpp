/**
@file	 SampleFifo.cpp
@brief   A FIFO for handling samples
@author  Tobias Blomberg / SM0SVX
@date	 2004-03-07

A_detailed_description_for_this_file

\verbatim
<A brief description of the program or library this file belongs to>
Copyright (C) 2003 Tobias Blomberg / SM0SVX

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

#include "SampleFifo.h"



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

static const int  MAX_WRITE_SIZE = 800;


/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/


SampleFifo::SampleFifo(int fifo_size)
  : fifo_size(fifo_size), head(0), tail(0), is_stopped(false),
    do_overwrite(false), write_buffer_is_full(false), prebuf_samples(0),
    prebuf(true), do_flush(false)
{
  fifo = new float[fifo_size];
} /* SampleFifo */


SampleFifo::~SampleFifo(void)
{
  delete [] fifo;
} /* ~SampleFifo */


int SampleFifo::addSamples(float *samples, int count)
{
  //printf("SampleFifo::addSamples: count=%d is_stopped=%s  empty=%s\n",
    //  	  count, is_stopped ? "true" : "false", empty() ? "true" : "false");
  
  if (do_flush)
  {
    //printf("SampleFifo::addSamples(%s): do_flush=false\n", debug_name.c_str());
    do_flush = false;
  }
  
  int samples_written = 0;
  if (!is_stopped && empty() && !prebuf)
  {
    samples_written = writeSamples(samples, count);
    //printf("SampleFifo::addSamples(%s): samples_to_write=%d "
    //  	   "samples_written=%d\n", debug_name.c_str(), count, samples_written);
  }
  
  while (samples_written < count)
  {
    int next_head = (head < fifo_size-1) ? head + 1 : 0;
    if (next_head == tail)
    {
      if (do_overwrite)
      {
      	tail = (tail < fifo_size-1) ? tail + 1 : 0;
      }
      else
      {
      	fifoFull(true);
      	break;
      }
    }
    fifo[head] = samples[samples_written++];
    head = next_head;
  }
  
  if (prebuf && (samplesInFifo() >= prebuf_samples))
  {
    writeSamplesFromFifo();
  }
  
  return samples_written;
  
} /* addSamples */


void SampleFifo::stopOutput(bool stop)
{
  /*
  printf("SampleFifo::stopOutput: stop=%s  is_stopped=%s\n",
      stop ? "true" : "false", is_stopped ? "true" : "false");
  */
    
  if (stop == is_stopped)
  {
    return;
  }
  
  is_stopped = stop;
  
  if (!stop)
  {
    writeSamplesFromFifo();
  }
    
} /* stopOutput */


bool SampleFifo::full(void) const
{
  return !do_overwrite && (((head + 1) % fifo_size) == tail);
} /* full */


unsigned SampleFifo::samplesInFifo(bool ignore_prebuf) const
{
  unsigned samples_in_buffer = (head - tail + fifo_size) % fifo_size;

  if (ignore_prebuf && prebuf && !do_flush)
  {
    if (samples_in_buffer < prebuf_samples)
    {
      return 0;
    }
  }

  return samples_in_buffer;

} /* SampleFifo::samplesInFifo */


void SampleFifo::writeBufferFull(bool is_full)
{
  //printf("SampleFifo::writeBufferFull: is_full=%s\n",
  //    	  is_full ? "true" : "false");
  
  write_buffer_is_full = is_full;
  
  if (!is_full && !is_stopped && !empty())
  {
    writeSamplesFromFifo();
  }
} /* writeBufferFull */


int SampleFifo::readSamples(float *samples, int count)
{
  if (count <= 0)
  {
    return 0;
  }
  
  if (prebuf && !do_flush)
  {
    if (samplesInFifo() < prebuf_samples)
    {
      return 0;
    }
    prebuf = false;
  }

  int was_full = full();
  
  int tot_samples_read = 0;
  do
  {
    int samples_to_read = min((unsigned)count, samplesInFifo());
    int to_end_of_fifo = fifo_size - tail;
    samples_to_read = min(samples_to_read, to_end_of_fifo);
    memcpy(samples+tot_samples_read, fifo+tail,
      	    samples_to_read * sizeof(float));
    tail = (tail + samples_to_read) % fifo_size;
    count -= samples_to_read;
    tot_samples_read += samples_to_read;
    //printf("SampleFifo::readSamples: samples_to_read=%d "
      //	   "tot_samples_read=%d\n", samples_to_read, tot_samples_read);
  } while((count > 0) && !empty());
  
  if (was_full)
  {
    //printf("Clearing FIFO full condition\n");
    fifoFull(false);
    if (empty())
    {
      allSamplesWritten();
    }
  }
  else
  {
    allSamplesWritten();
  }
  
  return tot_samples_read;
  
} /* SampleFifo::readSamples */


void SampleFifo::setPrebufSamples(unsigned prebuf_samples)
{
  this->prebuf_samples = prebuf_samples;
} /* SampleFifo::setPrebufSamples */


void SampleFifo::flushSamples(void)
{
  //printf("SampleFifo::flushSamples\n");
  //printf("SampleFifo::flushSamples(%s): do_flush=true\n", debug_name.c_str());
  do_flush = true;
  prebuf = true;
  if (!empty())
  {
    writeSamplesFromFifo();
  }
  else
  {
    allSamplesWritten();
  }
} /* SampleFifo::flushSamples */



/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/


/*
 *------------------------------------------------------------------------
 * Method:    
 * Purpose:   
 * Input:     
 * Output:    
 * Author:    
 * Created:   
 * Remarks:   
 * Bugs:      
 *------------------------------------------------------------------------
 */






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

 

void SampleFifo::writeSamplesFromFifo(void)
{
  if (empty() || write_buffer_is_full || is_stopped)
  {
    return;
  }
  
  if (prebuf && !do_flush)
  {
    if (samplesInFifo() < prebuf_samples)
    {
      return;
    }
    prebuf = false;
  }
  
  int was_full = full();
  
  int samples_to_write = 0;
  int samples_written;
  do
  {
    int samples_in_fifo = (head + fifo_size - tail) % fifo_size;
    samples_to_write = min(MAX_WRITE_SIZE, samples_in_fifo);
    int to_end_of_fifo = fifo_size - tail;
    samples_to_write = min(samples_to_write, to_end_of_fifo);
    samples_written = writeSamples(fifo+tail, samples_to_write);
    //printf("SampleFifo::writeSamplesFromFifo(%s): samples_to_write=%d "
    //  	   "samples_written=%d\n", debug_name.c_str(), samples_to_write,
	//   samples_written);
    tail = (tail + samples_written) % fifo_size;
  } while((samples_to_write == samples_written) && !empty() &&
      	  !write_buffer_is_full);

  /*  
  if (was_full)
  {
    fifoFull(false);
    if (empty())
    {
      allSamplesWritten();
    }
  }
  else
  {
    allSamplesWritten();
  }
  */
  
  if (was_full && !full())
  {
    fifoFull(false);
  }
  
  if (do_flush && empty())
  {
    allSamplesWritten();
    //printf("SampleFifo::writeSamplesFromFifo(%s): do_flush=false\n",
    //  	   debug_name.c_str());
    do_flush = false;
  }
  
} /* writeSamplesFromFifo */






/*
 * This file has not been truncated
 */

