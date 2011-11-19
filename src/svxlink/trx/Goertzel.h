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
      v2 = v3 = 0.0f;
    }
    
    /**
     * @brief 	Call this function for each sample in a block
     * @param 	sample The sample to process
     */
    inline void calc(float sample)
    {
      float v1 = v2;
      v2 = v3;
      v3 = fac * v2 - v1 + sample;
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
      float real = v3 - v2 * cosw;
      float imag = v2 * sinw;
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
     * This function returns the relative magnitude squared. To convert
     * it to a power measure, use the formula:
     *
     *   2*rel_mag_sqr / (block_len*block_len)
     *
     * Converting to power may be useful if you are going to compare the
     * result to some other measure, like the total passband power,
     * for example calculated in the time domain.
     */
    float relativeMagnitudeSquared(const std::complex<float> &res)
    {
      return std::norm(res);
    }
    
    /**
     * @brief 	Read back the result after calling "calc" for a whole block
     * @return	Returns the relative magnitude squared
     *
     * This function returns the relative magnitude squared. To convert
     * it to a power measure, use the formula:
     *
     *   2*rel_mag_sqr / (block_len*block_len)
     *
     * Converting to power may be useful if you are going to compare the
     * result to some other measure, like the total passband power,
     * for example calculated in the time domain.
     */
    float relativeMagnitudeSquared(void)
    {
        // Push a zero through the process to finish things off.
        // FIXME: Shoule we really do this?
      //float v1 = v2;
      //v2 = v3;
      //v3 = fac * v2 - v1;
      
        // Now calculate the non-recursive side of the filter.
        // The result here is not scaled down to allow for the magnification
        // effect of the filter (the usual DFT magnification effect).
      return v3 * v3 + v2 * v2 - v2 * v3 * fac;
    }

  protected:
    
  private:
    float w;
    float cosw;
    float sinw;
    float fac;
    float v2;
    float v3;
    
    //Goertzel(const Goertzel&);
    //Goertzel& operator=(const Goertzel&);
    
};  /* class Goertzel */


//} /* namespace */

#endif /* GOERTZEL_INCLUDED */



/*
 * This file has not been truncated
 */

