/**
@file	 RtlTcp.cpp
@brief   An interface class for communicating to the rtl_tcp utility
@author  Tobias Blomberg / SM0SVX
@date	 2014-07-16

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
#include <sstream>
#include <cassert>
#include <cerrno>


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
  : con(remote_host, remote_port, blockSize()),
    reconnect_timer(1000, Timer::TYPE_PERIODIC)
{
  con.dataReceived.connect(mem_fun(*this, &RtlTcp::dataReceived));
  con.connected.connect(mem_fun(*this, &RtlTcp::connected));
  con.disconnected.connect(mem_fun(*this, &RtlTcp::disconnected));
  con.connect();

  reconnect_timer.expired.connect(
      hide(mem_fun(con, &Async::TcpClient<>::connect)));
} /* RtlTcp::RtlTcp */



/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/

const std::string RtlTcp::displayName(void) const
{
  ostringstream ss;
  ss << con.remoteHost() << ":" << con.remotePort();
  return ss.str();
} /* RtlTcp::displayName */


void RtlTcp::handleSetTunerIfGain(uint16_t stage, int16_t gain)
{
  uint32_t param = stage;
  param <<= 16;
  param |= (uint16_t)gain;
  sendCommand(6, param);
} /* RtlTcp::handleSetTunerIfGain */


void RtlTcp::handleSetCenterFq(uint32_t fq)
{
  sendCommand(1, fq);
} /* RtlTcp::handleSetCenterFq */


void RtlTcp::handleSetSampleRate(uint32_t rate)
{
  con.setRecvBufLen(blockSize());
  sendCommand(2, rate);
} /* RtlTcp::handleSetSampleRate */


void RtlTcp::handleSetGainMode(uint32_t mode)
{
  sendCommand(3, mode);
} /* RtlTcp::handleSetGainMode */


void RtlTcp::handleSetGain(int32_t gain)
{
  sendCommand(4, static_cast<uint32_t>(gain));
} /* RtlTcp::handleSetGain */


void RtlTcp::handleSetFqCorr(int corr)
{
  sendCommand(5, static_cast<uint32_t>(corr));
} /* RtlTcp::handleSetFqCorr */


void RtlTcp::handleEnableTestMode(bool enable)
{
  sendCommand(7, enable ? 1 : 0);
} /* RtlTcp::handleEnableTestMode */


void RtlTcp::handleEnableDigitalAgc(bool enable)
{
  sendCommand(8, enable ? 1 : 0);
} /* RtlTcp::handleEnableDigitalAgc */



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
    if (con.write(msg, sizeof(msg)) == -1)
    {
      cout << "*** ERROR: RtlTcp socket write error: " << strerror(errno) << endl;
      con.disconnect();
      disconnected(&con, TcpConnection::DR_SYSTEM_ERROR);
    }
  }
} /* RtlTcp::sendCommand */


void RtlTcp::connected(void)
{
  /*
  cout << "### Connected to RtlTcp at " << con.remoteHost() << ":"
       << con.remotePort() << endl;
  */
  reconnect_timer.setEnable(false);
} /* RtlTcp::connected */


void RtlTcp::disconnected(Async::TcpConnection *c,
                          Async::TcpConnection::DisconnectReason reason)
{
  setTunerType(TUNER_UNKNOWN);
  if (!reconnect_timer.isEnabled())
  {
    /*
    cout << "### Disconnected from RtlTcp at " << con.remoteHost() << ":"
         << con.remotePort() << endl;
    */
    reconnect_timer.setEnable(true);
    readyStateChanged();
  }
} /* RtlTcp::disconnected */


int RtlTcp::dataReceived(Async::TcpConnection *con, void *buf, int count)
{
  //cout << "### Data received: " << count << " bytes\n";
  assert(count > 0);
  if (tunerType() == 0)
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
    setTunerType(
      static_cast<TunerType>(ntohl(*reinterpret_cast<uint32_t *>(ptr+4))));
#if 0
    uint32_t tuner_gain_count = ntohl(*reinterpret_cast<uint32_t *>(ptr+8));
    cout << "Tuner type        : " << tunerTypeString() << endl;
    vector<int> int_tuner_gains = getTunerGains();
    if (int_tuner_gains.size() != tuner_gain_count)
    {
      cout << "*** WARNING: The number of tuner gains returned by rtl_tcp "
           << "(" << tuner_gain_count << ") at address " << displayName()
           << " differ from what I think it should be ("
           << int_tuner_gains.size() << ")\n";
    }
    vector<float> tuner_gains;
    tuner_gains.assign(int_tuner_gains.begin(), int_tuner_gains.end());
    transform(tuner_gains.begin(), tuner_gains.end(),
        tuner_gains.begin(), bind2nd(divides<float>(),10.0));
    cout << "Valid tuner gains : ";
    copy(tuner_gains.begin(), tuner_gains.end(),
         ostream_iterator<float>(cout, " "));
    cout << endl;
#endif
    readyStateChanged();
    updateSettings();
    return 12;
  }

  if (static_cast<size_t>(count) < blockSize())
  {
    return 0;
  }
  count = blockSize();

  int samp_count = count / 2;
  complex<uint8_t> *samples = reinterpret_cast<complex<uint8_t>*>(buf);
  handleIq(samples, samp_count);

  return 2 * samp_count;
} /* RtlTcp::dataReceived */


/*
 * This file has not been truncated
 */
