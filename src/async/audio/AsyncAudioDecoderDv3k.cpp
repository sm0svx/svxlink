/**
@file	 AudioDecoderDv3k.cpp
@brief   An audio decoder that use the Dv3k audio codec
@author  Tobias Blomberg / SM0SVX & Adi Bier / DL1HRC
@date	 2017-04-02

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
#include <string.h>
#include <cmath>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncSerial.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "AsyncAudioDecoderDv3k.h"


/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/

using namespace std;
using namespace Async;
using namespace sigc;



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

AudioDecoderDv3k::AudioDecoderDv3k(void) :
  device(""), baudrate(0), dv3kfd(0), outcnt(0)
{
} /* AudioDecoderDv3k::AudioDecoderDv3k */


AudioDecoderDv3k::~AudioDecoderDv3k(void)
{
  delete dv3kfd;
} /* AudioDecoderDv3k::~AudioDecoderDv3k */


void AudioDecoderDv3k::setOption(const std::string &name,
      	      	      	      	  const std::string &value)
{
    // DV3K_DEVICE and DV3K_BAUDRATE
  if (name == "DEVICE")
  {
    device = value;
  }
  if (name == "BAUDRATE")
  {
    baudrate = atoi(value.c_str());
  }

  if (!device.empty() && baudrate > 0)
  {
    openDv3k();
  }
} /* AudioDecoderDv3k::setOption */


void AudioDecoderDv3k::writeEncodedSamples(void *buf, int size)
{

  unsigned char *tbuf = (unsigned char *)buf;

  // decode ambe stream received from network to raw
  // audio stream by sending dmr to Dv3k
  if (outcnt + size < 160)
  {
    memcpy(outbuf+outcnt, tbuf, size);
    outcnt += size;
    return;
  }

  int diff = 160-outcnt;
  memcpy(outbuf+outcnt, tbuf, diff);
  sendToDv3k(outbuf, outcnt+diff);
  outcnt = size - diff;
  memcpy(outbuf, tbuf+diff, size-diff);
} /* AudioDecoderDv3k::writeEncodedSamples */



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

void AudioDecoderDv3k::openDv3k(void)
{

  dv3kfd = new Async::Serial(device);
  if (!dv3kfd->open())
  {
    cout << "*** ERROR: Can not open DV3K" << endl;
  }
  dv3kfd->setParams(baudrate, Serial::PARITY_NONE, 8, 1, Serial::FLOW_NONE);
  dv3kfd->charactersReceived.connect(
          mem_fun(*this, &AudioDecoderDv3k::charactersReceived));
  dv3kfd->write(DV3000_REQ_PRODID, DV3000_REQ_PRODID_LEN);
  m_state = PROD_ID;
} /* AudioDecoderDv3k::open3k */


void AudioDecoderDv3k::charactersReceived(char *buf, int len)
{

  if (buf[0] != 0x61 || len < 4)
  {
    cout << "*** ERROR: Decoder invalid response from DV3K" << endl;
    return;
  }

  int tlen = buf[2] + buf[1] * 256;

  if (tlen-4 != len)
  {
    cout << "*** ERROR: length in DV3K message != length received" << endl;
  }

  int type = buf[3];
  char *tbuf = buf + 4;

  switch (type) {
    case DV3000_TYPE_CONTROL:
      cout << "CONTROL received\n";
      handleControl(tbuf, len-4);
      return;

    case DV3000_TYPE_AMBE:
      cout << "AMBE received\n";
      return;

    case DV3000_TYPE_AUDIO:
      cout << "AUDIO received\n";
      sendAudio(tbuf, len-4);
      return;

    default:
      cout << "*** ERROR: unknown DV3K type" << endl;
      return;
  }
} /* AudioDecoderDv3k::charactersReceived */


void AudioDecoderDv3k::handleControl(char *buf, int len)
{
  if (m_state == PROD_ID)
  {
//    cout << "--- Response from DV3k: " << buf << endl;
    dv3kfd->write(DV3000_REQ_RATEP, DV3000_REQ_RATEP_LEN);
    m_state = RATEP;
    return;
  }
} /* AudioDecoderDv3k::handleControl */


void AudioDecoderDv3k::sendToDv3k(unsigned char *buf, int len)
{
  char out[175];

  for (int a=0; a<len; a++)
  {
    out[a] = buf[a] - 128;
  }

  memcpy(out, DV3000_AMBE_HEADER, DV3000_AMBE_HEADER_LEN);
  memcpy(out + DV3000_AMBE_HEADER_LEN, buf, len);
//  cout << "sending to DV3K:" << len << " BUF:" << out << endl;
  dv3kfd->write(out, len + DV3000_AMBE_HEADER_LEN);
} /* AudioDecoderDv3k::sendToDv3k */


void AudioDecoderDv3k::sendAudio(char *buf, int len)
{
  char *audio = reinterpret_cast<char *>(buf);
  int count = len / sizeof(char);
  float samples[count];

  for (int i=0; i<count; i +=2)
  {
    samples[i] = static_cast<float>((audio[i]<<8) | (audio[i+1]<<0)) / 32768.0;
  }
  sinkWriteSamples(samples, count);
} /* AudioDecoderDv3k::sendAudio */


/*
 * This file has not been truncated
 */

