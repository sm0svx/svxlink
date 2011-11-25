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
    MixerSrc(AudioMixer *mixer) : mixer(mixer), is_active(false) {}
    
    void availSamples(void)
    {
      //printf("Async::AudioMixer::MixerSrc::availSamples\n");
      is_active = true;
      AudioReader::availSamples();
      mixer->availSamples();
    }
    
    void flushSamples(void)
    {
      //printf("Async::AudioMixer::MixerSrc::flushSamples\n");
      is_active = false;
      AudioReader::flushSamples();
      if (!isReading())
      {
        mixer->checkFlushSamples();
      }
    }

    bool isActive(void) const { return is_active; }
    
  private:
    AudioMixer *mixer;
    bool       is_active;
    
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
{
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


void AudioMixer::requestSamples(int count)
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
  assert(sinkWriteSamples(buf, count) == count);

  // Second pass: Check if all sources are inactive now
  // so we can flush the sink
  checkFlushSamples();
  
} /* AudioMixer::requestSamples */


void AudioMixer::discardSamples(void)
{
  list<MixerSrc *>::const_iterator it;
  for (it = sources.begin(); it != sources.end(); ++it)
  {
    MixerSrc *mix_src = *it;
    if (mix_src->isActive())
    {
      mix_src->discardSamples();
    }
  }

} /* AudioMixer::discardSamples */


void AudioMixer::availSamples(void)
{
  sinkAvailSamples();
} /* AudioMixer::availSamples */


/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/

void AudioMixer::allSamplesFlushed(void)
{
} /* AudioMixer::allSamplesFlushed */


/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/

void AudioMixer::checkFlushSamples(void)
{
  list<MixerSrc *>::const_iterator it;
  for (it = sources.begin(); it != sources.end(); ++it)
  {
    if ((*it)->isActive()) return;
  }

  //printf("AudioMixer::checkFlushSamples: Flushing!\n");
  sinkFlushSamples();

} /* AudioMixer::checkFlushSamples */


/*
 * This file has not been truncated
 */
