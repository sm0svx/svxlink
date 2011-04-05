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

class Async::AudioMixer::MixerSrc : public AudioReader
{
  public:
    MixerSrc(AudioMixer *mixer)
      : stream_state(STREAM_IDLE), mixer(mixer)
    {
    }
    
    int writeSamples(const float *samples, int count)
    {
      //printf("Async::AudioMixer::MixerSrc::writeSamples: count=%d\n", count);
      stream_state = STREAM_ACTIVE;
      if (isReading())
      {
        return AudioReader::writeSamples(samples, count);
      }
      return mixer->writeSamples(this, samples, count);
    }

    void availSamples(void)
    {
      //printf("Async::AudioMixer::MixerSrc::availSamples\n");
      stream_state = STREAM_ACTIVE;
      if (!isReading())
      {
        mixer->availSamples();
      }
    }
    
    void flushSamples(void)
    {
      //printf("Async::AudioMixer::MixerSrc::flushSamples\n");
      stream_state = STREAM_FLUSHING;
      if (!isReading())
      {
        mixer->checkFlushSamples();
      }
    }

    void allSamplesFlushed(void)
    {
      stream_state = STREAM_IDLE;
      sourceAllSamplesFlushed();
    }
    
    bool isActive(void) const { return (stream_state == STREAM_ACTIVE); }
    bool isFlushing(void) const { return (stream_state == STREAM_FLUSHING); }
    
  private:
    typedef enum
    {
      STREAM_IDLE, STREAM_ACTIVE, STREAM_FLUSHING
    } StreamState;
    StreamState stream_state;
    AudioMixer  *mixer;
    
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
  : fifo(64), is_flushing(false)
{
  AudioSource::setHandler(&fifo);
  sigsrc.registerSink(&fifo);
  sigsrc.sigRequestSamples.connect(slot(*this, &AudioMixer::onRequestSamples));
  sigsrc.sigAllSamplesFlushed.connect(slot(*this, &AudioMixer::onAllSamplesFlushed));
} /* AudioMixer::AudioMixer */


AudioMixer::~AudioMixer(void)
{
  list<MixerSrc *>::const_iterator it;
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
  float buf[count];
  memset(buf, 0, count * sizeof(float));

  // First pass: Read samples from the active sources and mix them
  list<MixerSrc *>::const_iterator it;
  for (it = sources.begin(); it != sources.end(); ++it)
  {
    MixerSrc *mix_src = *it;
    if (mix_src->isActive())
    {
      float tmp[count];
      int samples_read = mix_src->readSamples(tmp, count);
      for (int i=0; i<samples_read; ++i)
      {
        buf[i] += tmp[i];
      }
    }
  }

  // Write samples into the sink
  assert(sigsrc.writeSamples(buf, count) == count);

  // Second pass: Check for flushing sources
  checkFlushSamples();
  
} /* AudioMixer::requestSamples */


void AudioMixer::onAllSamplesFlushed(void)
{
  //printf("AudioMixer::allSamplesFlushed\n");
  list<MixerSrc *>::const_iterator it;
  for (it = sources.begin(); it != sources.end(); ++it)
  {
    MixerSrc *mix_src = *it;
    if (mix_src->isFlushing())
    {
      mix_src->allSamplesFlushed();
    }
  }
  is_flushing = false;
} /* AudioMixer::allSamplesFlushed */



/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/


int AudioMixer::writeSamples(MixerSrc *src, const float *samples, int count)
{
  int ret = 0;
  is_flushing = false;

  // First pass: Mix the incoming sample block with the other active sources 
  int len = min((int)fifo.spaceAvail(), count);
  while (len > 0)
  {
    float buf[len];
    memcpy(buf, samples, len * sizeof(float));

    list<MixerSrc *>::const_iterator it;
    for (it = sources.begin(); it != sources.end(); ++it)
    {
      MixerSrc *mix_src = *it;
      if ((mix_src != src) && mix_src->isActive())
      {
        float tmp[len];
        int samples_read = mix_src->readSamples(tmp, len);
        for (int i=0; i<samples_read; ++i)
        {
          buf[i] += tmp[i];
        }
      }
    }

    assert(sigsrc.writeSamples(buf, len) == len);

    count -= len;
    samples += len;
    ret += len;

    len = min((int)fifo.spaceAvail(), count);
  }

  // Second pass: Check for flushing sources
  checkFlushSamples();

  return ret;
} /* AudioMixer::writeSamples */


void AudioMixer::availSamples(void)
{
  sigsrc.availSamples();
} /* AudioMixer::availSamples */


void AudioMixer::checkFlushSamples(void)
{
  bool do_flush = false;
  
  list<MixerSrc *>::const_iterator it;
  for (it = sources.begin(); it != sources.end(); ++it)
  {
    MixerSrc *mix_src = *it;
    if (mix_src->isActive()) return;
    do_flush |= mix_src->isFlushing();
  }

  if (!is_flushing && do_flush)
  {  
    //printf("AudioMixer::checkFlushSamples: Flushing!\n");
    is_flushing = true;
    sigsrc.flushSamples();
  }

} /* AudioMixer::checkFlushSamples */


/*
 * This file has not been truncated
 */
