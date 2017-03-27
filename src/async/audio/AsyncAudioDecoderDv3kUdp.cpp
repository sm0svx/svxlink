/**
@file	 AudioDecoderDv3kUdp.cpp
@brief   An audio decoder that use the Dv3kUdp audio codec
@author  Tobias Blomberg / SM0SVX & Adi Bier / DL1HRC
@date	 2017-03-16

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2003-2017 Tobias Blomberg / SM0SVX

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

#include <AsyncUdpHandler.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "AsyncAudioDecoderDv3kUdp.h"


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

AudioDecoderDv3kUdp::AudioDecoderDv3kUdp(void)
  : port(0), host("")
{
} /* AudioDecoderDv3kUdp::AudioDecoderDv3kUdp */


AudioDecoderDv3kUdp::~AudioDecoderDv3kUdp(void)
{
  delete m_udp_sock;
  delete dns;
} /* AudioDecoderDv3kUdp::~AudioDecoderDv3kUdp */


void AudioDecoderDv3kUdp::setOption(const std::string &name,
      	      	      	      	  const std::string &value)
{
    // AMBESERVER_PORT and AMBESERVER_HOST
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
} /* AudioDecoderDv3kUdp::setOption */


void AudioDecoderDv3kUdp::writeEncodedSamples(void *buf, int size)
{
  sendData(buf, size);
} /* AudioDecoderDv3kUdp::writeEncodedSamples */



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

void AudioDecoderDv3kUdp::connect(void)
{
  if (ip_addr.isEmpty())
  {
    dns = new DnsLookup(host);
    dns->resultsReady.connect(mem_fun(*this,
                &AudioDecoderDv3kUdp::dnsResultsReady));
    return;
  }

  cout << name() << ": Decoder try to connect AMBEServer " << ip_addr.toString()
       << ":" << port << endl;
  m_udp_sock = new Async::UdpHandler(port, ip_addr);
  m_udp_sock->dataReceived.connect(
      mem_fun(*this, &AudioDecoderDv3kUdp::onDataReceived));
  m_udp_sock->open();

} /* AudioDecoderDv3kUdp::connect */


void AudioDecoderDv3kUdp::dnsResultsReady(DnsLookup& dns_lookup)
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
} /* RewindLogic::dnsResultsReady */


void AudioDecoderDv3kUdp::onDataReceived(const IpAddress& addr, uint16_t port,
                                         void *buf, int count)
{
  cout << "### " << name() << ": AudioDecoderDv3kUdpLogic::onDataReceived: "
       << "addr=" << addr << " port=" << port << " count=" << count << endl;

  float *samples = reinterpret_cast<float*>(buf);

  sinkWriteSamples(samples, count);

} /* AudioDecoderDv3kUdp::onDataReceived */


void AudioDecoderDv3kUdp::sendData(void *data, int size)
{
  if (!dns)
  {
    connect();
    return;
  }

  m_udp_sock->write(ip_addr, port, data, size);
} /* AudioDecoderDv3kUdp::sendData */



/*
 * This file has not been truncated
 */

