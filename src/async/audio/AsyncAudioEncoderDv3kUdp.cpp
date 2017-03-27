/**
@file	 AsyncAudioEncoderDv3kUdp.cpp
@brief   An audio encoder that encodes samples using the Dv3kUdp codec
@author  Tobias Blomberg / SM0SVX
@date	 2017-03-12

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

#include <AsyncUdpHandler.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "AsyncAudioEncoderDv3kUdp.h"



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

AudioEncoderDv3kUdp::AudioEncoderDv3kUdp(void)
  : port(0), host("")
{
} /* AsyncAudioEncoderDv3kUdp::AsyncAudioEncoderDv3kUdp */


AudioEncoderDv3kUdp::~AudioEncoderDv3kUdp(void)
{
  delete m_udp_sock;
  delete dns;
} /* AsyncAudioEncoderDv3kUdp::~AsyncAudioEncoderDv3kUdp */


void AudioEncoderDv3kUdp::setOption(const std::string &name,
      	      	    	      	 const std::string &value)
{
  if (name == "PORT")
  {
    port = atoi(value.c_str());
  }
  if (name == "HOST")
  {
    host = value;
  }

  if (port > 0 && !host.empty())
  {
    connect();
  }
} /* AudioEncoderDv3kUdp::setOption */


int AudioEncoderDv3kUdp::writeSamples(const float *samples, int count)
{
  return count;
} /* AudioEncoderDv3kUdp::writeSamples */



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

void AudioEncoderDv3kUdp::connect(void)
{
  if (ip_addr.isEmpty())
  {
    dns = new DnsLookup(host);
    dns->resultsReady.connect(mem_fun(*this,
        &AudioEncoderDv3kUdp::dnsResultReady));
    return;
  }

  cout << name() << ": Encoder try to connect AMBEServer " << ip_addr.toString()
       << ":" << port << endl;

  m_udp_sock = new Async::UdpHandler(port, ip_addr);
  m_udp_sock->dataReceived.connect(
      mem_fun(*this, &AudioEncoderDv3kUdp::onDataReceived));
  m_udp_sock->open();

} /* AudioEncoderDv3kUdp::connect */


void AudioEncoderDv3kUdp::dnsResultReady(DnsLookup& dns_lookup)
{
  vector<IpAddress> result = dns->addresses();

  delete dns;
  dns = 0;

  if (result.empty() || result[0].isEmpty())
  {
    ip_addr.clear();
    return;
  }

  ip_addr = result[0];
  connect();
} /* AudioEncoderDv3kUdp::dnsResultReady */


void AudioEncoderDv3kUdp::onDataReceived(const IpAddress& addr, uint16_t port,
                                         void *buf, int count)
{
  cout << "### " << name() << ": AudioEncoderDv3kUdpLogic::onDataReceived: "
       << "addr=" << addr << " port=" << port << " count=" << count << endl;

  float *samples = reinterpret_cast<float*>(buf);

//  sinkWriteSamples(samples, count);

} /* AudioDecoderDv3kUdp::onDataReceived */


/*
 * This file has not been truncated
 */

