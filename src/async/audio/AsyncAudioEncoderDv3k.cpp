/**
@file	 AsyncAudioEncoderDv3k.cpp
@brief   An audio encoder that encodes samples using the Dv3k codec
@author  Tobias Blomberg / SM0SVX
@date	 2017-04-03

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

#include "AsyncAudioEncoderDv3k.h"



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

AudioEncoderDv3k::AudioEncoderDv3k(void)
  : device("/dev/ttyUSB0"), baudrate(460800), dv3kfd(0)
{
} /* AsyncAudioEncoderDv3k::AsyncAudioEncoderDv3k */


AudioEncoderDv3k::~AudioEncoderDv3k(void)
{
  delete dv3kfd;
} /* AsyncAudioEncoderDv3k::~AsyncAudioEncoderDv3k */


void AudioEncoderDv3k::setOption(const std::string &name,
      	      	    	      	 const std::string &value)
{
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
} /* AudioEncoderDv3k::setOption */


int AudioEncoderDv3k::writeSamples(const float *samples, int count)
{
  return count;
} /* AudioEncoderDv3k::writeSamples */



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

void AudioEncoderDv3k::openDv3k(void)
{
  dv3kfd = new Async::Serial(device);
  if (!dv3kfd->open())
  {
    cout << "*** ERROR: Can not open DV3K" << endl;
  }
  dv3kfd->setParams(baudrate, Serial::PARITY_NONE, 8, 1, Serial::FLOW_NONE);
  dv3kfd->charactersReceived.connect(
          mem_fun(*this, &AudioEncoderDv3k::charactersReceived));
  dv3kfd->write(DV3000_REQ_PRODID, DV3000_REQ_PRODID_LEN);
  m_state = PROD_ID;
} /* AudioEncoderDv3k::connect */


void AudioEncoderDv3k::charactersReceived(char *buf, int len)
{
  if (buf[0] != 0x61 || len < 4)
  {
    cout << "*** ERROR: Encoder invalid response from DV3K" << endl;
    return;
  }

  int tlen = buf[2] + buf[1] * 256;

  if (tlen-1 != len)
  {
    cout << "*** ERROR: dv3k-length " << tlen << ", length " << len << endl;
  //  return;
  }

  int type = buf[3];
  char *tbuf = buf + 4;

  switch (type) {
    case DV3000_TYPE_CONTROL:
      cout << "TYP=CONTROL" << endl;
      handleControl(tbuf, tlen-4);
      return;

    case DV3000_TYPE_AMBE:
      cout << "TYP=AMBE" << endl;
      handleAmbe(tbuf, tlen-4);
      return;

    case DV3000_TYPE_AUDIO:
      cout << "TYP=AUDIO" << endl;
      handleAudio(tbuf, tlen-4);
      return;

    default:
      cout << "*** ERROR: unknown DV3K type" << endl;
      return;
  }
} /* AudioEncoderDv3k::onDataReceived */

void AudioEncoderDv3k::handleControl(char *buf, int len)
{
  if (m_state == PROD_ID)
  {
    cout << "--- Response from DV3k: " << buf << endl;
  }
} /* AudioEncoderDv3k::handleControl */


void AudioEncoderDv3k::handleAmbe(char *buf, int len)
{

} /* AudioEncoderDv3k::handleAmbe */


void AudioEncoderDv3k::handleAudio(char *buf, int len)
{
  char *audio = reinterpret_cast<char *>(buf);
  int count = len / sizeof(char);
  float samples[count];

  for (int i=0; i<count; i +=2)
  {
    samples[i] = static_cast<float>((audio[i]<<8) | (audio[i+1]<<0)) / 32768.0;
  }
  writeSamples(samples, count);
} /* AudioEncoderDv3k::handleControl */

/*
 * This file has not been truncated
 */

