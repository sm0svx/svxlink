/**
@file    AsyncAudioGenerator.h
@brief   An audio generator
@author  Tobias Blomberg / SM0SVX
@date    2015-09-28

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2003-2019 Tobias Blomberg / SM0SVX

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
#include <cassert>
#include <iostream>


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
@brief  A class for generating periodic audio signals
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
      SIN,      ///< Sine wave
      SQUARE,   ///< Square wave
      TRIANGLE  ///< Triangular wave
    } Waveform;

    /**
     * @brief   Contructor
     * @param   wf The waveform to use (@see Waveform)
     */
    explicit AudioGenerator(Waveform wf=SIN)
      : m_arg(0.0f), m_arginc(0.0f), m_peak(0.0f),
        m_sample_rate(INTERNAL_SAMPLE_RATE), m_waveform(wf), m_power(0.0f),
        m_enabled(false)
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
      m_waveform = wf;
      calcLevel();
    }

    /**
     * @brief   Set the audio frequency
     * @param   tone_fq The frequency in Hz
     */
    void setFq(float tone_fq)
    {
      m_arginc = 2.0f * M_PI * tone_fq / m_sample_rate;
      assert(m_arginc <= M_PI);
    }

    /**
     * @brief   Set the power of the generated signal
     * @param   pwr_db The power of the signal in dBFS
     *
     * Use this function to set the power of the generated signal. 0dB power is
     * defined as a full-scale sine wave.
     */
    void setPower(float pwr_db)
    {
      m_power = powf(10.0f, pwr_db / 10.0f) / 2.0f;
      calcLevel();
    }

    /**
     * @brief   Enable or disable the generator
     * @param   enable Set to \em true to enable the generator or \em false
     *          to disable it
     */
    void enable(bool enable)
    {
      m_enabled = enable;
      if (enable)
      {
        m_arg = 0.0f;
        writeSamples();
      }
      else
      {
        sinkFlushSamples();
      }
    }

    /**
     * @brief Resume audio output to the sink
     *
     * This function is normally only called from a connected sink object.
     */
    void resumeOutput(void)
    {
      if (m_enabled)
      {
        writeSamples();
      }
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

    float     m_arg;
    float     m_arginc;
    float     m_peak;
    int       m_sample_rate;
    Waveform  m_waveform;
    float     m_power;
    bool      m_enabled;

    AudioGenerator(const AudioGenerator&);
    AudioGenerator& operator=(const AudioGenerator&);

    /**
     * @brief   Calculate the peak level corresponding to the set power for
     *          the set waveform
     */
    void calcLevel(void)
    {
      switch (m_waveform)
      {
        case SIN:
          m_peak = sqrt(2.0f * m_power);
          break;
        case SQUARE:
          m_peak = sqrt(m_power);
          break;
        case TRIANGLE:
          m_peak = sqrt(3.0f * m_power);
          break;
        default:
          m_peak = 0.0f;
          break;
      }
    }

    /**
     * @brief   Write samples to the connected sink
     */
    void writeSamples(void)
    {
      int written = 0;
      do
      {
        float buf[BLOCK_SIZE];
        float arg = m_arg;
        for (int i=0; i<BLOCK_SIZE; ++i)
        {
          switch (m_waveform)
          {
            case SIN:
              buf[i] = m_peak * sinf(arg);
              break;
            case SQUARE:
              buf[i] = (arg < M_PI) ? m_peak : -m_peak;
              break;
            case TRIANGLE:
              if (arg < M_PI / 2.0f)
              {
                buf[i] = m_peak * arg * 2.0f / M_PI;
              }
              else if (arg < M_PI)
              {
                buf[i] = m_peak * (2.0f - 2.0 * arg / M_PI);
              }
              else if (arg < 3.0f * M_PI / 2.0f)
              {
                buf[i] = -m_peak * (2.0f * arg / M_PI - 2.0f);
              }
              else
              {
                buf[i] = -m_peak * (4.0f - 2.0f * arg / M_PI);
              }
              break;
            default:
              buf[i] = 0;
              break;
          }
          arg += m_arginc;
          if (arg >= 2.0f * M_PI)
          {
            arg -= 2.0f * M_PI;
          }
        }
        written = sinkWriteSamples(buf, BLOCK_SIZE);
        if (written > 0)
        {
          m_arg += written * m_arginc;
          while (m_arg >= 2.0f * M_PI)
          {
            m_arg -= 2.0f * M_PI;
          }
        }
      } while (m_enabled && (written > 0));
    }
};  /* class AudioGenerator */


} /* namespace */

#endif /* ASYNC_AUDIO_GENERATOR_INCLUDED */

/*
 * This file has not been truncated
 */
