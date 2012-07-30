/**
@file	  AsyncAudioDelayLine.h
@brief	  An audio pipe component to create a delay used for muting
@author	  Tobias Blomberg / SM0SVX
@date	  2006-07-08

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2003-2010  Tobias Blomberg / SM0SVX

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


#ifndef ASYNC_AUDIO_DELAY_LINE_INCLUDED
#define ASYNC_AUDIO_DELAY_LINE_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncAudioSink.h>
#include <AsyncAudioSource.h>


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
@brief	This class implements an audio delay line
@author Tobias Blomberg / SM0SVX
@date   2006-07-08

This class implements an audio delay line. It simply delays the audio with
the specified amount of time. This can be useful if you want to mute audio
based on a slow detector. With a delay line you have the possibility to
mute audio that have passed the detector but have not yet passed through the
delay line.
*/
class AudioDelayLine : public Async::AudioSink, public Async::AudioSource
{
  public:
    /**
     * @brief Constuctor
     * @param length_ms The length in milliseconds of the delay line
     */
    explicit AudioDelayLine(int length_ms);
  
    /**
     * @brief 	Destructor
     */
    ~AudioDelayLine(void);

    /**
     * @brief   Set the fade in/out time when muting and clearing
     * @param	time_ms The time in milliseconds for the fade in/out
     *
     * When a mute or clear is issued the audio stream will not abruptly
     * go to zero. Instead it will fade in and out smoothly to avoid
     * popping sounds due to discontinueties in the sample stream.
     * The default is 10 milliseconds. Set to 0 to turn off.
     */
    void setFadeTime(int time_ms);
    
    /**
     * @brief 	Mute audio
     * @param 	do_mute If \em true mute else unmute
     * @param 	time_ms How much more time in milliseconds to mute (see below)
     *
     * This function is used to mute audio in the delay line. With time_ms
     * equal to zero its function is trivial. Mute incoming audio until
     * mute is called with do_mute = false. The time_ms paramter specify how
     * much time before (do_mute = true) or time after (do_mute = false)
     * should be muted. Negative values are not allowed.
     */
    void mute(bool do_mute, int time_ms=0);
    
    /**
     * @brief 	Clear samples in the delay line
     * @param 	time_ms How much time in milliseconds to clear
     *
     * Will clear the specified amount of samples in the delay line. If a
     * clear is issued right before the delay line is flushed, the cleared
     * samples will not be flushed. They will be thrown away.
     */
    void clear(int time_ms=-1);
  
    /**
     * @brief 	Write samples into the delay line
     * @param 	samples The buffer containing the samples
     * @param 	count The number of samples in the buffer
     * @return	Returns the number of samples that has been taken care of
     *
     * This function will write samples into the delay line. It's normally
     * only called from a connected source object.
     */
    int writeSamples(const float *samples, int count);
    
    /**
     * @brief 	Tell the sink to flush the previously written samples
     *
     * This function is used to tell the sink to flush previously written
     * samples. When done flushing, the sink should call the
     * sourceAllSamplesFlushed function.
     * This function is normally only called from a connected source object.
     */
    void flushSamples(void);

    /**
     * @brief Resume audio output to the sink
     * 
     * This function must be reimplemented by the inheriting class. It
     * will be called when the registered audio sink is ready to
     * accept more samples.
     * This function is normally only called from a connected sink object.
     */
    void resumeOutput(void);
    
    /**
     * @brief The registered sink has flushed all samples
     *
     * This function must be implemented by the inheriting class. It
     * will be called when all samples have been flushed in
     * the registered sink.
     * This function is normally only called from a connected sink object.
     */
    void allSamplesFlushed(void);
    
    
  protected:
    
  private:
    static const int DEFAULT_FADE_TIME = 10; // 10ms default fade time

    float	*buf;
    int		size;
    int		ptr;
    int		flush_cnt;
    bool	is_muted;
    int		mute_cnt;
    int		last_clear;
    float	*fade_gain;
    int		fade_len;
    int		fade_pos;
    int		fade_dir;
    
    AudioDelayLine(const AudioDelayLine&);
    AudioDelayLine& operator=(const AudioDelayLine&);
    void writeRemainingSamples(void);

    inline float currentFadeGain(void)
    {
      if (fade_gain == 0)
      {
        return 1.0f;
      }
      
      float gain = fade_gain[fade_pos];
      fade_pos += fade_dir;
      
      if ((fade_dir > 0) && (fade_pos >= fade_len-1))
      {
        fade_dir = 0;
        fade_pos = fade_len-1;
      }
      else if ((fade_dir < 0) && (fade_pos <= 0))
      {
        fade_dir = 0;
        fade_pos = 0;
      }
      
      return gain;
      
    } /* AudioDelayLine::currentFadeGain  */

};  /* class AudioDelayLine */


} /* namespace */

#endif /* ASYNC_AUDIO_DELAY_LINE_INCLUDED */



/*
 * This file has not been truncated
 */

