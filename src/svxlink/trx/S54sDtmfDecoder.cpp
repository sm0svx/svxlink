/**
@file	 S54sDtmfDecoder.cpp
@brief   This file contains a class that add support for the S54S interface
@author  Tobias Blomberg / SM0SVX
@date	 2008-02-04

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2004-2014  Tobias Blomberg / SM0SVX

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

#include "S54sDtmfDecoder.h"



/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/

using namespace std;
using namespace sigc;
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

static const char digit_map[16] =
{
  'D', '1', '2', '3', '4', '5', '6', '7',
  '8', '9', '0', '*', '#', 'A', 'B', 'C'
};


/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/

S54sDtmfDecoder::S54sDtmfDecoder(Config &cfg, const string &name)
  : HwDtmfDecoder(cfg, name), serial(0)
{
  //cout << "### S54S DTMF decoder loaded...\n";
  
} /* S54sDtmfDecoder::S54sDtmfDecoder */


S54sDtmfDecoder::~S54sDtmfDecoder(void)
{
  delete serial;
} /* S54sDtmfDecoder::~S54sDtmfDecoder */


bool S54sDtmfDecoder::initialize(void)
{
  if (!HwDtmfDecoder::initialize())
  {
    return false;
  }
  
  string serial_dev;
  if (!cfg().getValue(name(), "DTMF_SERIAL", serial_dev))
  {
    cerr << "*** ERROR: Config variable " << name()
      	 << "/DTMF_SERIAL not specified\n";
    return false;
  }
  
  serial = new Serial(serial_dev);
  if (!serial->open(true))
  {
    cerr << "*** ERROR: Could not open the serial port " << serial_dev
         << " specified in " << name() << "/DTMF_SERIAL\n";
    return false;
  }
  if (!serial->setParams(9600, Serial::PARITY_NONE, 8, 1, Serial::FLOW_NONE))
  {
    cerr << "*** ERROR: Could not setup serial port parameters for "
      	 << serial_dev << " specified in " << name() << "/DTMF_DERIAL\n";
    serial->close();
    return false;
  }
  serial->charactersReceived.connect(
      mem_fun(*this, &S54sDtmfDecoder::charactersReceived));
  
  return true;
  
} /* S54sDtmfDecoder::initialize */




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

void S54sDtmfDecoder::charactersReceived(char *buf, int len)
{
  for (int i=0; i<len; ++i)
  {
    int func = (buf[i] >> 4) & 0x07;
    int data = buf[i] & 0x0f;
    printf("event=%02x (func=%d data=%d)\n", (unsigned int)buf[i], func, data);
    switch (func)
    {
      case 0:	// DTMF digit deactivated
        digitIdle();
        break;

      case 1:	// DTMF digit activated
        digitActive(digit_map[data]);
        break;

      case 3:	// Maintenence functions
        if (data == 0xf)	// '?' received == identify
        {
          serial->write("s", 1);
        }
        break;
    }
  }
} /* S54sDtmfDecoder::charactersReceived */



/*
 * This file has not been truncated
 */

