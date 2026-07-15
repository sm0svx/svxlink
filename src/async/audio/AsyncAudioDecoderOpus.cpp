/**
@file	 AudioDecoderOpus.cpp
@brief   An audio decoder that use the Opus audio codec
@author  Tobias Blomberg / SM0SVX
@date	 2013-10-12

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2003-2026 Tobias Blomberg / SM0SVX

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

#include <stdint.h>
#include <iostream>
#include <cstdlib>
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

#include "AsyncAudioEncoderOpus.h"
#include "AsyncAudioDecoderOpus.h"



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

AudioDecoderOpus::AudioDecoderOpus(void)
{
  int error;
  m_dec = opus_decoder_create(INTERNAL_SAMPLE_RATE, 1, &error);
  if (error != OPUS_OK)
  {
    cerr << "*** ERROR: Could not initialize Opus decoder\n";
    exit(1);
  }
} /* AudioDecoderOpus::AudioDecoderOpus */


AudioDecoderOpus::~AudioDecoderOpus(void)
{
  opus_decoder_destroy(m_dec);
} /* AudioDecoderOpus::~AudioDecoderOpus */


void AudioDecoderOpus::setOption(const std::string &name,
      	      	      	      	  const std::string &value)
{
#if OPUS_MAJOR > 0
  if (name == "GAIN")
  {
    setGain(atof(value.c_str()));
  }
  else
#endif
  {
    cerr << "*** WARNING AudioDecoderOpus: Unknown option \""
      	 << name << "\". Ignoring it.\n";
  }
} /* AudioDecoderOpus::setOption */


void AudioDecoderOpus::printCodecParams(void) const
{
#if OPUS_MAJOR > 0
  cout << "------ Opus decoder parameters ------\n";
  cout << "Gain       = " << gain() << "dB\n";
  cout << "--------------------------------------\n";
#endif
} /* AudioDecoderOpus::printCodecParams */


#if OPUS_MAJOR > 0
float AudioDecoderOpus::setGain(float new_gain)
{
  opus_int32 gaini = static_cast<opus_int32>(256.0f * new_gain);
  opus_decoder_ctl(m_dec, OPUS_SET_GAIN(gaini));
  return gain();
} /* AudioDecoderOpus::setGain */


float AudioDecoderOpus::gain(void) const
{
  opus_int32 gaini;
    // coverity[ptr_arith]
  opus_decoder_ctl(m_dec, OPUS_GET_GAIN(&gaini));
  return gaini / 256.0f;
} /* AudioDecoderOpus::gain */
#endif


void AudioDecoderOpus::reset(void)
{
  opus_decoder_ctl(m_dec, OPUS_RESET_STATE);
} /* AudioDecoderOpus::reset */


void AudioDecoderOpus::writeEncodedSamples(void *buf, int size)
{
  if ((size <= 0) || (size > AudioEncoderOpus::MAX_ENCODED_FRAME_SIZE))
  {
    std::cerr << "*** WARNING: AudioDecoderOpus received an encoded frame with "
                 "an out of range size (" << size << " bytes). Discarding it."
              << std::endl;
    return;
  }

  const unsigned char* const packet = reinterpret_cast<unsigned char*>(buf);

  const int frame_cnt = opus_packet_get_nb_frames(packet, size);
  if (frame_cnt == 0)
  {
    return;
  }
  else if (frame_cnt < 0)
  {
    std::cerr << "*** ERROR: Opus decoder error: " << opus_strerror(frame_cnt)
              << std::endl;
    return;
  }
  const int frame_size =
    opus_packet_get_samples_per_frame(packet, INTERNAL_SAMPLE_RATE);
  if (frame_size == 0)
  {
    return;
  }
  else if (frame_size < 0)
  {
    std::cerr << "*** ERROR: Opus decoder error: " << opus_strerror(frame_size)
              << std::endl;
    return;
  }
  int channels = opus_packet_get_nb_channels(packet);
  if (channels <= 0)
  {
    std::cerr << "*** ERROR: Opus decoder error: " << opus_strerror(channels)
              << std::endl;
    return;
  }
  else if (channels != 1)
  {
    cerr << "*** ERROR: Multi channel Opus packet received but only one "
            "channel can be handled\n";
    return;
  }
    // A well-formed Opus packet decodes to at most MAX_DECODED_SAMPLES mono
    // samples. Guard against malformed packets whose advertised frame_cnt /
    // frame_size would exceed that, then decode into a fixed-size stack buffer,
    // passing the real buffer capacity to opus_decode_float() as the output
    // frame-size limit so it can never overrun the buffer.
  if ((frame_cnt * frame_size) > MAX_DECODED_SAMPLES)
  {
    std::cerr << "*** WARNING: AudioDecoderOpus packet advertises more samples "
                 "than a valid Opus packet can contain. Discarding it."
              << std::endl;
    return;
  }

  float samples[MAX_DECODED_SAMPLES];
  const int decoded_samples = opus_decode_float(m_dec, packet, size, samples,
                                                MAX_DECODED_SAMPLES, 0);
  if (decoded_samples > 0)
  {
    sinkWriteSamples(samples, decoded_samples);
  }
  else if (decoded_samples < 0)
  {
    std::cerr << "**** ERROR: Opus decoder error: "
              << opus_strerror(decoded_samples)
              << std::endl;
  }
} /* AudioDecoderOpus::writeEncodedSamples */



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

