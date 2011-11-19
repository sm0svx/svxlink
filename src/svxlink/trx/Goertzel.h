/**
@file	 Goertzel.h
@brief   An implementation of the Gortzel single bin DFT algorithm
@author  Tobias Blomberg / SM0SVX
@date	 2009-05-23

\verbatim
<A brief description of the program or library this file belongs to>
Copyright (C) 2003-2008 Tobias Blomberg / SM0SVX

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

The Goertzel algorithm is used to calculate the DFT for a single
bin. This is much more efficient than for example using an FFT to
calculate all bins.

Create and initialize a new object either using the default constructor
and then calling the "initialize" method or use the constuctor that
initialize the object upon construction. The information needed for
initialization is the center frequency of the bin to calculate and the
sampling frequency used.

Now you need to choose a block length. There are two things to consider
to choose a good block length. The first thing is the wanted bin width.
The bin width, in Hz, get more narrow with increasing block length. The
bin width can be calculated using the following formula:

  sampling_rate / block_len

So, if you are using a sampling frequency of 8000 Hz and a block length of
100 samples, the bin width will become 80Hz.

Reset the object by calling the "reset" method. Now call the "calc" method
for each sample in the block. When block_len samples have been processed,
the result is ready to be read. If only the magnitude is interesting, use
the "relativeMagnitudeSquared" function since it's more efective than using
the "result" function. The "result" function give a complex value from
which both phase and magnitude can be calculated.

The relative magnitude squared value can be used directly to compare it
against other relative magnitude squared values. If you want to compare it
to other measured values, it will probably be necessary to recalculate it
to an absolute magnitude value. For example, an input signal containing a
sinus with amplitude 1.0 will produce magnitude 0.5 in the frequency plane.
The reason it's 0.5 is because the peak in the frequency plane is mirrored
around zero so it's actually two peaks with magnitude 0.5. So to get the
magnitude for the relative magnitude squared value, we do the following:

  2 * sqrt(rel_mag_sqr) / block_len

Now we will get magnitude one for the case described above. However, the
magnitude may not be what you need. The mean power in the signal during
the block may be more interesting. We calculate this by first converting
the magnitude (peak) value to a RMS value. This is easily done by dividing
by sqrt(2). We then square the whole thing to get the mean power. This
calculation can be simplified to:

  2 * rel_mag_sqr / (block_len * block_len)

If using Goertzel to find one or more tones, one way to determine if a
tone is present or not is to compare the power given by Goertzel to the
power in the whole passband. The passband power is easily calculated by
squaring each sample in the block and adding them together. Then divide
this with the block length. This will give the mean power in the passband
during the block, including the tone power of course. We will see later
that what we actually need is the passband_energy, which we get by simply
not dividing the summed squares with the block lenth. Now the result from
Goertzel and the passband power can be compared to give a measure if a
tone is present or not. Dividing the tone power with the passband power
will give a value between 0.0 and 1.0. If it's close to one, almost all
power is in the tone. If close to zero, we probably just have broad band
noise. After some simplification, the relation can be computed using:

  2 * rel_mag_sqr / (block_len * passband_energy)

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
    Goertzel(void) { }

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
      w = 2.0f * M_PI * (freq / (float)sample_rate);
      cosw = cosf(w);
      sinw = sinf(w);
      fac = 2.0f * cosw;
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
      q0 = fac * q1 - q2 + sample;
    }
    
    /**
     * @brief  Calculate the final result in complex form
     * @return Returns the final result in complex form
     *
     * This function will calculate and return the final result of
     * one block. The result is returned in complex form. The magnitude
     * and phase can be calculated from the complex value.
     * If only the magnitude is required, use the relativeMagnitudeSquared
     * function instead which is more efficient.
     */
    std::complex<float> result(void)
    {
      float real = q0 - q1 * cosw;
      float imag = q1 * sinw;
      return std::complex<float>(real, imag);
    }

    /**
     * @brief  Calculate the phase using a previously calculated complex result
     * @param  res The complex result as returned by the "result" function
     * @return Returns the phase of the signal
     */
    float phase(const std::complex<float> &res) { return std::arg(res); }

    /**
     * @brief  Calculate the phase
     * @return Returns the phase of the signal
     */
    float phase(void) { return std::arg(result()); }

    /**
     * @brief 	Calculate the relative magnitude squared from a complex result
     * @return	Returns the relative magnitude squared
     *
     * See the class documentation for information on how to use the
     * returned value.
     */
    float relativeMagnitudeSquared(const std::complex<float> &res)
    {
      return std::norm(res);
    }
    
    /**
     * @brief 	Read back the result after calling "calc" for a whole block
     * @return	Returns the relative magnitude squared
     *
     * See the class documentation for information on how to use the
     * returned value.
     */
    float relativeMagnitudeSquared(void)
    {
        // Push a zero through the process to finish things off.
        // FIXME: Should we really do this?  Why?
      //calc(0.0f);
      
        // Now calculate the non-recursive side of the filter.
        // The result here is not scaled down to allow for the magnification
        // effect of the filter (the usual DFT magnification effect).
      return q0 * q0 + q1 * q1 - q0 * q1 * fac;
    }

  protected:
    
  private:
    float w;
    float cosw;
    float sinw;
    float fac;
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

