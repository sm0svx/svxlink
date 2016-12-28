/**
@file	 AsyncAudioCompressor.cpp
@brief   Contains a class to do audio compression/limiting
@author  Tobias Blomberg / SM0SVX
@date	 2006-05-01

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2004-2008 Tobias Blomberg / SM0SVX

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



/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <iostream>


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

#include "AsyncAudioCompressor.h"



/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/

using namespace std;
using namespace Async;



/****************************************************************************
 *
 * Defines & typedefs
 *
 ****************************************************************************/

// DC offset to prevent denormal
static const double DC_OFFSET = 1.0E-25;





/****************************************************************************
 *
 * Local class definitions
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Prototypes
 *
 ****************************************************************************/

// linear -> dB conversion
static inline double lin2dB( double lin )
{
    // 20 / ln( 10 )
  static const double LOG_2_DB = 8.6858896380650365530225783783321;
  return log( lin ) * LOG_2_DB;
}

// dB -> linear conversion
static inline double dB2lin( double dB )
{
    // ln( 10 ) / 20
  static const double DB_2_LOG = 0.11512925464970228420089957273422;
  return exp( dB * DB_2_LOG );
}



/****************************************************************************
 *
 * Exported Global Variables
 *
 ****************************************************************************/




/****************************************************************************
 *
 * Local Global Variables
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/

AudioCompressor::AudioCompressor(void)
  : threshdB_(0.0), ratio_(1.0), output_gain(1.0),att_(10.0), rel_(100.0),
    envdB_(DC_OFFSET)
{
} /* AudioCompressor::AudioCompressor */


AudioCompressor::~AudioCompressor(void)
{
} /* AudioCompressor::~AudioCompressor */


#if 0
void AudioCompressor::setRatio(float ratio)
{
  comp_ratio = ratio;
  attack_step = comp_ratio / (attack * sampling_rate / 1000);
  decay_step = comp_ratio / (decay * sampling_rate / 1000);
} /* AudioCompressor::setRatio */


void AudioCompressor::setAttack(unsigned attack_ms)
{
  attack = attack_ms;
  attack_step = comp_ratio / (attack * sampling_rate / 1000);
} /* AudioCompressor::setAttack */


void AudioCompressor::setDecay(unsigned decay_ms)
{
  decay = decay_ms;
  decay_step = comp_ratio / (decay * sampling_rate / 1000);
} /* AudioCompressor::setDecay */
#endif


void AudioCompressor::setOutputGain(float gain)
{
  if (gain == 0)
  {
    output_gain = dB2lin(threshdB_ * ratio_ - threshdB_);
  }
  else
  {
    output_gain = gain;
  }
  //cout << "output_gain=" << output_gain << endl;
} /* AudioCompressor::setOutputGain */


void AudioCompressor::reset(void)
{
  envdB_ = DC_OFFSET;
} /* AudioCompressor::reset */


/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/


void AudioCompressor::processSamples(float *dest, const float *src, int count)
{
  //double max_sample = 0.0;
  for (int i=0; i<count; ++i)
  {
    double rect = fabs(src[i]);	// rectify input

    /* if desired, one could use another EnvelopeDetector to smooth
     * the rectified signal.
     */

    rect += DC_OFFSET;			// add DC offset to avoid log( 0 )
    double keydB = lin2dB( rect );	// convert linear -> dB

    // threshold
    double overdB = keydB - threshdB_;	// delta over threshold
    if ( overdB < 0.0 )
	    overdB = 0.0;

    // attack/release

    overdB += DC_OFFSET;		// add DC offset to avoid denormal

    if ( overdB > envdB_ )
    {
      att_.run( overdB, envdB_ );		// attack
    }
    else
    {
      rel_.run( overdB, envdB_ );		// release
    }

    overdB = envdB_ - DC_OFFSET;		// subtract DC offset
    //cout << overdB << endl;

    /* Regarding the DC offset: In this case, since the offset is added before 
     * the attack/release processes, the envelope will never fall below the offset,
     * thereby avoiding denormals. However, to prevent the offset from causing
     * constant gain reduction, we must subtract it from the envelope, yielding
     * a minimum value of 0dB.
     */

    // transfer function
    double gr = overdB * ( ratio_ - 1.0 );    // gain reduction (dB)
    gr = dB2lin( gr );			      // convert dB -> linear

    // output gain
    dest[i] = output_gain * src[i] * gr;      // apply gain reduction to input
    
    //if (fabs(dest[i]) > max_sample)
    //  max_sample = dest[i];
  }
  
  //cout << "max_sample=" << max_sample << endl;
  
} /* AudioCompressor::writeSamples */



/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/




/*
 * This file has not been truncated
 */

