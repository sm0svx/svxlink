/**
@file	 AfskDemodulator.h
@brief   A class to demodulate Audio Frequency Shift Keying
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

#ifndef AFSK_DEMODULATOR_INCLUDED
#define AFSK_DEMODULATOR_INCLUDED


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
@brief	A class to demodulate Audio Frequency Shift Keying
@author Tobias Blomberg / SM0SVX
@date   2013-05-09

Use this class to demodulate an AFSK, Audio Frequency Shift Keying, audio
stream. The output is a sample stream of high and low DC values corresponding
to the high and low AFSK frequencies. The demodulated sample stream will have
to be bit synchronized and downsampled in the next stage.

The method used to demodulate the signal is to correlate a sample with a
previous sample of the same signal, an autocorrelation. This will produce the
output signal consisting of the two DC levels. The delay used in the correlator
is optimized to maximize the difference between the high and low frequencies.

  cos(x) * cos(x-phi) =
  cos(x) * (cos(x)*cos(phi) + sin(x)*sin(phi)) =
  cos(x)*cos(x)*cos(phi) + cos(x)*sin(x)*sin(phi) =
  0.5*cos(2x)*cos(phi) + 0.5*(1 - sin(2x))*sin(phi) =
  [low pass filter to remote the 2x frequency component] =
  0.5 * sin(phi)

*/
class AfskDemodulator : public Async::AudioSink, public Async::AudioSource
{
  public:
    /**
     * @brief 	Constuctor
     * @param   f0          Lower audio frequency
     * @param   f1          Upper audio frequency
     * @param   baudrate    The baudrate of the datastream
     * @param   sample_rate The sample rate of the audio stream
     */
    AfskDemodulator(unsigned f0, unsigned f1, unsigned baudrate,
                   unsigned sample_rate=INTERNAL_SAMPLE_RATE);

    /**
     * @brief 	Destructor
     */
    ~AfskDemodulator(void);

  private:
    const unsigned f0;
    const unsigned f1;
    const unsigned baudrate;

    AfskDemodulator(const AfskDemodulator&);
    AfskDemodulator& operator=(const AfskDemodulator&);

};  /* class AfskDemodulator */


//} /* namespace */

#endif /* AFSK_DEMODULATOR_INCLUDED */

/*
 * This file has not been truncated
 */
