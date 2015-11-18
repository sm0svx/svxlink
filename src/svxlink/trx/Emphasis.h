/**
@file	 Emphasis.h
@brief   An implementation of pre- and de-emphasis filters
@author  Tobias Blomberg / SM0SVX
@date	 2013-09-29

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2013 Tobias Blomberg / SM0SVX

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

#ifndef EMPHASIS_INCLUDED
#define EMPHASIS_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <sstream>
#include <cstdlib>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncAudioFilter.h>
//#include <AsyncAudioProcessor.h>
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
@brief	Base class for the pre- and de-emphasis filters
@author Tobias Blomberg / SM0SVX
@date   2013-09-29

This filter is based on a simple RC filter commonly used for de-emphasis in
real receivers. The transfer function for such a filter is 1/(1+sRC).  This
transfer function can be converted to time discrete form using MATLAB.

  fs=16000;         % Sampling rate
  f1=300;           % 3dB point of filter
  z1=0.9;           % Where we should put the filter zero
  G=0;              % The passband gain in dB
  [b, a]=bilinear([1], [1 2*pi*f1], fs); % Map S-plane to Z-plane
  b=2*pi*f1*b;      % Gain compensation
  k=(a(1)+a(2))/(b(1)+z1*b(2)); % Gain compensation for zero moving
  b(2)=z1*b(2);     % Move the zero
  b=k*b;            % Use the compensation factor calculated above
  b=10.^(G/20)*k*b; % Apply passband gain
  fvtool(b, a)      % Plot the result

The "b" (nominator) terms are adjusted for 0dB gain in the passband by
multiplying them with 2*pi*f1. Furthermore, we move the filter zero that is on
the unit circle so that we can invert the filter in the pre-emphasis stage
without getting an unstable filter. Inverting the de-emphasis filter will give
a pre-emphasis filter that exactly cancels out what the de-emphasis did.

fvtool will give the filter coefficients used below.
*/
class EmphasisBase : public Async::AudioFilter
{
  public:
    /**
     * @brief 	Default constructor
     */
    EmphasisBase(void) : output_gain(1.0) {}
  
    /**
     * @brief 	Destructor
     */
    ~EmphasisBase(void) {}
  
    /*
    void setOutputGain(double gain_db)
    {
      b0 /= output_gain;
      b1 /= output_gain;
      output_gain = pow(10.0, gain_db / 20.0);
      b0 *= output_gain;
      b1 *= output_gain;
    }
    */

    /**
     * @brief 	A_brief_member_function_description
     * @param 	param1 Description_of_param1
     * @return	Return_value_of_this_member_function
     */
    /*
    virtual void processSamples(float *dest, const float *src, int count)
    {
      for (int i=0; i<count; ++i)
      {
        double y0 = (b0 * src[i] + b1 * x1 - a1 * y1);// / a0;
        x1 = src[i];
        y1 = y0;
        dest[i] = y0;
      }
    }
    */
    
  protected:
#if INTERNAL_SAMPLE_RATE == 16000
      // 0dB gain, f1=300Hz, fs=16kHz
    static CONSTEXPR double b0 = 0.058555891443177958410881700501704472117;
    static CONSTEXPR double b1 = 0.052700302299058421340305358171463012695;
    static CONSTEXPR double a0 = 1.0;
    static CONSTEXPR double a1 = -0.88874380625776361330991903741960413754;
#elif INTERNAL_SAMPLE_RATE == 8000
      // 0dB gain, f1=300Hz, fs=8kHz
    static CONSTEXPR double b0 = 0.110940380645014949334559162252844544128;
    static CONSTEXPR double b1 = 0.099846342580711719416619587263994617388;
    static CONSTEXPR double a0 = 1.0;
    static CONSTEXPR double a1 = -0.789213276774273331248821250483160838485;
#else
#error "Only 16 and 8kHz sampling rate is supported by the pre- and de-emphasis filters."
#endif

    static float outputGain(void) { return 12.0f; }

  private:
    double output_gain;

    EmphasisBase(const EmphasisBase&);
    EmphasisBase& operator=(const EmphasisBase&);
    
};  /* class EmphasisBase */


/**
@brief	Implements a pre-emphasis filter
@author Tobias Blomberg / SM0SVX
@date   2013-09-29
*/
class PreemphasisFilter : public EmphasisBase
{
  public:
    PreemphasisFilter(void)
    {
      std::stringstream ss;

        // First we band pass filter the signal to get rid of high and low
        // frequency components. In the 8000 kHz case, only a high pass filter
        // is needed.
#if INTERNAL_SAMPLE_RATE >= 16000
      ss << "BpBu4/300-4300 x ";
#elif INTERNAL_SAMPLE_RATE == 8000
      ss << "HpBu4/300 x ";
#endif

        // Create the filter spec with inverted transfer function to cancel
        // out the de-emphasis filter. Also we need to normalize on b0 so that
        // we get a 1 as the y(0) coeffiient. This is required by the filter
        // library.
      ss << (a0 / b0) << " " << (a1 / b0) << " / " << 1.0 << " " << (b1 / b0);
      if (!parseFilterSpec(ss.str()))
      {
        std::cerr << "***ERROR: Preemphasis filter creation error: "
                  << errorString() << std::endl;
        std::exit(1);
      }
      setOutputGain(-outputGain());
    }

  private:
    PreemphasisFilter(const PreemphasisFilter&);
    PreemphasisFilter& operator=(const PreemphasisFilter&);

}; /* class PreemphasisFilter */


/**
@brief	Implements a de-emphasis filter
@author Tobias Blomberg / SM0SVX
@date   2013-09-29
*/
class DeemphasisFilter : public EmphasisBase
{
  public:
    DeemphasisFilter(void)

    {
      std::stringstream ss;

        // First low pass filter the signal to get rid of frequency components
        // below the voice band (<300Hz).
      //ss << "HpCh6/-0.5/300 x ";

        // Create the filter specification
      ss << b0 << " " << b1 << " / " << a0 << " " << a1;
      if (!parseFilterSpec(ss.str()))
      {
        std::cerr << "***ERROR: Deemphasis filter creation error: "
                  << errorString() << std::endl;
        exit(1);
      }
      setOutputGain(outputGain());
    }

  private:
    DeemphasisFilter(const DeemphasisFilter&);
    DeemphasisFilter& operator=(const DeemphasisFilter&);

}; /* class DeemphasisFilter */


//} /* namespace */

#endif /* EMPHASIS_INCLUDED */



/*
 * This file has not been truncated
 */
