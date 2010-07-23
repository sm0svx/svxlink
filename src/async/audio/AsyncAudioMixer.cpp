/**
@file	 AsyncAudioMixer.cpp
@brief   This file contains an audio mixer class
@author  Tobias Blomberg / SM0SVX
@date	 2007-10-05

\verbatim
Async - A library for programming event driven applications
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
    MixerSrc(AudioMixer *mixer)
      : fifo(64), mixer(mixer), is_flushed(true), do_flush(false)
    {
      AudioSink::setHandler(&fifo);
      fifo.registerSink(&reader);
    }
    
    int writeSamples(const float *samples, int count)
    {
      //printf("Async::AudioMixer::MixerSrc::writeSamples: count=%d\n", count);
      is_flushed = false;
      do_flush = false;
      int ret = fifo.writeSamples(samples, count);
      mixer->setAudioAvailable();
      return ret;
    }
    
    void flushSamples(void)
    {
      //printf("Async::AudioMixer::MixerSrc::flushSamples\n");
      if (is_flushed && !do_flush && fifo.empty())
      {
        fifo.flushSamples();
      }
      
      is_flushed = true;
      do_flush = true;
      if (fifo.empty())
      {
        mixer->flushSamples();
      }
    }
    
    bool isActive(void) const { return !is_flushed || !fifo.empty(); }
    
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
    bool        is_flushed;
    bool        do_flush;
    
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
  : fifo(64), is_flushed(true), is_requesting(false)
{
  AudioSource::setHandler(&fifo);
  sink.registerSink(&fifo);
  sink.sigRequestSamples.connect(slot(*this, &AudioMixer::onRequestSamples));
  sink.sigAllSamplesFlushed.connect(slot(*this, &AudioMixer::onAllSamplesFlushed));
} /* AudioMixer::AudioMixer */


AudioMixer::~AudioMixer(void)
{
  list<MixerSrc *>::iterator it;
  for (it = sources.begin(); it != sources.end(); ++it)
  {
    delete *it;
  }
  AudioSource::clearHandler();
} /* AudioMixer::~AudioMixer */


void AudioMixer::addSource(AudioSource *source)
{
  MixerSrc *mixer_src = new MixerSrc(this);
  mixer_src->registerSource(source);
  sources.push_back(mixer_src);
} /* AudioMixer::addSource */


/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/

void AudioMixer::onRequestSamples(int count)
{
  is_requesting = true;

  float buf[count];
  memset(buf, 0, count * sizeof(float));

  int total_samples = 0;
  list<MixerSrc *>::const_iterator it;
  for (it = sources.begin(); it != sources.end(); ++it)
  {
    MixerSrc *mix_src = *it;
    if (mix_src->isActive())
    {
      float tmp[count];
      int samples_read = mix_src->readSamples(tmp, count);
      total_samples += samples_read;
      /* if (samples_read < count)
      {
        printf("underrun: %d %d\n", samples_read, count);
      } */
      for (int i=0; i<samples_read; ++i)
      {
        buf[i] += tmp[i];
      }
    }
  }

  is_requesting = false;
  
  if (is_flushed && (total_samples == 0))
  {
    fifo.flushSamples();
    return;
  }
  
  assert(sink.writeSamples(buf, count) == count);
  
} /* AudioMixer::requestSamples */


void AudioMixer::onAllSamplesFlushed(void)
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


void AudioMixer::setAudioAvailable(void)
{
  if (is_requesting) return;
  is_flushed = false;
  
  unsigned len = 0;
  
  list<MixerSrc *>::const_iterator it;
  for (it = sources.begin(); it != sources.end(); ++it)
  {
    MixerSrc *mix_src = *it;
    if (mix_src->isActive())
    {
      len = max(len, mix_src->samplesInFifo());
    }
  }

  onRequestSamples(min(len, fifo.spaceAvail()));

} /* AudioMixer::setAudioAvailable */


void AudioMixer::flushSamples(void)
{
  //printf("AudioMixer::flushSamples\n");
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
  if (fifo.empty())
  {
    fifo.flushSamples();
  }

} /* AudioMixer::flushSamples */


/*
 * This file has not been truncated
 */
