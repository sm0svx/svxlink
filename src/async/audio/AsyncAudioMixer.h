/**
@file	 AsyncAudioMixer.h
@brief   This file contains an audio mixer class
@author  Tobias Blomberg / SM0SVX
@date	 2007-10-05

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2004-2007  Tobias Blomberg / SM0SVX

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


#ifndef ASYNC_AUDIO_MIXER_INCLUDED
#define ASYNC_AUDIO_MIXER_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <list>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncAudioSource.h>
#include <AsyncAudioFifo.h>
#include <SigCAudioSource.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Forward declarations
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Namespace
 *
 ****************************************************************************/

namespace Async
{


/****************************************************************************
 *
 * Forward declarations of classes inside of the declared namespace
 *
 ****************************************************************************/

class Timer;
  

/****************************************************************************
 *
 * Defines & typedefs
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Exported Global Variables
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Class definitions
 *
 ****************************************************************************/

/**
@brief	A class for mixing audio streams
@author Tobias Blomberg / SM0SVX
@date   2007-10-05

This class is used to mix audio streams together.
*/
class AudioMixer : public SigC::Object, public Async::AudioSource
{
  public:
    /**
     * @brief 	Default constuctor
     */
    AudioMixer(void);
  
    /**
     * @brief 	Destructor
     */
    ~AudioMixer(void);
  
    /**
     * @brief 	Add an audio source to the mixer
     * @param 	source The audio source to add
     */
    void addSource(AudioSource *source);

    
  protected:

    /**
     * @brief Request audio samples from the source
     * @param count
     * 
     * This function will be called when the registered audio sink is ready
     * to accept more samples.
     */
    void onRequestSamples(int count);

    /**
     * @brief The registered sink has flushed all samples
     *
     * This function will be called when all samples have been flushed in the
     * registered sink.
     * This function is normally only called from a connected sink object.
     */
    void onAllSamplesFlushed(void);
    
  private:
    class MixerSrc;
    
    std::list<MixerSrc *> sources;
    AudioFifo             fifo;
    SigCAudioSource       sigsrc;
    
    AudioMixer(const AudioMixer&);
    AudioMixer& operator=(const AudioMixer&);
    
    int writeSamples(MixerSrc *src, const float *samples, int count);
    void availSamples(void);
    void checkFlushSamples(void);
    
    friend class MixerSrc;
    
};  /* class AudioMixer */


} /* namespace */

#endif /* ASYNC_AUDIO_MIXER_INCLUDED */



/*
 * This file has not been truncated
 */

