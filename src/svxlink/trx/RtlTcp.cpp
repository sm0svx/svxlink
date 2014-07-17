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
  : con(remote_host, remote_port, BLOCK_SIZE), tuner_type(0),
    center_fq_set(false), center_fq(100000000), samp_rate_set(false),
    samp_rate(2048000), gain_mode(-1), gain(GAIN_UNSET), fq_corr_set(false),
    fq_corr(0), test_mode_set(false), test_mode(false),
    use_digital_agc_set(false), use_digital_agc(false)
{
  con.dataReceived.connect(mem_fun(*this, &RtlTcp::dataReceived));
  con.connected.connect(mem_fun(*this, &RtlTcp::connected));
  con.disconnected.connect(mem_fun(*this, &RtlTcp::disconnected));
  con.connect();

  for (int i=0; i<MAX_IF_GAIN_STAGES; ++i)
  {
    tuner_if_gain[i] = GAIN_UNSET;
  }
} /* RtlTcp::RtlTcp */


void RtlTcp::setCenterFq(uint32_t fq)
{
  center_fq = fq;
  center_fq_set = true;
  sendCommand(1, fq);
} /* RtlTcp::setCenterFq */


void RtlTcp::setSampleRate(uint32_t rate)
{
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
    cout << "### sendCommand(" << (int)cmd << ", " << param << ")" << endl;
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
  cout << "Connected\n";
} /* RtlTcp::connected */


void RtlTcp::disconnected(Async::TcpConnection *c, Async::TcpConnection::DisconnectReason reason)
{
  cout << "Disconnected\n";
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
  for (int i=0; i<MAX_IF_GAIN_STAGES; ++i)
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
    tuner_type = ntohl(*reinterpret_cast<uint32_t *>(ptr+4));
    tuner_gain_count = ntohl(*reinterpret_cast<uint32_t *>(ptr+8));
    cout << "### tuner_type=" << tuner_type
         << " tuner_gain_count=" << tuner_gain_count << endl;
    updateSettings();
    return 12;
  }

  if (count < BLOCK_SIZE)
  {
    return 0;
  }
  count = BLOCK_SIZE;

  int samp_count = count / 2;
  complex<uint8_t> *samples =
    reinterpret_cast<complex<uint8_t>*>(buf);
  vector<Sample> iq;
  iq.reserve(samp_count);
  for (int idx=0; idx<samp_count; ++idx)
  {
    float i = samples[idx].real();
    i = i / 127.5f - 1.0f;
    float q = samples[idx].imag();
    q = q / 127.5f - 1.0f;
    iq.push_back(complex<float>(i, q));
  }
  iqReceived(iq);

    // FIXME: Returning 0 terminates the connection. This have to be
    // handled. Fix Async::TcpConnection?
  return 2 * samp_count;
} /* RtlTcp::dataReceived */


/*
 * This file has not been truncated
 */

