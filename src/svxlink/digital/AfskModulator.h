/**
@file	 AfskModulator.h
@brief   Audio Frequency Shift Keying modulator
@author  Tobias Blomberg / SM0SVX
@date	 2013-05-09

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2018 Tobias Blomberg / SM0SVX

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

#ifndef AFSK_MODULATOR_INCLUDED
#define AFSK_MODULATOR_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <deque>
#include <vector>
#include <sigc++/sigc++.h>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncAudioSource.h>
#include <CppStdCompat.h>


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

namespace Async
{
  class SigCAudioSource;
};


/****************************************************************************
 *
 * Namespace
 *
 ****************************************************************************/

//namespace MyNameSpace
//{


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
@brief	Audio Frequency Shift Keying modulator
@author Tobias Blomberg / SM0SVX
@date   2013-05-09

This class implement an Audio Frequency Shift Keying modulator. It will
generate an audio sample stream from the given bitstream.
*/
class AfskModulator : public Async::AudioSource, public sigc::trackable
{
  public:
    /**
     * @brief 	Constuctor
     * @param   f0          The lower frequency
     * @param   f1          The upper frequency
     * @param   baudrate    The baudrate of the bitstream
     * @param   sample_rate The sample rate of the audio stream
     */
    AfskModulator(unsigned f0, unsigned f1, unsigned baudrate, float level,
                  unsigned sample_rate=INTERNAL_SAMPLE_RATE);

    /**
     * @brief 	Destructor
     */
    ~AfskModulator(void);

    /**
     * @brief 	Generate audio samples from the given bits
     * @param 	bit The bit vector to send
     */
    void sendBits(const std::vector<bool> &bits);

  private:
    static CONSTEXPR unsigned BUFSIZE = 256;
    static CONSTEXPR unsigned FADE_SYMBOLS = 1;
    static CONSTEXPR float    FADE_START_VAL = 0.01f;

    const unsigned          baudrate;
    const unsigned          sample_rate;
    unsigned                N;
    float                   *sin_lookup;
    unsigned                k0;
    unsigned                k1;
    unsigned                phi;
    std::deque<bool>        bitbuf;
    unsigned                bitclock;
    float                   buf[BUFSIZE];
    unsigned                buf_pos;
    Async::SigCAudioSource  *sigc_src;
    unsigned                fade_len;
    unsigned                fade_pos;
    int                     fade_dir;
    float                   *exp_lookup;

    AfskModulator(const AfskModulator&);
    AfskModulator& operator=(const AfskModulator&);
    void writeToSink(void);
    void onResumeOutput(void);
    void onAllSamplesFlushed(void);

};  /* class AfskModulator */


//} /* namespace */

#endif /* AFSK_MODULATOR_INCLUDED */

/*
 * This file has not been truncated
 */
