/**
@file	 AsyncAudioCompressor.h
@brief   Contains a class to do audio compression/limiting
@author  Tobias Blomberg / SM0SVX
@date	 2006-05-01

\verbatim
<A brief description of the program or library this file belongs to>
Copyright (C) 2004-2005  Tobias Blomberg / SM0SVX

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


#ifndef ASYNC_AUDIO_COMPRESSOR_INCLUDED
#define ASYNC_AUDIO_COMPRESSOR_INCLUDED


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

#include "AsyncAudioProcessor.h"


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

class EnvelopeDetector
{
  public:
    EnvelopeDetector( double ms = 1.0, double sampleRate = 8000.0 )
      : sampleRate_( sampleRate ), ms_( ms ), coef_( 0.0 )
    {
      setCoef();
    }

    virtual ~EnvelopeDetector() {}

    // time constant
    virtual void   setTc( double ms )
    {
      ms_ = ms;
      setCoef();
    }

    virtual double getTc( void ) { return ms_; }

    // sample rate
    virtual void   setSampleRate( double sampleRate )
    {
      sampleRate_ = sampleRate;
      setCoef();
    }

    virtual double getSampleRate( void ) { return sampleRate_; }

    // runtime function
    inline void run( double in, double &state )
    {
      state = in + coef_ * ( state - in );
    }

  private:
    double sampleRate_;		// sample rate
    double ms_;				// time constant in ms
    double coef_;			// runtime coefficient
    
    void setCoef( void )	// coef algorithm
    {
      coef_ = exp( -1.0 / ( 0.001 * ms_ * sampleRate_ ) );
    }

};	// end EnvelopeDetector class





/**
@brief	A class to do audio compression/limiting
@author Tobias Blomberg / SM0SVX
@date   2006-05-01

A_detailed_class_description

\include AudioCompressor_demo.cpp
*/
class AudioCompressor : public AudioProcessor
{
  public:
    /**
     * @brief 	Default constuctor
     */
    AudioCompressor(void);
  
    /**
     * @brief 	Destructor
     */
    ~AudioCompressor(void);
  
    /**
     * @brief 	A_brief_member_function_description
     * @param 	param1 Description_of_param1
     * @return	Return_value_of_this_member_function
     */
    void setThreshold(double thresh_db) { threshdB_ = thresh_db; }
    void setRatio(double ratio) { ratio_ = ratio; }
    void setAttack(double attack_ms) { att_.setTc(attack_ms);}
    void setDecay(double decay_ms) { rel_.setTc(decay_ms); }
    void setOutputGain(float gain);
    void reset(void);
    
  protected:
    void processSamples(float *dest, const float *src, int count);
    
    
  private:
    // transfer function
    double threshdB_;	// threshold (dB)
    double ratio_;		// ratio (compression: < 1 ; expansion: > 1)
    double output_gain;

    // attack/release
    EnvelopeDetector att_;	// attack
    EnvelopeDetector rel_;	// release

    // runtime variables
    double envdB_;			// over-threshold envelope (dB)
    
    AudioCompressor(const AudioCompressor&);
    AudioCompressor& operator=(const AudioCompressor&);
    
};  /* class AudioCompressor */


} /* namespace */

#endif /* ASYNC_AUDIO_COMPRESSOR_INCLUDED */



/*
 * This file has not been truncated
 */

