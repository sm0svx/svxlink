/**
@file	 AsyncAudioEncoderOpus.cpp
@brief   An audio encoder that encodes samples using the Opus codec
@author  Tobias Blomberg / SM0SVX
@date	 2013-10-12

\verbatim
Async - A library for programming event driven applications
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



/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <iostream>
#include <cassert>
#include <cstdlib>
#include <sstream>


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

AudioEncoderOpus::AudioEncoderOpus(void)
  : enc(0), frame_size(0), sample_buf(0), buf_len(0)
{
  int error;
  enc = opus_encoder_create(INTERNAL_SAMPLE_RATE, 1, OPUS_APPLICATION_AUDIO,
                            &error);
  if (error != OPUS_OK)
  {
    cerr << "*** ERROR: Opus encoder error: " << opus_strerror(error) << endl;
    exit(1);
  }

  setFrameSize(20);
  setBitrate(20000);
  enableVbr(true);
  setMaxBandwidth(OPUS_BANDWIDTH_MEDIUMBAND);
  setBandwidth(OPUS_AUTO);
  setSignalType(OPUS_SIGNAL_VOICE);
  enableDtx(false);
#if OPUS_MAJOR > 0
  setLsbDepth(16);
#endif

} /* AsyncAudioEncoderOpus::AsyncAudioEncoderOpus */


AudioEncoderOpus::~AudioEncoderOpus(void)
{
  delete [] sample_buf;
  opus_encoder_destroy(enc);
} /* AsyncAudioEncoderOpus::~AsyncAudioEncoderOpus */


void AudioEncoderOpus::setOption(const std::string &name,
      	      	    	      	 const std::string &value)
{
#if 0
  if (name == "FRAMES_PER_PACKET")
  {
    setFramesPerPacket(atoi(value.c_str()));
  }
#endif
  if (name == "FRAME_SIZE")
  {
    stringstream ss(value);
    float frame_size;
    if (ss >> frame_size)
    {
      setFrameSize(frame_size);
    }
  }
  else if (name == "COMPLEXITY")
  {
    setComplexity(atoi(value.c_str()));
  }
  else if (name == "BITRATE")
  {
    setBitrate(atoi(value.c_str()));
  }
  else if (name == "VBR")
  {
    enableVbr(atoi(value.c_str()) != 0);
  }
  else if (name == "CVBR")
  {
    enableConstrainedVbr(atoi(value.c_str()) != 0);
  }
  else
  {
    cerr << "*** WARNING AudioEncoderOpus: Unknown option \""
      	 << name << "\". Ignoring it.\n";
  }
} /* AudioEncoderOpus::setOption */


void AudioEncoderOpus::printCodecParams(void)
{
  cout << "------ Opus encoder parameters ------\n";
  cout << "Frame size           = " << frameSize() << endl;
  cout << "Complexity           = " << complexity() << endl;
  cout << "Bitrate              = " << bitrate() << endl;
  cout << "VBR                  = "
       << (vbrEnabled() ? "YES" : "NO") << endl;
  cout << "Constrained VBR      = "
       << (constrainedVbrEnabled() ? "YES" : "NO") << endl;
  cout << "Maximum audio bw     = " << bandwidthStr(maxBandwidth()) << endl;
  cout << "Audio bw             = " << bandwidthStr(bandwidth()) << endl;
  cout << "Signal type          = " << signalTypeStr(signalType()) << endl;
  cout << "Application type     = "
       << applicationTypeStr(applicationType()) << endl;
  cout << "Inband FEC           = "
       << (inbandFecEnabled() ? "YES" : "NO") << endl;
  cout << "Expected Packet Loss = " << expectedPacketLoss() << "%\n";
  cout << "DTX                  = " << (dtxEnabled() ? "YES" : "NO") << endl;
#if OPUS_MAJOR > 0
  cout << "LSB depth            = " << lsbDepth() << endl;
#endif
  cout << "--------------------------------------\n";
} /* AudioEncoderOpus::printCodecParams */


float AudioEncoderOpus::setFrameSize(float new_frame_size_ms)
{
    // The frame size may be 2.5, 5, 10, 20, 40 or 60 ms
  frame_size =
    static_cast<int>(new_frame_size_ms * INTERNAL_SAMPLE_RATE / 1000);
  delete [] sample_buf;
  sample_buf = new float[frame_size];
  return new_frame_size_ms;
} /* AudioEncoderOpus::setFrameSize */


opus_int32 AudioEncoderOpus::setComplexity(opus_int32 new_comp)
{
  int err = opus_encoder_ctl(enc, OPUS_SET_COMPLEXITY(new_comp));
  if (err != OPUS_OK)
  {
    cerr << "*** ERROR: Could not set Opus encoder complexity: "
         << opus_strerror(err) << endl;
  }
  return complexity();
} /* AudioEncoderOpus::setBitrate */


opus_int32 AudioEncoderOpus::complexity(void)
{
  opus_int32 comp;
    // coverity[ptr_arith]
  int err = opus_encoder_ctl(enc, OPUS_GET_COMPLEXITY(&comp));
  if (err != OPUS_OK)
  {
    cerr << "*** ERROR: Could not get Opus encoder complexity: "
         << opus_strerror(err) << endl;
    return -1;
  }
  return comp;
} /* AudioEncoderOpus::complexity */


opus_int32 AudioEncoderOpus::setBitrate(opus_int32 new_bitrate)
{
  int err = opus_encoder_ctl(enc, OPUS_SET_BITRATE(new_bitrate));
  if (err != OPUS_OK)
  {
    cerr << "*** ERROR: Could not set Opus encoder bitrate: "
         << opus_strerror(err) << endl;
  }
  return bitrate();
} /* AudioEncoderOpus::setBitrate */


opus_int32 AudioEncoderOpus::bitrate(void)
{
  opus_int32 br;
    // coverity[ptr_arith]
  int err = opus_encoder_ctl(enc, OPUS_GET_BITRATE(&br));
  if (err != OPUS_OK)
  {
    cerr << "*** ERROR: Could not get Opus encoder bitrate: "
         << opus_strerror(err) << endl;
    return -1;
  }
  return br;
} /* AudioEncoderOpus::bitrate */


bool AudioEncoderOpus::enableVbr(bool enable)
{
  opus_int32 do_enable = enable ? 1 : 0;
  int err = opus_encoder_ctl(enc, OPUS_SET_VBR(do_enable));
  if (err != OPUS_OK)
  {
    cerr << "*** ERROR: Could set Opus encoder VBR: "
         << opus_strerror(err) << endl;
  }
  return vbrEnabled();
} /* AudioEncoderOpus::enableVbr */


bool AudioEncoderOpus::vbrEnabled(void)
{
  opus_int32 enabled;
    // coverity[ptr_arith]
  int err = opus_encoder_ctl(enc, OPUS_GET_VBR(&enabled));
  if (err != OPUS_OK)
  {
    cerr << "*** ERROR: Could not get Opus encoder VBR: "
         << opus_strerror(err) << endl;
    return false;
  }
  return (enabled != 0);
} /* AudioEncoderOpus::vbrEnabled */


bool AudioEncoderOpus::enableConstrainedVbr(bool enable)
{
  opus_int32 do_enable = enable ? 1 : 0;
  int err = opus_encoder_ctl(enc, OPUS_SET_VBR_CONSTRAINT(do_enable));
  if (err != OPUS_OK)
  {
    cerr << "*** ERROR: Could not set Opus encoder constrained VBR: "
         << opus_strerror(err) << endl;
  }
  return constrainedVbrEnabled();
} /* AudioEncoderOpus::enableConstrainedVbr */


bool AudioEncoderOpus::constrainedVbrEnabled(void)
{
  opus_int32 enabled;
    // coverity[ptr_arith]
  int err = opus_encoder_ctl(enc, OPUS_GET_VBR_CONSTRAINT(&enabled));
  if (err != OPUS_OK)
  {
    cerr << "*** ERROR: Could not get Opus encoder constrained VBR: "
         << opus_strerror(err) << endl;
    return false;
  }
  return (enabled != 0);
} /* AudioEncoderOpus::constrainedVbrEnabled */


opus_int32 AudioEncoderOpus::setMaxBandwidth(opus_int32 new_bw)
{
  int err = opus_encoder_ctl(enc, OPUS_SET_MAX_BANDWIDTH(new_bw));
  if (err != OPUS_OK)
  {
    cerr << "*** ERROR: Could not set Opus encoder max bandwidth: "
         << opus_strerror(err) << endl;
  }
  return maxBandwidth();
} /* AudioEncoderOpus::setMaxBandwidth */


opus_int32 AudioEncoderOpus::maxBandwidth(void)
{
  opus_int32 bw;
    // coverity[ptr_arith]
  int err = opus_encoder_ctl(enc, OPUS_GET_MAX_BANDWIDTH(&bw));
  if (err != OPUS_OK)
  {
    cerr << "*** ERROR: Could not get Opus encoder max bandwidth: "
         << opus_strerror(err) << endl;
    return -1;
  }
  return bw;
} /* AudioEncoderOpus::maxBandwidth */


opus_int32 AudioEncoderOpus::setBandwidth(opus_int32 new_bw)
{
  int err = opus_encoder_ctl(enc, OPUS_SET_BANDWIDTH(new_bw));
  if (err != OPUS_OK)
  {
    cerr << "*** ERROR: Could not set Opus encoder bandwidth: "
         << opus_strerror(err) << endl;
  }
  return bandwidth();
} /* AudioEncoderOpus::setBandwidth */


opus_int32 AudioEncoderOpus::bandwidth(void)
{
  opus_int32 bw;
    // coverity[ptr_arith]
  int err = opus_encoder_ctl(enc, OPUS_GET_BANDWIDTH(&bw));
  if (err != OPUS_OK)
  {
    cerr << "*** ERROR: Could not get Opus encoder bandwidth: "
         << opus_strerror(err) << endl;
    return -1;
  }
  return bw;
} /* AudioEncoderOpus::bandwidth */


opus_int32 AudioEncoderOpus::setSignalType(opus_int32 new_type)
{
  int err = opus_encoder_ctl(enc, OPUS_SET_SIGNAL(new_type));
  if (err != OPUS_OK)
  {
    cerr << "*** ERROR: Could not set Opus encoder signal type: "
         << opus_strerror(err) << endl;
  }
  return signalType();
} /* AudioEncoderOpus::setSignalType */


opus_int32 AudioEncoderOpus::signalType(void)
{
  opus_int32 type;
    // coverity[ptr_arith]
  int err = opus_encoder_ctl(enc, OPUS_GET_SIGNAL(&type));
  if (err != OPUS_OK)
  {
    cerr << "*** ERROR: Could not get Opus encoder signal type: "
         << opus_strerror(err) << endl;
    return -1;
  }
  return type;
} /* AudioEncoderOpus::signalType */


opus_int32 AudioEncoderOpus::setApplicationType(opus_int32 new_app)
{
  int err = opus_encoder_ctl(enc, OPUS_SET_APPLICATION(new_app));
  if (err != OPUS_OK)
  {
    cerr << "*** ERROR: Could not set Opus encoder application type: "
         << opus_strerror(err) << endl;
  }
  return applicationType();
} /* AudioEncoderOpus::setApplicationType */


opus_int32 AudioEncoderOpus::applicationType(void)
{
  opus_int32 app;
    // coverity[ptr_arith]
  int err = opus_encoder_ctl(enc, OPUS_GET_APPLICATION(&app));
  if (err != OPUS_OK)
  {
    cerr << "*** ERROR: Could not get Opus encoder application type: "
         << opus_strerror(err) << endl;
    return -1;
  }
  return app;
} /* AudioEncoderOpus::applicationType */


bool AudioEncoderOpus::enableInbandFec(bool enable)
{
  opus_int32 do_enable = enable ? 1 : 0;
  int err = opus_encoder_ctl(enc, OPUS_SET_INBAND_FEC(do_enable));
  if (err != OPUS_OK)
  {
    cerr << "*** ERROR: Could not set Opus encoder inband FEC: "
         << opus_strerror(err) << endl;
  }
  return inbandFecEnabled();
} /* AudioEncoderOpus::enableInbandFec */


bool AudioEncoderOpus::inbandFecEnabled(void)
{
  opus_int32 enabled;
    // coverity[ptr_arith]
  int err = opus_encoder_ctl(enc, OPUS_GET_INBAND_FEC(&enabled));
  if (err != OPUS_OK)
  {
    cerr << "*** ERROR: Could not get Opus encoder inband FEC: "
         << opus_strerror(err) << endl;
    return false;
  }
  return (enabled != 0);
} /* AudioEncoderOpus::inbandFecEnabled */


opus_int32 AudioEncoderOpus::setExpectedPacketLoss(opus_int32 new_pl_perc)
{
  int err = opus_encoder_ctl(enc, OPUS_SET_PACKET_LOSS_PERC(new_pl_perc));
  if (err != OPUS_OK)
  {
    cerr << "*** ERROR: Could not set Opus encoder expected packet loss: "
         << opus_strerror(err) << endl;
  }
  return expectedPacketLoss();
} /* AudioEncoderOpus::setExpectedPacketLoss */


opus_int32 AudioEncoderOpus::expectedPacketLoss(void)
{
  opus_int32 pl_perc;
    // coverity[ptr_arith]
  int err = opus_encoder_ctl(enc, OPUS_GET_PACKET_LOSS_PERC(&pl_perc));
  if (err != OPUS_OK)
  {
    cerr << "*** ERROR: Could not get Opus encoder estimated packet loss: "
         << opus_strerror(err) << endl;
    return -1;
  }
  return pl_perc;
} /* AudioEncoderOpus::expectedPacketLoss */


bool AudioEncoderOpus::enableDtx(bool enable)
{
  opus_int32 do_enable = enable ? 1 : 0;
  int err = opus_encoder_ctl(enc, OPUS_SET_DTX(do_enable));
  if (err != OPUS_OK)
  {
    cerr << "*** ERROR: Could not set Opus encoder DTX: "
         << opus_strerror(err) << endl;
  }
  return dtxEnabled();
} /* AudioEncoderOpus::enableDtx */


bool AudioEncoderOpus::dtxEnabled(void)
{
  opus_int32 enabled;
    // coverity[ptr_arith]
  int err = opus_encoder_ctl(enc, OPUS_GET_DTX(&enabled));
  if (err != OPUS_OK)
  {
    cerr << "*** ERROR: Could not get Opus encoder DTX: "
         << opus_strerror(err) << endl;
    return false;
  }
  return (enabled != 0);
} /* AudioEncoderOpus::dtxEnabled */


#if OPUS_MAJOR > 0
opus_int32 AudioEncoderOpus::setLsbDepth(opus_int32 new_depth)
{
  int err = opus_encoder_ctl(enc, OPUS_SET_LSB_DEPTH(new_depth));
  if (err != OPUS_OK)
  {
    cerr << "*** ERROR: Could not set Opus encoder LSB depth: "
         << opus_strerror(err) << endl;
  }
  return lsbDepth();
} /* AudioEncoderOpus::setLsbDepth */


opus_int32 AudioEncoderOpus::lsbDepth(void)
{
  opus_int32 depth;
    // coverity[ptr_arith]
  int err = opus_encoder_ctl(enc, OPUS_GET_LSB_DEPTH(&depth));
  if (err != OPUS_OK)
  {
    cerr << "*** ERROR: Could not get Opus encoder LSB depth: "
         << opus_strerror(err) << endl;
    return -1;
  }
  return depth;
} /* AudioEncoderOpus::lsbDepth */
#endif


void AudioEncoderOpus::reset(void)
{
  int err = opus_encoder_ctl(enc, OPUS_RESET_STATE);
  if (err != OPUS_OK)
  {
    cerr << "*** ERROR: Could not reset Opus encoder: "
         << opus_strerror(err) << endl;
  }
} /* AudioEncoderOpus::reset */


#if 0
void AudioEncoderOpus::setFramesPerPacket(unsigned fpp)
{
  frames_per_packet = fpp;
} /* AudioEncoderOpus::setFramesPerPacket */
#endif


const char *AudioEncoderOpus::bandwidthStr(opus_int32 bw)
{
  switch (bw)
  {
    case OPUS_AUTO:
      return "AUTO";
    case OPUS_BANDWIDTH_NARROWBAND:
      return "NARROWBAND";
    case OPUS_BANDWIDTH_MEDIUMBAND:
      return "MEDIUMBAND";
    case OPUS_BANDWIDTH_WIDEBAND:
      return "WIDEBAND";
    case OPUS_BANDWIDTH_SUPERWIDEBAND:
      return "SUPERWIDEBAND";
    case OPUS_BANDWIDTH_FULLBAND:
      return "FULLBAND";
    default:
      return "?";
  }
} /* AudioEncoderOpus::bandwidthStr */


const char *AudioEncoderOpus::signalTypeStr(opus_int32 type)
{
  switch (type)
  {
    case OPUS_AUTO:
      return "AUTO";
    case OPUS_SIGNAL_VOICE:
      return "VOICE";
    case OPUS_SIGNAL_MUSIC:
      return "MUSIC";
    default:
      return "?";
  }
} /* AudioEncoderOpus::signalTypeStr */


const char *AudioEncoderOpus::applicationTypeStr(opus_int32 type)
{
  switch (type)
  {
    case OPUS_APPLICATION_VOIP:
      return "VOIP";
    case OPUS_APPLICATION_AUDIO:
      return "AUDIO";
    case OPUS_APPLICATION_RESTRICTED_LOWDELAY:
      return "RESTRICTED_LOWDELAY";
    default:
      return "?";
  }
} /* AudioEncoderOpus::applicationTypeStr */


int AudioEncoderOpus::writeSamples(const float *samples, int count)
{
  for (int i=0; i<count; ++i)
  {
    sample_buf[buf_len++] = samples[i];
    
    if (buf_len == frame_size)
    {
      buf_len = 0;
      unsigned char output_buf[4000];
      opus_int32 nbytes = opus_encode_float(enc, sample_buf, frame_size,
                                            output_buf, sizeof(output_buf));
      //cout << "### frame_size=" << frame_size << " nbytes=" << nbytes << endl;
      if (nbytes > 0)
      {
        writeEncodedSamples(output_buf, nbytes);
      }
      else if (nbytes < 0)
      {
        cerr << "**** ERROR: Opus encoder error: " << opus_strerror(frame_size)
             << endl;
      }
    }
  }
  
  return count;
  
} /* AudioEncoderOpus::writeSamples */




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

