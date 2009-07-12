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
     * @param   freq        The frequency of interest, in Hz
     * @param   sample_rate The sample rate used
     */
    Goertzel(float freq, unsigned sample_rate)
    {
      fac = 2.0f * cosf(2.0f * M_PI * (freq / (float)sample_rate));
      reset();
    }
  
    /**
     * @brief 	Destructor
     */
    ~Goertzel(void) {}
  
    /**
     * @brief 	Reset the state variables
     */
    void reset(void)
    {
      v2 = v3 = 0.0f;
    }
    
    /**
     * @brief 	Call this function for each sample in a block
     * @param 	sample A sample
     */
    void calc(float sample)
    {
      float v1 = v2;
      v2 = v3;
      v3 = fac * v2 - v1 + sample;
    }
    
    /**
     * @brief 	Read back the result after calling "calc" for a whole block
     * @return	Returns the relative magnitude squared
     */
    float result(void)
    {
        // Push a zero through the process to finish things off.
      float v1 = v2;
      v2 = v3;
      v3 = fac * v2 - v1;
      
        // Now calculate the non-recursive side of the filter.
        // The result here is not scaled down to allow for the magnification
        // effect of the filter (the usual DFT magnification effect).
      return v3 * v3 + v2 * v2 - v2 * v3 * fac;
    }
    
    
  protected:
    
  private:
    float v2;
    float v3;
    float fac;
    
    Goertzel(const Goertzel&);
    Goertzel& operator=(const Goertzel&);
    
};  /* class Goertzel */


//} /* namespace */

#endif /* GOERTZEL_INCLUDED */



/*
 * This file has not been truncated
 */

