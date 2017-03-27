/**
@file	 AudioRecoderDV3k.cpp
@brief   An audio en/decoder for the DV3000 stick
@author  Tobias Blomberg / SM0SVX & Adi Bier / DL1HRC
@date	 2017-03-11

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2003-2008 Tobias Blomberg / SM0SVX

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
#include <netinet/in.h>


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

#include "AsyncAudioRecoderDV3k.h"



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

#define dv3k_packet_size(a) (1 + sizeof((a).header) + ntohs((a).header.payload_length))


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

AudioRecoderDV3k::AudioRecoderDV3k(void)
{

} /* AudioRecoderDV3k::AudioRecoderDV3k */


AudioRecoderDV3k::~AudioRecoderDV3k(void)
{
  dv3kdev->close();
  delete dv3kdev;
} /* AudioRecoderDV3k::~AudioRecoderDV3k */


void AudioRecoderDV3k::setOption(const std::string &name,
      	      	      	      	  const std::string &value)
{
  if (name == "BAUDRATE")
  {
    baud = atoi(value.c_str());
  }
  else if (name == "PORT")
  {
    port = value;
  }
  else
  {
    cerr << "*** WARNING AudioRecoderDV3k: Unknown option \""
      	 << name << "\". Ignoring it.\n";
  }
} /* AudioRecoderDV3k::setOption */


bool AudioRecoderDV3k::createDV3k(void)
{
  dv3kdev = new Async::Serial(port);
  dv3kdev->charactersReceived.connect(
    	  mem_fun(*this, &AudioRecoderDV3k::onCharactersReceived));

  if (!dv3kdev->open(true))
  {
    cerr << "*** ERROR: opening port " << port << endl;
    return false;
  }

  dv3kdev->setParams(baud, Serial::PARITY_NONE, 8, 1, Serial::FLOW_NONE);

  bool ret = true;
  ret = initDV3k();

  return ret;
} /* */


void AudioRecoderDV3k::writeDecodedSamples(void *buf, int size)
{

} /* AudioRecoderDV3k::writeEncodedSamples */



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


bool AudioRecoderDV3k::initDV3k(void)
{



  //if (dv3kdev->write(&ctrlPacket, dv3k_packet_size(ctrlPacket)) == -1)
  {
     cerr << "*** ERROR: initializing DV3k Stick" << endl;

  }

  return true;
} /* AudioRecoderDV3k::initDV3k */


void AudioRecoderDV3k::onCharactersReceived(char *buf, int count)
{

  assert(buf != NULL);

  if (count < 1)
  {
    return;
  }

  unsigned char c1 = buf[0];
  unsigned char c2;

  switch (c1)
  {
    case  0xA0:
      cout << "--- resynchronizing DV-Dongle" << endl;
      buf[0] = 0x32;
      buf[1] = c1;
      return processResponse(buf, count);

    case 0x81:
      cout << "--- resynchronizing DV-Dongle" << endl;
      buf[0] = 0x42;
      buf[1] = c1;
      return processResponse(buf, count);

    default:
      c2 = buf[1];
      buf[0] = c1;
      buf[1] = c2;
      return processResponse(buf, count);
  }

} /* AudioRecoderDV3k::onCharactersReceived */


void AudioRecoderDV3k::processResponse(char *buf, int count)
{

} /* AudioRecoderDV3k::processResponse */


/*
 * This file has not been truncated
 */

