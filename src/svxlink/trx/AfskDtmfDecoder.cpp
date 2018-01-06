/**
@file	 AfskDtmfDecoder.cpp
@brief   This file contains a class that read DTMF digits from the data stream
@author  Tobias Blomberg / SM0SVX
@date	 2013-05-10

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2004-2008  Tobias Blomberg / SM0SVX

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
//#include <algorithm>
//#include <cstring>
//#include <cmath>
//#include <cstdlib>

//#include <stdint.h>



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

#include "AfskDtmfDecoder.h"
#include "Rx.h"
#include "Tx.h"



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

AfskDtmfDecoder::AfskDtmfDecoder(Rx *rx, Config &cfg, const string &name)
  : DtmfDecoder(cfg, name)
{
  rx->dataReceived.connect(mem_fun(*this, &AfskDtmfDecoder::dataReceived));
} /* AfskDtmfDecoder::AfskDtmfDecoder */


bool AfskDtmfDecoder::initialize(void)
{
  if (!DtmfDecoder::initialize())
  {
    return false;
  }

  return true;

} /* AfskDtmfDecoder::initialize */


int AfskDtmfDecoder::writeSamples(const float *buf, int len)
{
  return len;
} /* AfskDtmfDecoder::writeSamples */



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

void AfskDtmfDecoder::dataReceived(const vector<uint8_t> &frame)
{
  uint8_t cmd = frame[0];
  if (cmd != Tx::DATA_CMD_DTMF)
  {
    return;
  }
  if (frame.size() < 3)
  {
    cerr << "*** WARNING: Illegal DTMF frame received\n";
    return;
  }
  uint16_t duration = frame[1];
  duration |= static_cast<uint16_t>(frame[2]) << 8;

  cout << "### AfskDtmfDecoder::frameReceived:"
       << " cmd=" << (int)cmd
       << " duration=" << duration
       << " digits=";
  for (size_t i=3; i<frame.size(); ++i)
  {
    char digit = static_cast<char>(frame[i]);
    cout << digit;
  }
  cout << endl;

  for (size_t i=3; i<frame.size(); ++i)
  {
    uint8_t digit = frame[i];
    digitDeactivated(digit, duration);
  }
} /* AfskDtmfDecoder::dataReceived */



/*
 * This file has not been truncated
 */
