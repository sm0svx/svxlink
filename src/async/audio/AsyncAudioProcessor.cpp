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

#include <cstdlib>
#include <iostream>
#include <cstring>


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
  : remain_len(0), target_buf(0), target_size(0), target_len(0),
    input_rate(1), output_rate(1), is_flushing(false)
{
} /* AudioProcessor::AudioProcessor */


AudioProcessor::~AudioProcessor(void)
{
  delete [] target_buf;
} /* AudioProcessor::~AudioProcessor */


int AudioProcessor::writeSamples(const float *samples, int count)
{
  //cout << "AudioProcessor::writeSamples: len=" << count << endl;
  is_flushing = false;
  if (!flushBuffer())
  {
    return 0;
  }

  int len = count + remain_len;

  // Copy source samples into input buffer
  float source_buf[len];
  memcpy(source_buf, remain_buf, remain_len * sizeof(float));
  memcpy(source_buf + remain_len, samples, count * sizeof(float));

  // Note: The output sample rate may be different to the input rate.
  div_t ratio = div(len * output_rate, input_rate);
  target_len = ratio.quot;
  remain_len = ratio.rem;
  
  // Check for sufficient buffer size
  if (target_size < target_len)
  {
    delete [] target_buf;
    target_size = target_len;
    target_buf = new float[target_size];
  }
  
  processSamples(target_buf, source_buf, len - remain_len);
  memcpy(remain_buf, source_buf + len - remain_len, remain_len * sizeof(float));

  target_ptr = target_buf;

  int written = sinkWriteSamples(target_ptr, target_len);
  target_ptr += written;
  target_len -= written;

  return count;
} /* AudioProcessor::writeSamples */


void AudioProcessor::flushSamples(void)
{
  //cout << "AudioProcessor::flushSamples" << endl;
  is_flushing = true;
  if (flushBuffer())
  {
    sinkFlushSamples();
  }
} /* AudioProcessor::flushSamples */


void AudioProcessor::requestSamples(int count)
{
  //cout << "AudioProcessor::requestSamples" << endl;

  if (is_flushing)
  {
    if (flushBuffer())
    {
      sinkFlushSamples();
    }
    return;
  }

  int written = sinkWriteSamples(target_ptr, min(count, target_len));
  target_ptr += written;
  target_len -= written;

  int len = count - written;
  if (len > target_len)
  {
    // Note: We request input samples at input sample rate here.
    div_t ratio = div(len * input_rate, output_rate);
    sourceRequestSamples(ratio.rem ? ratio.quot + 1 : ratio.quot);
  }

} /* AudioProcessor::requestSamples */


void AudioProcessor::allSamplesFlushed(void)
{
  //cout << "AudioProcessor::allSamplesFlushed" << endl;
  is_flushing = false;
  sourceAllSamplesFlushed();
} /* AudioProcessor::allSamplesFlushed */


void AudioProcessor::setInputOutputSampleRate(int input_rate, int output_rate)
{
  assert((input_rate % output_rate == 0) || (output_rate % input_rate == 0));
  this->input_rate = input_rate;
  this->output_rate = output_rate;
} /* AudioProcessor::setSampleRateRatio */



/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/


bool AudioProcessor::flushBuffer(void)
{
  int ret = sinkWriteSamples(target_ptr, target_len);
  target_ptr += ret;
  target_len -= ret;
  return (target_len == 0);
} /* AudioProcessor::flushBuffer */


/*
 * This file has not been truncated
 */

