/**
@file	 AsyncAudioProcessor.cpp
@brief   A_brief_description_for_this_file
@author  Tobias Blomberg / SM0SVX
@date	 2006-04-23

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
#include <algorithm>

#include <cstring>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncApplication.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "AsyncAudioProcessor.h"



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

AudioProcessor::AudioProcessor(void)
  : buf_cnt(0), do_flush(false), input_stopped(false),
    output_stopped(false), input_rate(1), output_rate(1), input_buf(0),
    input_buf_cnt(0), input_buf_size(0)
{
  
} /* AudioProcessor::AudioProcessor */


AudioProcessor::~AudioProcessor(void)
{
  delete [] input_buf;
} /* AudioProcessor::~AudioProcessor */


int AudioProcessor::writeSamples(const float *samples, int len)
{
  //cout << "AudioProcessor::writeSamples: len=" << len << endl;
  
  assert(len > 0);
  
  do_flush = false;
  int orig_len = len;
  
  writeFromBuf();

    // Calculate the maximum number of samples we are able to process
  int max_proc = (BUFSIZE - buf_cnt) * input_rate / output_rate;
  if (max_proc == 0)
  {
    input_stopped = true;
    return 0;
  }
  
    // If we have samples in the input buffer, try to fill it up and process it
  if (input_buf_cnt > 0)
  {
    int copy_cnt = min(input_buf_size-input_buf_cnt, len);
    memcpy(input_buf + input_buf_cnt, samples, copy_cnt * sizeof(*input_buf));
    samples += copy_cnt;
    len -= copy_cnt;
    input_buf_cnt += copy_cnt;
    
    if (input_buf_cnt == input_buf_size)
    {
      processSamples(buf + buf_cnt, input_buf, input_buf_size);
      buf_cnt += 1;
      max_proc -= input_buf_size;
      input_buf_cnt = 0;
    }
  }
  
  int reminder = 0;
  if (input_buf_size > 0)
  {
    reminder = len % input_buf_size;
  }
  
  int proc_cnt = min(max_proc, len-reminder);
  if (proc_cnt > 0)
  {
    processSamples(buf + buf_cnt, samples, proc_cnt);
    buf_cnt += proc_cnt * output_rate / input_rate;
    samples += proc_cnt;
    len -= proc_cnt;
    writeFromBuf();
  }

  if ((len > 0) && (len < input_buf_size))
  {
    memcpy(input_buf, samples, len * sizeof(*input_buf));
    input_buf_cnt = len;
    len = 0;
  }
  
  int ret_len = orig_len - len;
  if (ret_len == 0)
  {
    input_stopped = true;
  }
  
  return ret_len;

} /* AudioProcessor::writeSamples */


void AudioProcessor::flushSamples(void)
{
  //cout << "AudioProcessor::flushSamples" << endl;
  
  do_flush = true;
  input_stopped = false;
  if (buf_cnt == 0)
  {
    if (input_buf_cnt > 0)
    {
      memset(input_buf + input_buf_cnt, 0,
      	     (input_buf_size - input_buf_cnt) * sizeof(*input_buf));
      processSamples(buf, input_buf, input_buf_size);
      buf_cnt += 1;
      input_buf_cnt = 0;
      writeFromBuf();
    }
    else
    {
      do_flush = false;
      sinkFlushSamples();
    }
  }
} /* AudioProcessor::flushSamples */


void AudioProcessor::resumeOutput(void)
{
  //cout << "AudioProcessor::resumeOutput" << endl;
  output_stopped = false;
  writeFromBuf();
} /* AudioProcessor::resumeOutput */


void AudioProcessor::allSamplesFlushed(void)
{
  //cout << "AudioProcessor::allSamplesFlushed" << endl;
  do_flush = false;
  sourceAllSamplesFlushed();
} /* AudioProcessor::allSamplesFlushed */


void AudioProcessor::setInputOutputSampleRate(int input_rate, int output_rate)
{
  assert((input_rate % output_rate == 0) || (output_rate % input_rate == 0));
  this->input_rate = input_rate;
  this->output_rate = output_rate;
  
  delete [] input_buf;
  if (input_rate > output_rate)
  {
    input_buf_size = input_rate / output_rate;
    input_buf = new float[input_buf_size];
  }
  else
  {
    input_buf_size = 0;
    input_buf = 0;
  }
} /* AudioProcessor::setSampleRateRatio */



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
 * Method:    AudioProcessor::writeFromBuf
 * Purpose:   Write processed samples from the buffer to the connected sink.
 * Input:     None
 * Output:    None
 * Author:    Tobias Blomberg / SM0SVX
 * Created:   2006-04-23
 * Remarks:   
 * Bugs:      
 *----------------------------------------------------------------------------
 */
void AudioProcessor::writeFromBuf(void)
{
  if ((buf_cnt == 0) || output_stopped)
  {
    return;
  }
  
  int written;
  do
  {
    written = sinkWriteSamples(buf, buf_cnt);
    assert((written >= 0) && (written <= buf_cnt));
    if (written > 0)
    {
      buf_cnt -= written;
      if (buf_cnt > 0)
      {
        memmove(buf, buf+written, buf_cnt * sizeof(*buf));
      }
    }

    if (do_flush && (buf_cnt == 0))
    {
      if (input_buf_cnt > 0)
      {
	memset(input_buf + input_buf_cnt, 0,
      	       (input_buf_size - input_buf_cnt) * sizeof(*input_buf));
	processSamples(buf, input_buf, input_buf_size);
	buf_cnt += 1;
	input_buf_cnt = 0;
      }
      else
      {
        do_flush = false;
        Application::app().runTask(
            mem_fun(*this, &AudioProcessor::sinkFlushSamples));
      }
    }
  }
  while ((written > 0) && (buf_cnt > 0));
    
  output_stopped = (written == 0);

  if (input_stopped && (buf_cnt < BUFSIZE))
  {
    input_stopped = false;
    Application::app().runTask(
		    mem_fun(*this, &AudioProcessor::sourceResumeOutput));
  }
} /* AudioProcessor::writeFromBuf */


/*
 * This file has not been truncated
 */

