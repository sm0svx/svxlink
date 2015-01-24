/**
@file	 RtlTcp.cpp
@brief   A_brief_description_for_this_file
@author  Tobias Blomberg / SM0SVX
@date	 2014-07-16

A_detailed_description_for_this_file

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2014 Tobias Blomberg / SM0SVX

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

#include "RtlTcp.h"



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

RtlTcp::RtlTcp(const string &remote_host, uint16_t remote_port)
  : samp_rate(2048000), block_size(10*2*samp_rate/1000),
    con(remote_host, remote_port, block_size), tuner_type(TUNER_UNKNOWN),
    tuner_gain_count(0), center_fq_set(false), center_fq(100000000),
    samp_rate_set(false), gain_mode(-1), gain(GAIN_UNSET), fq_corr_set(false),
    fq_corr(0), test_mode_set(false), test_mode(false),
    use_digital_agc_set(false), use_digital_agc(false), dist_print_cnt(-1),
    reconnect_timer(1000, Timer::TYPE_PERIODIC)
{
  con.dataReceived.connect(mem_fun(*this, &RtlTcp::dataReceived));
  con.connected.connect(mem_fun(*this, &RtlTcp::connected));
  con.disconnected.connect(mem_fun(*this, &RtlTcp::disconnected));
  con.connect();

  reconnect_timer.expired.connect(
      hide(mem_fun(con, &Async::TcpClient::connect)));

  for (unsigned i=0; i<MAX_IF_GAIN_STAGES; ++i)
  {
    tuner_if_gain[i] = GAIN_UNSET;
  }
} /* RtlTcp::RtlTcp */


void RtlTcp::enableDistPrint(bool enable)
{
  dist_print_cnt = enable ? 0 : -1;
} /* RtlTcp::enableDistPrint */


void RtlTcp::setCenterFq(uint32_t fq)
{
  center_fq = fq;
  center_fq_set = true;
  sendCommand(1, fq);
} /* RtlTcp::setCenterFq */


void RtlTcp::setSampleRate(uint32_t rate)
{
  block_size = 10 * 2 * rate / 1000; // 10ms block size
  con.setRecvBufLen(block_size);
  samp_rate = rate;
  samp_rate_set = true;
  sendCommand(2, rate);
} /* RtlTcp::setSampleRate */


void RtlTcp::setGainMode(uint32_t mode)
{
  gain_mode = mode;
  sendCommand(3, mode);
} /* RtlTcp::setGainMode */


void RtlTcp::setGain(int32_t gain)
{
  this->gain = gain;
  sendCommand(4, static_cast<uint32_t>(gain));
} /* RtlTcp::setGain */


void RtlTcp::setFqCorr(uint32_t corr)
{
  fq_corr = corr;
  fq_corr_set = true;
  sendCommand(5, corr);
} /* RtlTcp::setFqCorr */


void RtlTcp::setTunerIfGain(uint16_t stage, int16_t gain)
{
  if (stage >= MAX_IF_GAIN_STAGES)
  {
    return;
  }
  tuner_if_gain[stage] = gain;
  uint32_t param = stage;
  param <<= 16;
  param |= (uint16_t)gain;
  sendCommand(6, param);
} /* RtlTcp::setTunerIfGain */


void RtlTcp::enableTestMode(bool enable)
{
  test_mode = enable;
  test_mode_set = true;
  sendCommand(7, enable ? 1 : 0);
} /* RtlTcp::enableTestMode */


void RtlTcp::enableDigitalAgc(bool enable)
{
  use_digital_agc = enable;
  use_digital_agc_set = true;
  sendCommand(8, enable ? 1 : 0);
} /* RtlTcp::enableDigitalAgc */


const char *RtlTcp::tunerTypeString(TunerType type) const
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
    default:
      return "UNKNOWN";
  }
} /* RtlTcp::tunerTypeString */


vector<int> RtlTcp::getTunerGains(void) const
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
  static const int r820t_gains[] = {
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
      return vector<int>(r820t_gains,
                         r820t_gains+sizeof(r820t_gains)/sizeof(int));
    default:
      return vector<int>(unknown_gains,
                         unknown_gains+sizeof(unknown_gains)/sizeof(int));
  }
} /* RtlTcp::getTunerGains */



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

void RtlTcp::sendCommand(char cmd, uint32_t param)
{
  if (con.isConnected())
  {
    //cout << "### sendCommand(" << (int)cmd << ", " << param << ")" << endl;
    char msg[5];
    msg[0] = cmd;
    msg[4] = param & 0xff;
    msg[3] = (param >> 8) & 0xff;
    msg[2] = (param >> 16) & 0xff;
    msg[1] = (param >> 24) & 0xff;
    con.write(msg, sizeof(msg));
  }
} /* RtlTcp::sendCommand */


void RtlTcp::connected(void)
{
  cout << "Connected to RtlTcp at " << con.remoteHost() << ":"
       << con.remotePort() << endl;
  reconnect_timer.setEnable(false);
} /* RtlTcp::connected */


void RtlTcp::disconnected(Async::TcpConnection *c,
                          Async::TcpConnection::DisconnectReason reason)
{
  tuner_type = TUNER_UNKNOWN;
  if (!reconnect_timer.isEnabled())
  {
    cout << "Disconnected from RtlTcp at " << con.remoteHost() << ":"
         << con.remotePort() << endl;
    reconnect_timer.setEnable(true);
  }
} /* RtlTcp::disconnected */


void RtlTcp::updateSettings(void)
{
  if (samp_rate_set)
  {
    setSampleRate(samp_rate);
  }
  if (fq_corr_set)
  {
    setFqCorr(fq_corr);
  }
  if (center_fq_set)
  {
    setCenterFq(center_fq);
  }
  if (gain_mode >= 0)
  {
    setGainMode(gain_mode);
  }
  if ((gain_mode == 1) && (gain < GAIN_UNSET))
  {
    setGain(gain);
  }
  for (unsigned i=0; i<MAX_IF_GAIN_STAGES; ++i)
  {
    if (tuner_if_gain[i] < GAIN_UNSET)
    {
      setTunerIfGain(i, tuner_if_gain[i]);
    }
  }
  if (test_mode_set)
  {
    enableTestMode(test_mode);
  }
  if (use_digital_agc_set)
  {
    enableDigitalAgc(use_digital_agc);
  }
} /* RtlTcp::updateSettings */


int RtlTcp::dataReceived(Async::TcpConnection *con, void *buf, int count)
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
      cout << "*** WARNING: Distorsion detected on RtlTcp tuner "
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
} /* RtlTcp::dataReceived */


/*
 * This file has not been truncated
 */
