/**
@file	 AsyncAudioDeviceUDP.cpp
@brief   Handle simple streaming of audio samples via UDP
@author  Tobias Blomberg / SM0SVX
@date    2012-06-25

Implements a simple "audio interface" that stream samples via
UDP. This can for example be used to stream audio to/from
GNU Radio.

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2003-2012 Tobias Blomberg / SM0SVX

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

#include <sys/ioctl.h>

#include <cassert>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <sstream>



/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncUdpSocket.h>
#include <AsyncIpAddress.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "AsyncAudioDeviceUDP.h"
#include "AsyncAudioDeviceFactory.h"



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

REGISTER_AUDIO_DEVICE_TYPE("udp", AudioDeviceUDP);



/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/

size_t AudioDeviceUDP::readBlocksize(void)
{
  return block_size;
} /* AudioDeviceUDP::readBlocksize */


size_t AudioDeviceUDP::writeBlocksize(void)
{
  return block_size;
} /* AudioDeviceUDP::writeBlocksize */


bool AudioDeviceUDP::isFullDuplexCapable(void)
{
  return true;
} /* AudioDeviceUDP::isFullDuplexCapable */


void AudioDeviceUDP::audioToWriteAvailable(void)
{
  if (!pace_timer->isEnabled())
  {
    audioWriteHandler();
  }
} /* AudioDeviceUDP::audioToWriteAvailable */


void AudioDeviceUDP::flushSamples(void)
{
  if (!pace_timer->isEnabled())
  {
    audioWriteHandler();
  }
} /* AudioDeviceUDP::flushSamples */


int AudioDeviceUDP::samplesToWrite(void) const
{
  if ((mode() != MODE_WR) && (mode() != MODE_RDWR))
  {
    return 0;
  }
  assert(sock != 0);

  int len = 0;
  if (ioctl(sock->fd(), TIOCOUTQ, &len) == -1)
  {
    return 0;
  }
  
  return len / sizeof(*read_buf);
  
} /* AudioDeviceUDP::samplesToWrite */



/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/


AudioDeviceUDP::AudioDeviceUDP(const string& dev_name)
  : AudioDevice(dev_name), block_size(0), sock(0), read_buf(0),
    read_buf_pos(0), port(0), zerofill_on_underflow(false)
{
  assert(AudioDeviceUDP_creator_registered);
  assert(sampleRate() > 0);
  size_t pace_interval = 1000 * block_size_hint / sampleRate();
  block_size = pace_interval * sampleRate() / 1000;

  read_buf = new int16_t[block_size * channels];
  pace_timer = new Timer(pace_interval, Timer::TYPE_PERIODIC);
  pace_timer->setEnable(false);
  pace_timer->expired.connect(
      sigc::hide(mem_fun(*this, &AudioDeviceUDP::audioWriteHandler)));

  char *zerofill_str = std::getenv("ASYNC_AUDIO_UDP_ZEROFILL");
  if (zerofill_str != 0)
  {
    std::istringstream(zerofill_str) >> zerofill_on_underflow;
  }
} /* AudioDeviceUDP::AudioDeviceUDP */


AudioDeviceUDP::~AudioDeviceUDP(void)
{
  delete [] read_buf;
  delete pace_timer;
} /* AudioDeviceUDP::~AudioDeviceUDP */


bool AudioDeviceUDP::openDevice(Mode mode)
{
  if (sock != 0)
  {
    closeDevice();
  }

  const string &dev_name = devName();
  size_t colon = dev_name.find(':');
  if (colon == string::npos)
  {
    cerr << "*** ERROR: Illegal UDP audio device specification (" << devName()
         << "). Should be udp:ip-addr:port\n";
    return false;
  }
  
  string ip_addr_str = dev_name.substr(0, colon);
  string port_str = dev_name.substr(colon+1);
  if (ip_addr_str.empty() || port_str.empty())
  {
    cerr << "*** ERROR: Illegal UDP audio device specification (" << devName()
         << "). Should be udp:ip-addr:port\n";
    return false;
  }

  ip_addr = IpAddress(ip_addr_str);
  port = 0;
  stringstream ss(port_str);
  ss >> port;

  switch (mode)
  {
    case MODE_WR:
      if (ip_addr.isEmpty() || (port == 0))
      {
        cerr << "*** ERROR: Illegal UDP audio device specification ("
             << devName()
             << "). Should be udp:ip-addr:port\n";
        return false;
      }
      sock = new UdpSocket;
      if (!sock->initOk())
      {
        cerr << "*** ERROR: Could not create UDP socket for writing ("
             << devName() << ")\n";
        return false;
      }
      break;
      
    case MODE_RDWR:
      if (ip_addr.isEmpty())
      {
        cerr << "*** ERROR: Illegal UDP audio device specification ("
             << devName()
             << "). Should be udp:ip-addr:port\n";
        return false;
      }
      // Fall through!
      
    case MODE_RD:
      if (port == 0)
      {
        cerr << "*** ERROR: Illegal UDP audio device specification ("
             << devName()
             << "). Should be udp:ip-addr:port\n";
        return false;
      }
      sock = new UdpSocket(port, ip_addr);
      if (!sock->initOk())
      {
        cerr << "*** ERROR: Could not bind to UDP socket (" << devName()
             << ")\n";
        return false;
      }
      sock->dataReceived.connect(
              mem_fun(*this, &AudioDeviceUDP::audioReadHandler));
      break;
      
    case MODE_NONE:
      break;
  }

  return true;
  
} /* AudioDeviceUDP::openDevice */


void AudioDeviceUDP::closeDevice(void)
{
  pace_timer->setEnable(false);
  delete sock;
  sock = 0;
  ip_addr = IpAddress();
  port = 0;
} /* AudioDeviceUDP::closeDevice */



/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/


void AudioDeviceUDP::audioReadHandler(const IpAddress &ip, uint16_t port,
                                      void *buf, int count)
{
  for (unsigned i=0; i < count / (channels * sizeof(int16_t)); ++i)
  {
    for (size_t ch=0; ch < channels; ++ch)
    {
      read_buf[read_buf_pos * channels + ch] =
              ((int16_t *)buf)[i * channels + ch];
    }
    if (++read_buf_pos == block_size)
    {
      putBlocks(read_buf, block_size);
      read_buf_pos = 0;
    }
  }
} /* AudioDeviceUDP::audioReadHandler */


void AudioDeviceUDP::audioWriteHandler(void)
{
  assert(sock != 0);
  assert((mode() == MODE_WR) || (mode() == MODE_RDWR));
  
  unsigned frags_read;
  const unsigned frag_size = block_size * sizeof(int16_t) * channels;
  int16_t buf[block_size * channels];
  frags_read = getBlocks(buf, 1);
  if (frags_read == 0)
  {
    if (zerofill_on_underflow)
    {
      frags_read = 1;
      std::memset(buf, 0, block_size * channels);
    }
    else
    {
      pace_timer->setEnable(false);
      return;
    }
  }

    // Write the samples to the socket
  if (!sock->write(ip_addr, port, (void *)buf, frag_size))
  {
    perror("write in AudioDeviceUDP::write");
    pace_timer->setEnable(false);
    return;
  }

  pace_timer->setEnable(true);

} /* AudioDeviceUDP::audioWriteHandler */



/*
 * This file has not been truncated
 */

