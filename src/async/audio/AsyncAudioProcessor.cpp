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
  : fifo(64), input_rate(1), output_rate(1)
{
  AudioSource::setHandler(&fifo);
  sigsrc.registerSink(&fifo);
  sigsrc.sigRequestSamples.connect(slot(*this, &AudioProcessor::onRequestSamples));
  sigsrc.sigAllSamplesFlushed.connect(slot(*this, &AudioProcessor::onAllSamplesFlushed));
} /* AudioProcessor::AudioProcessor */


AudioProcessor::~AudioProcessor(void)
{
  AudioSource::clearHandler();
} /* AudioProcessor::~AudioProcessor */


int AudioProcessor::writeSamples(const float *samples, int count)
{
  //cout << "AudioProcessor::writeSamples: len=" << count << endl;
  int written = 0;
  int space = fifo.spaceAvail();
  int len = min(space * input_rate / output_rate, count);
  
  while (len > 0)
  {
    float buf[space];
    int dest_len = processSamples(buf, samples, len);
    assert(sigsrc.writeSamples(buf, dest_len) == dest_len);

    count -= len;
    samples += len;
    written += len;

    space = fifo.spaceAvail();
    len = min(space * input_rate / output_rate, count);
  }

  return written;
} /* AudioProcessor::writeSamples */


void AudioProcessor::availSamples(void)
{
  //cout << "AudioProcessor::availSamples" << endl;
  sigsrc.availSamples();
} /* AudioProcessor::availSamples */


void AudioProcessor::flushSamples(void)
{
  //cout << "AudioProcessor::flushSamples" << endl;
  sigsrc.flushSamples();
} /* AudioProcessor::flushSamples */


void AudioProcessor::onRequestSamples(int count)
{
  //cout << "AudioProcessor::requestSamples: count=" << count << endl;

  // Note: We request input samples at input sample rate here.
  div_t ratio = div(count * input_rate, output_rate);
  sourceRequestSamples(ratio.rem ? ratio.quot + 1 : ratio.quot);
} /* AudioProcessor::requestSamples */


void AudioProcessor::onAllSamplesFlushed(void)
{
  //cout << "AudioProcessor::allSamplesFlushed" << endl;
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


/*
 * This file has not been truncated
 */
