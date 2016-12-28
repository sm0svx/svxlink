/**
@file	 AsyncAudioGenerator.h
@brief   An audio generator
@author  Tobias Blomberg / SM0SVX
@date	 2015-09-28

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

#ifndef ASYNC_AUDIO_GENERATOR_INCLUDED
#define ASYNC_AUDIO_GENERATOR_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <cmath>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

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
@brief	A class for generating periodic audio signals
@author Tobias Blomberg / SM0SVX
@date   2015-09-28

This class is used to generate periodic audio signals. Note that audio samples
will be produced in an endless loop until the connected sink stop the flow.
This means that a sink have to be connected before enabling the generator or
the application will get stuck. There also must be some form of flow control
downstream in the audio pipe. One way to get flow control, if there are none
naturally, is to use an Async::AudioPacer.
*/
class AudioGenerator : public Async::AudioSource
{
  public:
    /**
     * @brief The type of waveform to generate
     */
    typedef enum {
      SIN,    ///< Sine wave
      SQUARE  ///< Square wave
    } Waveform;

    /**
     * @brief   Contructor
     * @param   wf The waveform to use (@see Waveform)
     */
    explicit AudioGenerator(Waveform wf=SIN)
      : pos(0), fq(0.0), level(0.0), sample_rate(INTERNAL_SAMPLE_RATE),
        waveform(wf), power(0.0)
    {
    }
    
    /**
     * @brief   Destructor
     */
    ~AudioGenerator(void)
    {
      enable(false);
    }

    /**
     * @brief   Set which waveform to use
     * @param   wf The waveform to use (@see Waveform)
     */
    void setWaveform(Waveform wf)
    {
      waveform = wf;
      calcLevel();
    }
    
    /**
     * @brief   Set the audio frequency
     * @param   tone_fq The frequency in Hz
     */
    void setFq(double tone_fq)
    {
      fq = tone_fq;
    }
    
    /**
     * @brief   Set the power of the generated signal
     * @param   pwr_db The power of the signal in dB
     *
     * Use this function to set the power of the generated signal. 0dB power is
     * defined as a full-scale sine wave.
     */
    void setPower(float pwr_db)
    {
      power = pow(10.0, pwr_db / 10.0f) / 2;
      calcLevel();
    }
    
    /**
     * @brief   Enable or disable the generator
     * @param   enable Set to \em true to enable the generator or \em false
     *          to disable it
     */
    void enable(bool enable)
    {
      if (enable && (fq != 0))
      {
        pos = 0;
        writeSamples();
      }
    }

    /**
     * @brief Resume audio output to the sink
     * 
     * This function is normally only called from a connected sink object.
     */
    void resumeOutput(void)
    {
      writeSamples();
    }
    
    /**
     * @brief The registered sink has flushed all samples
     *
     * This function is normally only called from a connected sink object.
     */
    void allSamplesFlushed(void)
    {
    }
    
  private:
    static const int BLOCK_SIZE = 128;
    
    unsigned  pos;
    double    fq;
    double    level;
    int       sample_rate;
    Waveform  waveform;
    float     power;
    
    AudioGenerator(const AudioGenerator&);
    AudioGenerator& operator=(const AudioGenerator&);
    
    /**
     * @brief   Calculate the peak level corresponding to the set power for
     *          the set waveform
     */
    void calcLevel(void)
    {
      switch (waveform)
      {
        case SIN:
          level = sqrt(2 * power);
          break;
        case SQUARE:
          level = sqrt(power);
          break;
        default:
          level = 1.0;
          break;
      }
    }

    /**
     * @brief   Write samples to the connected sink
     */
    void writeSamples(void)
    {
      int written;
      do {
	float buf[BLOCK_SIZE];
	for (int i=0; i<BLOCK_SIZE; ++i)
	{
          switch (waveform)
          {
            case SIN:
      	      buf[i] = level * sin(2 * M_PI * fq * (pos+i) / sample_rate);
              break;
            case SQUARE:
      	      buf[i] = level * (sin(2 * M_PI * fq * (pos+i) / sample_rate)
                                > 0.0 ? 1 : -1);
              break;
            default:
              buf[i] = 0;
              break;
          }
	}
	written = sinkWriteSamples(buf, BLOCK_SIZE);
	pos += written;
      } while (written != 0);
    }
    
};  /* class AudioGenerator */


} /* namespace */

#endif /* ASYNC_AUDIO_GENERATOR_INCLUDED */



/*
 * This file has not been truncated
 */
