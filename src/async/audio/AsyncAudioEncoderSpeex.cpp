/**
@file	 AsyncAudioEncoderSpeex.cpp
@brief   An audio encoder that encodes samples using the Speex codec
@author  Tobias Blomberg / SM0SVX
@date	 2008-10-16

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
#include <cassert>
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

#include "AsyncAudioEncoderSpeex.h"



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

AudioEncoderSpeex::AudioEncoderSpeex(void)
  : buf_len(0), frames_per_packet(4), frame_cnt(0)
{
  speex_bits_init(&bits);
#if INTERNAL_SAMPLE_RATE == 16000
  enc_state = speex_encoder_init(&speex_wb_mode);
#else
  enc_state = speex_encoder_init(&speex_nb_mode);
#endif
  speex_encoder_ctl(enc_state, SPEEX_GET_FRAME_SIZE, &frame_size);
  sample_buf = new float[frame_size];
  
  //setQuality(10);
  //setComplexity(10);
  //setBitrate(8000);
  //enableVbr(true);
  //setVbrQuality(10);
  //setAbr(8000);
  //setVbrMaxBitrate(32000);
  //enableHighpass(false);
} /* AsyncAudioEncoderSpeex::AsyncAudioEncoderSpeex */


AudioEncoderSpeex::~AudioEncoderSpeex(void)
{
  delete [] sample_buf;
  speex_bits_destroy(&bits);
  speex_encoder_destroy(enc_state);
} /* AsyncAudioEncoderSpeex::~AsyncAudioEncoderSpeex */


void AudioEncoderSpeex::setOption(const std::string &name,
      	      	      	      	  const std::string &value)
{
  if (name == "FRAMES_PER_PACKET")
  {
    setFramesPerPacket(atoi(value.c_str()));
  }
  else if (name == "QUALITY")
  {
    setQuality(atoi(value.c_str()));
  }
  else if (name == "BITRATE")
  {
    setBitrate(atoi(value.c_str()));
  }
  else if (name == "COMPLEXITY")
  {
    setComplexity(atoi(value.c_str()));
  }
  else if (name == "VBR")
  {
    enableVbr(atoi(value.c_str()) != 0);
  }
  else if (name == "VBR_QUALITY")
  {
    setVbrQuality(atoi(value.c_str()));
  }
  else if (name == "ABR")
  {
    setAbr(atoi(value.c_str()));
  }
  else
  {
    cerr << "*** WARNING AudioEncoderSpeex: Unknown option \""
      	 << name << "\". Ignoring it.\n";
  }
} /* AudioEncoderSpeex::setOption */


void AudioEncoderSpeex::printCodecParams(void)
{
  cout << "------ Speex encoder parameters ------\n";
  cout << "Frame size      = " << frameSize() << endl;
  cout << "Bitrate         = " << bitrate() << endl;
  cout << "Complexity      = " << complexity() << endl;
  //cout << "VBR quality = " << vbrQuality() << endl;
  cout << "ABR             = " << abr() << endl;
  cout << "VBR enabled     = " << (vbrEnabled() ? "EN" : "DIS") << "ABLED\n";
  //cout << "VBR max bitrate = " << vbrMaxBitrate() << endl;
  //cout << "Highpass filter = " << (highpassEnabled() ? "EN" : "DIS")
  //     << "ABLED\n";
  cout << "--------------------------------------\n";
} /* AudioEncoderSpeex::printCodecParams */


void AudioEncoderSpeex::setFramesPerPacket(unsigned fpp)
{
  frames_per_packet = fpp;
} /* AudioEncoderSpeex::setFramesPerPacket */


void AudioEncoderSpeex::setQuality(int quality)
{
  speex_encoder_ctl(enc_state, SPEEX_SET_QUALITY, &quality);
} /* AudioEncoderSpeex::setQuality */


int AudioEncoderSpeex::setBitrate(int new_bitrate)
{
  speex_encoder_ctl(enc_state, SPEEX_SET_BITRATE, &new_bitrate);
  return bitrate();
} /* AudioEncoderSpeex::setBitrate */


int AudioEncoderSpeex::bitrate(void)
{
  int br;
  speex_encoder_ctl(enc_state, SPEEX_GET_BITRATE, &br);
  return br;
} /* AudioEncoderSpeex::bitrate */


int AudioEncoderSpeex::setComplexity(int new_comp)
{
  speex_encoder_ctl(enc_state, SPEEX_SET_COMPLEXITY, &new_comp);
  return complexity();
} /* AudioEncoderSpeex::setBitrate */


int AudioEncoderSpeex::complexity(void)
{
  int comp;
  speex_encoder_ctl(enc_state, SPEEX_GET_COMPLEXITY, &comp);
  return comp;
} /* AudioEncoderSpeex::complexity */


void AudioEncoderSpeex::enableVbr(bool enable)
{
  int do_enable = enable ? 1 : 0;
  speex_encoder_ctl(enc_state, SPEEX_SET_VBR, &do_enable);
} /* AudioEncoderSpeex::enableVbr */


bool AudioEncoderSpeex::vbrEnabled(void)
{
  int enabled;
  speex_encoder_ctl(enc_state, SPEEX_GET_VBR, &enabled);
  return (enabled != 0);
} /* AudioEncoderSpeex::vbrEnabled */


int AudioEncoderSpeex::setVbrQuality(int quality)
{
  speex_encoder_ctl(enc_state, SPEEX_SET_VBR_QUALITY, &quality);
  return vbrQuality();
} /* AudioEncoderSpeex::setVbrQuality */


int AudioEncoderSpeex::vbrQuality(void)
{
  int quality;
  speex_encoder_ctl(enc_state, SPEEX_GET_VBR_QUALITY, &quality);
  return quality;
} /* AudioEncoderSpeex::vbrQuality */


#if 0
int AudioEncoderSpeex::setVbrMaxBitrate(int bitrate)
{
  speex_encoder_ctl(enc_state, SPEEX_SET_VBR_MAX_BITRATE, &bitrate);
  return vbrMaxBitrate();
} /* AudioEncoderSpeex::setVbrMaxBitrate */


int AudioEncoderSpeex::vbrMaxBitrate(void)
{
  int bitrate;
  speex_encoder_ctl(enc_state, SPEEX_GET_VBR_MAX_BITRATE, &bitrate);
  return bitrate;
} /* AudioEncoderSpeex::vbrMaxBitrate */
#endif


int AudioEncoderSpeex::setAbr(int new_abr)
{
  speex_encoder_ctl(enc_state, SPEEX_SET_ABR, &new_abr);
  return abr();
} /* AudioEncoderSpeex::setAbr */


int AudioEncoderSpeex::abr(void)
{
  int a;
  speex_encoder_ctl(enc_state, SPEEX_GET_ABR, &a);
  return a;
} /* AudioEncoderSpeex::abr */


#if 0
bool AudioEncoderSpeex::enableHighpass(bool enable)
{
  int hp = enable ? 1 : 0;
  speex_encoder_ctl(enc_state, SPEEX_SET_HIGHPASS, &hp);
  return highpassEnabled();
} /* AudioEncoderSpeex::setAbr */


bool AudioEncoderSpeex::highpassEnabled(void)
{
  int hp;
  speex_encoder_ctl(enc_state, SPEEX_GET_HIGHPASS, &hp);
  return (hp != 0);
} /* AudioEncoderSpeex::highpassEnabled */
#endif



int AudioEncoderSpeex::writeSamples(const float *samples, int count)
{
  for (int i=0; i<count; ++i)
  {
    sample_buf[buf_len++] = samples[i] * 32767.0;
    
    if (buf_len == frame_size)
    {
      speex_encode(enc_state, sample_buf, &bits);
      buf_len = 0;
      
      if (++frame_cnt == frames_per_packet)
      {
        frame_cnt = 0;
      	speex_bits_insert_terminator(&bits);
        int nbytes = speex_bits_nbytes(&bits);
        char output_buf[nbytes];
        nbytes = speex_bits_write(&bits, output_buf, nbytes);
        speex_bits_reset(&bits);
        //cout << "writing " << nbytes << " bytes\n";
        writeEncodedSamples(output_buf, nbytes);
      }
    }
  }
  
  return count;
  
} /* AudioEncoderSpeex::writeSamples */




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

