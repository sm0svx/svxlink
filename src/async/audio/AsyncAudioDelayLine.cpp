/**
@file	 AsyncAudioDelayLine.cpp
@brief   A_brief_description_for_this_file
@author  Tobias Blomberg / SM0SVX
@date	 2006-07-08

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

#include "AsyncAudioDelayLine.h"



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

AudioDelayLine::AudioDelayLine(int length_ms)
  : size(length_ms * 8000 / 1000), ptr(0), flush_cnt(0), is_muted(false),
    mute_cnt(0), last_clear(0)
{
  buf = new float[size];
  clear();
} /* AudioDelayLine::AudioDelayLine */


AudioDelayLine::~AudioDelayLine(void)
{
  delete [] buf;
} /* AudioDelayLine::~AudioDelayLine */


void AudioDelayLine::mute(bool do_mute, int time_ms)
{
  if (do_mute)
  {
    int count = min(size, time_ms * 8000 / 1000);
    for (int i=0; i<count; ++i)
    {
      ptr = (ptr > 0) ? ptr-1 : size-1;
      buf[ptr] = 0;
    }
    is_muted = true;
    mute_cnt = 0;
  }
  else
  {
    if (time_ms == 0)
    {
      is_muted = false;
    }
    else
    {
      mute_cnt = time_ms * 8000 / 1000;
    }
  }
} /* AudioDelayLine::mute */


void AudioDelayLine::clear(int time_ms)
{
  int count;
  
  if (time_ms == -1)
  {
    memset(buf, 0, size * sizeof(*buf));
    ptr = 0;
    count = size;
  }
  else
  {
    count = min(size, time_ms * 8000 / 1000);
    for (int i=0; i<count; ++i)
    {
      ptr = (ptr > 0) ? ptr-1 : size-1;
      buf[ptr] = 0;
    }
  }
  
  last_clear = count;
  
} /* AudioDelayLine::clear */


int AudioDelayLine::writeSamples(const float *samples, int count)
{
  float output[count];
  
  flush_cnt = 0;
  last_clear = 0;
  
  for (int i=0; i<count; ++i)
  {
    output[i] = buf[ptr];
    if (!is_muted)
    {
      buf[ptr] = samples[i];
    }
    else
    {
      buf[ptr] = 0;
      if (mute_cnt > 0)
      {
      	if (--mute_cnt == 0)
	{
	  is_muted = false;
	}
      }
    }
    ptr = (ptr < size-1) ? ptr+1 : 0;
  }
  
  int ret = sinkWriteSamples(output, count);
  
  for (int i=count; i>ret; --i)
  {
    ptr = (ptr > 0) ? ptr-1 : size-1;
    buf[ptr] = output[i-1];
  }
  
  return ret;
}


void AudioDelayLine::flushSamples(void)
{
  //printf("AudioDelayLine::flushSamples\n");
  flush_cnt = size - last_clear;
  
  if (flush_cnt > 0)
  {
    writeRemainingSamples();
  }
  else
  {
    sinkFlushSamples();
  }
}


void AudioDelayLine::resumeOutput(void)
{
  //printf("AudioDelayLine::resumeOutput\n");
  if (flush_cnt > 0)
  {
    writeRemainingSamples();
  }
  else
  {
    sourceResumeOutput();
  }
}


void AudioDelayLine::allSamplesFlushed(void)
{
  //printf("AudioDelayLine::allSamplesFlushed\n");
  sourceAllSamplesFlushed();
}



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
void AudioDelayLine::writeRemainingSamples(void)
{
  float output[512];
  int ret;

  do
  {
    int count = min(512, flush_cnt);
    
    for (int i=0; i<count; ++i)
    {
      output[i] = buf[ptr];
      buf[ptr] = 0;
      ptr = (ptr < size-1) ? ptr+1 : 0;
    }

    ret = sinkWriteSamples(output, count);
    //printf("%d samples flushed\n", ret);

    for (int i=count; i>ret; --i)
    {
      ptr = (ptr > 0) ? ptr-1 : size-1;
      buf[ptr] = output[i-1];
    }

    flush_cnt -= ret;
  } while ((ret > 0) && (flush_cnt > 0));
  
  if (flush_cnt == 0)
  {
    sinkFlushSamples();
  }
  
} /* AudioDelayLine::writeRemainingSamples */





/*
 * This file has not been truncated
 */

