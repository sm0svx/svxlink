/**
@file	 Goertzel.h
@brief   An implementation of the Gortzel single bin DFT algorithm
@author  Tobias Blomberg / SM0SVX
@date	 2009-05-23

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2022 Tobias Blomberg / SM0SVX

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


#ifndef GOERTZEL_INCLUDED
#define GOERTZEL_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <cmath>
#include <utility>
#include <complex>


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
@brief	An implementation of the Gortzel single bin DFT algorithm
@author Tobias Blomberg / SM0SVX
@date   2009-05-23

The Goertzel algorithm is used to calculate the DFT (Discrete Fourier
Transform) for a single bin. This is much more efficient than for example using
an FFT to calculate all bins. If M is the number of Goertzel detectors in use
and N is the number of bins in the equivalent FFT, the Goertzel algorithm is
more efficient if M < log2(N).  Other advantages of the Goertzel algorithm is
that every single bin can be placed exactly at the frequency of interest and
the block length can be chosen arbitrarily.

Create and initialize a new object either by using the default constructor and
then calling the "initialize" method or use the constuctor that initialize the
object upon construction. The information needed for initialization is the
center frequency of the bin to calculate and the sampling frequency used.

Now you need to choose a block length. There are two things to consider to
choose a good block length. The first thing is the wanted bin width.  The bin
width, in Hz, get more narrow with increasing block length. The bin width can
be calculated using the following formula:

  sampling_rate / block_len

So, if you are using a sampling frequency of 8000 Hz and a block length of 100
samples, the bin width will become 80Hz. The other thing to consider when
choosing the block length is the calculation delay. Higher block lengths give
more delay.  The delay, in seconds, will be:

  block_len / sampling_rate

Reset the object by calling the "reset" method. Now call the "calc" method for
each sample in the block. When block_len samples have been processed, the
result is ready to be read. If only the magnitude is interesting, use the
"magnitudeSquared" function since it's more efficient than using the
"result" function. The "result" function give a complex value from which both
phase and magnitude can be calculated.

The magnitude squared value (mag_sqr) can be used directly to compare it
against other magnitude squared values. If you want to calculate the power
difference in dB between two bins, just do this:

  diff_db = 10.0 * log10(mag_sqr / other_mag_sqr)

Why the expression above works will be explained below.

If you want to compare the magnitude squared value to values not calculated
using Goertzel, we will need to understand what the magnitude squared really
is. For example, if the input signal is a sinus with amplitude A, that
amplitude can be recovered from the magnitude squared value using the following
formula:

  A = 2.0 * sqrt(mag_sqr) / block_len

The amplitude may not be what you need though. The mean power in the signal
during the block may be more interesting. We calculate this by first converting
the (peak) amplitude value to an RMS value. This is easily done by dividing the
expression above (A) by sqrt(2). We then square the whole thing to get the mean
power. That calculation can be simplified to:

  pwr = 2.0 * mag_sqr / (block_len * block_len)

Now we can understand how the first calculation of diff_db works, where the
power difference between two bins in dB were calculated. If you divide two of
the power expressions above with each other, only the two magnitude squared
values will remain. So, dividing two magnitude squared values with each other
is the same thing as dividing two power values with each other.

If using Goertzel to find one or more tones, one way to determine if a tone is
present or not is to compare the power given by Goertzel (Ptone) to the power
in the whole passband (Ppass=Ptone+Pnoise). The passband power (Ppass) is
easily calculated by squaring each sample in the block and adding them
together. Then divide this with the block length.  This will give the mean
power in the passband during the block, including the tone power of course. We
will see later that what we actually need is the passband_energy, which we get
by simply not dividing the summed squares with the block lenth. Now the result
from Goertzel and the passband power can be compared to give a measure if a
tone is present or not.  Dividing the tone power with the passband power will
give a value between 0.0 and 1.0. If it's close to one, almost all power is in
the tone. If close to zero, we probably just have broad band noise or a signal
somewhere outside the Goertzel detector bandwidth.  After some simplification,
the relation (Ptone/Ppass) can be calculated using:

  rel = 2 * mag_sqr / (block_len * passband_energy)

Another interesting value could be SNR, the Signal to Noise ratio. To
calculate the SNR we need the power in the noise (Ptnoise) within the tone
detector bandwidth (Btone) without the tone power (Ptone). The formula below
will calculate an estimate of the noise within the tone detector bandwidth
(Btone) by using the passband power (Ppass) normalized using the passband
bandwidth (Bpass).

  Ptnoise = (Ppass - Ptone) / ((Bpass-Btone) / Btone);

Now the SNR can be calculated using the formula below. This will give the SNR
in dB.

  SNR = 10.0 * log10(Ptone/Ptnoise)

The above calculation will give a good result if the frequency spectrum is
reasonably flat. If the frequency spectrum is leaning, the SNR will have an
offset. This offset can be calibrated away by only inputting the noise without
a tone into the detector. The SNR formula above will now output the offset
value (SNRoffset). Use this value to calculate corrected SNR values (SNRreal).

  SNRreal = SNR - SNRoffset

Now you have an estimate for the tone SNR (Signal to Noise Ratio) within the
tone detector bandwidth, provided that the selected passband only contain the
tone and noise.

When finished with the block, call "reset" again and start over.
*/
class Goertzel
{
  public:
    /**
     * @brief 	Default constuctor
     *
     * This constructor will create an uninitialized object. Use the
     * initialize method to initialize it later.
     */
    Goertzel(void)
      : cosw(0.0f), sinw(0.0f), two_cosw(0.0f), q0(0.0f), q1(0.0f) {}

    /**
     * @brief 	Constuctor
     * @param   freq        The frequency of interest, in Hz
     * @param   sample_rate The sample rate used
     *
     * This constructor will create an initialized object. You do not need
     * to call the initialize method unless you want to change something
     * after construction.
     */
    Goertzel(float freq, unsigned sample_rate)
    {
      initialize(freq, sample_rate);
    }
  
    /**
     * @brief 	Destructor
     */
    ~Goertzel(void) {}
  
    /**
     * @brief  Initialize the object
     * @param  freq The frequency of interest, in Hz
     * @param  sample_rate The sample rate used
     *
     * This method will initialize the object. It may be called more than
     * once if something need to be changed.
     */
    void initialize(float freq, unsigned sample_rate)
    {
      float w = 2.0f * M_PI * (freq / (float)sample_rate);
      cosw = cosf(w);
      sinw = sinf(w);
      two_cosw = 2.0f * cosw;
      reset();
    }

    /**
     * @brief 	Reset the state variables
     */
    void reset(void)
    {
      q0 = q1 = 0.0f;
    }
    
    /**
     * @brief 	Call this function for each sample in a block
     * @param 	sample The sample to process
     */
    inline void calc(float sample)
    {
      float q2 = q1;
      q1 = q0;
      q0 = two_cosw * q1 - q2 + sample;
    }
    
    /**
     * @brief  Calculate the final result in complex form
     * @return Returns the final result in complex form
     *
     * This function will calculate and return the final result of
     * one block. The result is returned in complex form. The magnitude
     * and phase can be calculated from the complex value.
     * If only the magnitude is required, use the magnitudeSquared
     * function instead which is more efficient.
     */
    std::complex<float> result(void)
    {
      float real = cosw * q0 - q1;
      float imag = sinw * q0;
      return std::complex<float>(real, imag);
    }

    /**
     * @brief  Calculate the phase using a previously calculated complex result
     * @param  res The complex result as returned by the "result" function
     * @return Returns the phase of the DFT
     */
    static float phase(const std::complex<float> &res) { return std::arg(res); }

    /**
     * @brief  Calculate the phase
     * @return Returns the phase of the DFT
     */
    float phase(void) { return std::arg(result()); }

    /**
     * @brief 	Calculate the magnitude from a complex result
     * @return	Returns the magnitude of the DFT
     *
     * See the class documentation for information on how to use the
     * returned value. Note that if only the squared magnitude is needed
     * and not the phase, the magnitudeSquared function is more efficient.
     */
    float magnitude(const std::complex<float> &res)
    {
      return std::abs(res);
    }
    
    /**
     * @brief 	Calculate the magnitude squared from a complex result
     * @return	Returns the magnitude of the DFT
     *
     * See the class documentation for information on how to use the
     * returned value. Note that if only the squared magnitude is needed
     * and not the phase, the magnitudeSquared function without argument is
     * more efficient.
     */
    static float magnitudeSquared(const std::complex<float> &res)
    {
      return std::norm(res);
    }
    
    /**
     * @brief 	Read back the result after calling "calc" for a whole block
     * @return	Returns the magnitude squared
     *
     * This function will calculate only the squared magnitude so if only
     * that is of interest, this function is more effective than using the
     * "result" function.
     * See the class documentation for information on how the returned value
     * may be used.
     */
    float magnitudeSquared(void)
    {
      return q0 * q0 + q1 * q1 - q0 * q1 * two_cosw;
    }

  protected:
    
  private:
    float cosw;
    float sinw;
    float two_cosw;
    float q0;
    float q1;
    
    //Goertzel(const Goertzel&);
    //Goertzel& operator=(const Goertzel&);
    
};  /* class Goertzel */


//} /* namespace */

#endif /* GOERTZEL_INCLUDED */



/*
 * This file has not been truncated
 */

