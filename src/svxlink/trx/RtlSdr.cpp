/**
@file	 RtlSdr.cpp
@brief   A base class for communicating to a RTL2832u based DVB-T dongle
@author  Tobias Blomberg / SM0SVX
@date	 2015-04-10

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2015 Tobias Blomberg / SM0SVX

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

#include <cstring>
#include <cstdlib>
#include <iterator>
#include <algorithm>
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

#include "RtlSdr.h"



/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/

using namespace std;



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

RtlSdr::RtlSdr(void)
  : samp_rate(2048000), block_size(10*2*samp_rate/1000),
    tuner_type(TUNER_UNKNOWN), center_fq_set(false), center_fq(100000000),
    samp_rate_set(false), gain_mode(-1), gain(GAIN_UNSET), fq_corr_set(false),
    fq_corr(0), test_mode_set(false), test_mode(false),
    use_digital_agc_set(false), use_digital_agc(false), dist_print_cnt(-1)
{
  for (unsigned i=0; i<MAX_IF_GAIN_STAGES; ++i)
  {
    tuner_if_gain[i] = GAIN_UNSET;
  }
} /* RtlSdr::RtlSdr */


RtlSdr::~RtlSdr(void)
{
}


void RtlSdr::enableDistPrint(bool enable)
{
  dist_print_cnt = enable ? 0 : -1;
} /* RtlSdr::enableDistPrint */


void RtlSdr::setCenterFq(uint32_t fq)
{
  center_fq = fq;
  center_fq_set = true;
  handleSetCenterFq(fq);
} /* RtlSdr::setCenterFq */


void RtlSdr::setSampleRate(uint32_t rate)
{
  block_size = 10 * 2 * rate / 1000; // 10ms block size
  samp_rate = rate;
  samp_rate_set = true;
  handleSetSampleRate(rate);
} /* RtlSdr::setSampleRate */


void RtlSdr::setGainMode(uint32_t mode)
{
  gain_mode = mode;
  handleSetGainMode(mode);
} /* RtlSdr::setGainMode */


void RtlSdr::setGain(int32_t gain)
{
  this->gain = gain;
  handleSetGain(gain);
} /* RtlSdr::setGain */


void RtlSdr::setFqCorr(uint32_t corr)
{
  fq_corr = corr;
  fq_corr_set = true;
  handleSetFqCorr(corr);
} /* RtlSdr::setFqCorr */


void RtlSdr::setTunerIfGain(uint16_t stage, int16_t gain)
{
  if (stage >= MAX_IF_GAIN_STAGES)
  {
    return;
  }
  tuner_if_gain[stage] = gain;
  handleSetTunerIfGain(stage, gain);
} /* RtlSdr::setTunerIfGain */


void RtlSdr::enableTestMode(bool enable)
{
  test_mode = enable;
  test_mode_set = true;
  handleEnableTestMode(enable);
} /* RtlSdr::enableTestMode */


void RtlSdr::enableDigitalAgc(bool enable)
{
  use_digital_agc = enable;
  use_digital_agc_set = true;
  handleEnableDigitalAgc(enable);
} /* RtlSdr::enableDigitalAgc */


const char *RtlSdr::tunerTypeString(TunerType type) const
{
  switch (type)
  {
    case TUNER_E4000:
      return "E4000";
    case TUNER_FC0012:
      return "FC0012";
    case TUNER_FC0013:
      return "FC0013";
    case TUNER_FC2580:
      return "FC2580";
    case TUNER_R820T:
      return "R820T";
    case TUNER_R828D:
      return "R828D";
    default:
      return "UNKNOWN";
  }
} /* RtlSdr::tunerTypeString */


vector<int> RtlSdr::getTunerGains(void) const
{
  static const int e4k_gains[] = {
    -10, 15, 40, 65, 90, 115, 140, 165, 190, 215, 240, 290, 340, 420
  };
  static const int fc0012_gains[] = { -99, -40, 71, 179, 192 };
  static const int fc0013_gains[] = {
    -99, -73, -65, -63, -60, -58, -54, 58, 61, 63, 65, 67,
    68, 70, 71, 179, 181, 182, 184, 186, 188, 191, 197
  };
  static const int fc2580_gains[] = { 0 /* no gain values */ };
  static const int r82xx_gains[] = {
    0, 9, 14, 27, 37, 77, 87, 125, 144, 157, 166, 197, 207, 229, 254, 280,
    297, 328, 338, 364, 372, 386, 402, 421, 434, 439, 445, 480, 496
  };
  static const int unknown_gains[] = { 0 /* no gain values */ };

  switch (tuner_type)
  {
    case TUNER_E4000:
      return vector<int>(e4k_gains,
                         e4k_gains+sizeof(e4k_gains)/sizeof(int));
    case TUNER_FC0012:
      return vector<int>(fc0012_gains,
                         fc0012_gains+sizeof(fc0012_gains)/sizeof(int));
    case TUNER_FC0013:
      return vector<int>(fc0013_gains,
                         fc0013_gains+sizeof(fc0013_gains)/sizeof(int));
    case TUNER_FC2580:
      return vector<int>(fc2580_gains,
                         fc2580_gains+sizeof(fc2580_gains)/sizeof(int));
    case TUNER_R820T:
    case TUNER_R828D:
      return vector<int>(r82xx_gains,
                         r82xx_gains+sizeof(r82xx_gains)/sizeof(int));
    default:
      return vector<int>(unknown_gains,
                         unknown_gains+sizeof(unknown_gains)/sizeof(int));
  }
} /* RtlSdr::getTunerGains */



/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/

void RtlSdr::handleIq(const complex<uint8_t> *samples, int samp_count)
{
  //cout << "RtlSdr::handleIq: samp_count=" << samp_count << endl;

  vector<Sample> iq;
  iq.reserve(samp_count);
  for (int idx=0; idx<samp_count; ++idx)
  {
    if ((dist_print_cnt == 0) &&
        ((samples[idx].real() == 255) || (samples[idx].imag() == 255)))
    {
      dist_print_cnt = samp_rate;
    }
    float i = samples[idx].real();
    i = i / 127.5f - 1.0f;
    float q = samples[idx].imag();
    q = q / 127.5f - 1.0f;
    iq.push_back(complex<float>(i, q));
  }

  if (dist_print_cnt > 0)
  {
    if (dist_print_cnt == static_cast<int>(samp_rate))
    {
      cout << "*** WARNING: Distortion detected on Rtl tuner "
           << displayName() << ". Lower the RF gain\n";
    }
    dist_print_cnt -= samp_count;
    if (dist_print_cnt < 0)
    {
      dist_print_cnt = 0;
    }
  }

  iqReceived(iq);
} /* RtlSdr::handleIq */



/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/

void RtlSdr::updateSettings(void)
{
  if (samp_rate_set)
  {
    handleSetSampleRate(samp_rate);
  }
  if (fq_corr_set)
  {
    handleSetFqCorr(fq_corr);
  }
  if (center_fq_set)
  {
    handleSetCenterFq(center_fq);
  }
  if (gain_mode >= 0)
  {
    handleSetGainMode(gain_mode);
  }
  if ((gain_mode == 1) && (gain < GAIN_UNSET))
  {
    handleSetGain(gain);
  }
  for (unsigned i=0; i<MAX_IF_GAIN_STAGES; ++i)
  {
    if (tuner_if_gain[i] < GAIN_UNSET)
    {
      handleSetTunerIfGain(i, tuner_if_gain[i]);
    }
  }
  if (test_mode_set)
  {
    handleEnableTestMode(test_mode);
  }
  if (use_digital_agc_set)
  {
    handleEnableDigitalAgc(use_digital_agc);
  }
} /* RtlSdr::updateSettings */


#if 0
int RtlSdr::dataReceived(Async::TcpConnection *con, void *buf, int count)
{
  //cout << "### Data received: " << count << " bytes\n";
  if (tuner_type == 0)
  {
    if (count < 12)
    {
      return 0;
    }
    char *ptr = reinterpret_cast<char *>(buf);
    if (strncmp(ptr, "RTL0", 4) != 0)
    {
      cout << "*** ERROR: Expected magic RTL0\n";
      exit(1);
    }
    tuner_type =
      static_cast<TunerType>(ntohl(*reinterpret_cast<uint32_t *>(ptr+4)));
    tuner_gain_count = ntohl(*reinterpret_cast<uint32_t *>(ptr+8));
    cout << "Tuner type        : " << tunerTypeString() << endl;
    vector<int> int_tuner_gains = getTunerGains();
    vector<float> tuner_gains;
    tuner_gains.assign(int_tuner_gains.begin(), int_tuner_gains.end());
    transform(tuner_gains.begin(), tuner_gains.end(),
        tuner_gains.begin(), bind2nd(divides<float>(),10.0));
    cout << "Valid tuner gains : ";
    copy(tuner_gains.begin(), tuner_gains.end(),
         ostream_iterator<float>(cout, " "));
    cout << endl;
    updateSettings();
    return 12;
  }

  if (count < block_size)
  {
    return 0;
  }
  count = block_size;

  int samp_count = count / 2;
  complex<uint8_t> *samples =
    reinterpret_cast<complex<uint8_t>*>(buf);
  vector<Sample> iq;
  iq.reserve(samp_count);
  for (int idx=0; idx<samp_count; ++idx)
  {
    if ((dist_print_cnt == 0) &&
        ((samples[idx].real() == 255) || (samples[idx].imag() == 255)))
    {
      dist_print_cnt = samp_rate;
    }
    float i = samples[idx].real();
    i = i / 127.5f - 1.0f;
    float q = samples[idx].imag();
    q = q / 127.5f - 1.0f;
    iq.push_back(complex<float>(i, q));
  }

  if (dist_print_cnt > 0)
  {
    if (dist_print_cnt == static_cast<int>(samp_rate))
    {
      cout << "*** WARNING: Distortion detected on RtlSdr tuner "
           << con->remoteHost() << ":" << con->remotePort() << ". "
           << "Lower the RF gain\n";
    }
    dist_print_cnt -= samp_count;
    if (dist_print_cnt < 0)
    {
      dist_print_cnt = 0;
    }
  }

  iqReceived(iq);

  return 2 * samp_count;
} /* RtlSdr::dataReceived */
#endif


/*
 * This file has not been truncated
 */
