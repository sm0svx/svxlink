/**
@file	 AudioDecoderSpeex.cpp
@brief   An audio decoder that use the Speex audio codec
@author  Tobias Blomberg / SM0SVX
@date	 2008-10-15

\verbatim
Async - A library for programming event driven applications
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



/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <iostream>
#include <cstdlib>


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

#include "AsyncAudioDecoderSpeex.h"



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

AudioDecoderSpeex::AudioDecoderSpeex(void)
{
  speex_bits_init(&bits);
#if INTERNAL_SAMPLE_RATE == 16000
  dec_state = speex_decoder_init(&speex_wb_mode);
#else
  dec_state = speex_decoder_init(&speex_nb_mode);
#endif
  speex_decoder_ctl(dec_state, SPEEX_GET_FRAME_SIZE, &frame_size);
  
  //enableEnhancer(false);
} /* AudioDecoderSpeex::AudioDecoderSpeex */


AudioDecoderSpeex::~AudioDecoderSpeex(void)
{
  speex_bits_destroy(&bits);
  speex_decoder_destroy(dec_state);
} /* AudioDecoderSpeex::~AudioDecoderSpeex */


void AudioDecoderSpeex::setOption(const std::string &name,
      	      	      	      	  const std::string &value)
{
  if (name == "ENHANCER")
  {
    enableEnhancer(atoi(value.c_str()) != 0);
  }
  else
  {
    cerr << "*** WARNING AudioDecoderSpeex: Unknown option \""
      	 << name << "\". Ignoring it.\n";
  }
} /* AudioDecoderSpeex::setOption */


void AudioDecoderSpeex::printCodecParams(void) const
{
  cout << "------ Speex decoder parameters ------\n";
  cout << "Frame size = " << frameSize() << endl;
  cout << "Enhancer   = " << (enhancerEnabled() ? "EN" : "DIS") << "ABLED\n";
  cout << "--------------------------------------\n";
} /* AudioDecoderSpeex::printCodecParams */


bool AudioDecoderSpeex::enableEnhancer(bool enable)
{
  int enh = enable ? 1 : 0;
  speex_decoder_ctl(dec_state, SPEEX_SET_ENH, &enh);
  return enhancerEnabled();
} /* AudioDecoderSpeex::enableEnhancer */


bool AudioDecoderSpeex::enhancerEnabled(void) const
{
  int enh;
  speex_decoder_ctl(dec_state, SPEEX_GET_ENH, &enh);
  return (enh != 0);
} /* AudioDecoderSpeex::enhancerEnabled */




void AudioDecoderSpeex::writeEncodedSamples(void *buf, int size)
{
  char *ptr = (char *)buf;
  
  speex_bits_read_from(&bits, ptr, size);
  float samples[frame_size];
#if SPEEX_MAJOR > 1 || (SPEEX_MAJOR == 1 && SPEEX_MINOR >= 1)
  while (speex_decode(dec_state, &bits, samples) == 0)
#else
  while ((speex_decode(dec_state, &bits, samples) == 0) &&
         (speex_bits_remaining(&bits) > 0))
#endif
  {
    for (int i=0; i<frame_size; ++i)
    {
      samples[i] = samples [i] / 32767.0;
    }
    sinkWriteSamples(samples, frame_size);
  }
} /* AudioDecoderSpeex::writeEncodedSamples */



/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/



/*
 * This file has not been truncated
 */

