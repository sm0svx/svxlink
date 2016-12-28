/**
@file	 AsyncAudioMixer.cpp
@brief   This file contains an audio mixer class
@author  Tobias Blomberg / SM0SVX
@date	 2007-10-05

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2003-2015 Tobias Blomberg / SM0SVX

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

#include <algorithm>
#include <cstring>


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

#include "AsyncAudioMixer.h"
#include "AsyncAudioFifo.h"
#include "AsyncAudioReader.h"



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

class Async::AudioMixer::MixerSrc : public AudioSink
{
  public:
    static const int FIFO_SIZE = AudioMixer::OUTBUF_SIZE;
    
    MixerSrc(AudioMixer *mixer)
      : fifo(FIFO_SIZE), mixer(mixer), is_flushed(true), do_flush(false)
    {
      AudioSink::setHandler(&fifo);
      fifo.registerSink(&reader);
    }
    
    int writeSamples(const float *samples, int count)
    {
      //printf("Async::AudioMixer::MixerSrc::writeSamples: count=%d\n", count);
      is_flushed = false;
      do_flush = false;
      mixer->setAudioAvailable();
      return fifo.writeSamples(samples, count);
    }
    
    void flushSamples(void)
    {
      if (is_flushed && !do_flush && fifo.empty())
      {
      	fifo.flushSamples();
      }
      
      //printf("Async::AudioMixer::MixerSrc::flushSamples\n");
      is_flushed = true;
      do_flush = true;
      if (fifo.empty())
      {
      	mixer->flushSamples();
      }
    }
    
    bool isActive(void) const
    {
      return !is_flushed || !fifo.empty();
    }
    
    void mixerFlushedAllSamples(void)
    {
      //printf("Async::AudioMixer::MixerSrc::mixerFlushedAllSamples\n");
      if (do_flush)
      {
      	do_flush = false;
	//printf("\tFlushing FIFO\n");
	fifo.flushSamples();
      }
    }
    
    bool isFlushing(void) const { return do_flush; }
    
    int readSamples(float *samples, int count)
    {
      return reader.readSamples(samples, count);
    }
    
    unsigned samplesInFifo(void) const { return fifo.samplesInFifo(); }
    
  private:
    AudioFifo 	fifo;
    AudioReader reader;
    AudioMixer  *mixer;
    bool      	is_flushed;
    bool      	do_flush;
    
}; /* class Async::AudioMixer::MixerSrc */




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

AudioMixer::AudioMixer(void)
  : output_timer(0, Timer::TYPE_ONESHOT, false), outbuf_pos(0),
    outbuf_cnt(0), is_flushed(true), output_stopped(false)
{
  output_timer.expired.connect(mem_fun(*this, &AudioMixer::outputHandler));
} /* AudioMixer::AudioMixer */


AudioMixer::~AudioMixer(void)
{
  list<MixerSrc *>::iterator it;
  for (it = sources.begin(); it != sources.end(); ++it)
  {
    delete *it;
  }
} /* AudioMixer::~AudioMixer */


void AudioMixer::addSource(AudioSource *source)
{
  MixerSrc *mixer_src = new MixerSrc(this);
  //mixer_src->stopOutput(true);
  //mixer_src->setOverwrite(false);
  mixer_src->registerSource(source);
  sources.push_back(mixer_src);
} /* AudioMixer::addSource */


void AudioMixer::resumeOutput(void)
{
  //printf("AudioMixer::resumeOutput\n");
  output_stopped = false;
  outputHandler(0);
} /* AudioMixer::resumeOutput */



/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/

void AudioMixer::allSamplesFlushed(void)
{
  //printf("AudioMixer::allSamplesFlushed\n");
  list<MixerSrc *>::iterator it;
  for (it = sources.begin(); it != sources.end(); ++it)
  {
    (*it)->mixerFlushedAllSamples();
  }
} /* AudioMixer::allSamplesFlushed */



/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/


/*
 *----------------------------------------------------------------------------
 * Method:    AudioMixer::setAudioAvailable
 * Purpose:   Called by one of the incoming stream handlers when there is
 *            data available. This will trigger a delayed execution of the
 *            outputHandler method. The execution of the real output handler
 *            is delayed so that all input streams have a chance to fill up.
 * Input:     None
 * Output:    None
 * Created:   2007-10-07
 * Remarks:   
 * Bugs:      
 *----------------------------------------------------------------------------
 */
void AudioMixer::setAudioAvailable(void)
{
  output_timer.setEnable(true);
} /* AudioMixer::setAudioAvailable */


/*
 *----------------------------------------------------------------------------
 * Method:    AudioMixer::flushSamples
 * Purpose:   Used by the input stream handlers to tell the mixer that
 *            they want to flush.
 * Input:     None
 * Output:    None
 * Created:   2007-10-07
 * Remarks:   
 * Bugs:      
 *----------------------------------------------------------------------------
 */
void AudioMixer::flushSamples(void)
{
  output_timer.setEnable(true);
} /* AudioMixer::flushSamples */


/*
 *----------------------------------------------------------------------------
 * Method:    AudioMixer::outputHandler
 * Purpose:   Handle the output of audio samples. All input streams are read
 *            and mixed together to a single output stream.
 * Input:     t - A pointer to the timer that triggered this function.
 * Output:    None
 * Created:   2007-10-07
 * Remarks:   
 * Bugs:      
 *----------------------------------------------------------------------------
 */
void AudioMixer::outputHandler(Timer *t)
{
  output_timer.setEnable(false);
  
  if (output_stopped)
  {
    return;
  }
  
  unsigned samples_written;
  do
  {
      // First empty the outbut buffer
    samples_written = 1; // Initialize to 1 to enter the loop
    while ((outbuf_pos < outbuf_cnt) && (samples_written > 0))
    {
      //printf("Writing %d samples\n", outbuf_cnt-outbuf_pos);
      is_flushed = false;
      samples_written = sinkWriteSamples(outbuf+outbuf_pos,
                                	 outbuf_cnt-outbuf_pos);
      outbuf_pos += samples_written;
    }
    
      // If the output buffer is empty, fill it up
    if (outbuf_pos >= outbuf_cnt)
    {
      	// Calculate the maximum number of samples we can read from the FIFOs
      unsigned samples_to_read = MixerSrc::FIFO_SIZE+1;
      list<MixerSrc *>::iterator it;
      for (it = sources.begin(); it != sources.end(); ++it)
      {
	if ((*it)->isActive())
	{
	  samples_to_read = min(samples_to_read, (*it)->samplesInFifo());
	}
      }
      
      	// There are no active input streams
      if (samples_to_read == MixerSrc::FIFO_SIZE+1)
      {
      	samples_to_read = 0;
      }
      
      //printf("samples_to_read=%d\n", samples_to_read);
      
      	// The output buffer is empty and we have nothing to fill it with
      if (samples_to_read == 0)
      {
      	checkFlush();
	break;
      }

      	// Fill the output buffer with samples from all active FIFOs
      memset(outbuf, 0, sizeof(outbuf));
      float tmp[OUTBUF_SIZE];
      for (it = sources.begin(); it != sources.end(); ++it)
      {
	if ((*it)->isActive())
	{
	  unsigned samples_read = (*it)->readSamples(tmp, samples_to_read);
	  assert(samples_read == samples_to_read);

	  for (unsigned i=0; i<samples_to_read; ++i)
	  {
      	    outbuf[i] += tmp[i];
	  }
	}
      }

      outbuf_pos = 0;
      outbuf_cnt = samples_to_read;
    }
  } while (samples_written > 0);
  
  output_stopped = (samples_written == 0);
  
} /* AudioMixer::outputHandler */


void AudioMixer::checkFlush(void)
{
  if (is_flushed)
  {
    return;
  }
  
  list<MixerSrc *>::iterator it;
  for (it = sources.begin(); it != sources.end(); ++it)
  {
    if ((*it)->isActive() && !(*it)->isFlushing())
    {
      return;
    }
  }
  
  //printf("AudioMixer::checkFlush: Flushing!\n");
  is_flushed = true;
  sinkFlushSamples();
  
} /* AudioMixer::checkFlush */





/*
 * This file has not been truncated
 */

