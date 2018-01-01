/**
@file	 HdlcFramer.cpp
@brief   Create HDLC frames from data bytes
@author  Tobias Blomberg / SM0SVX
@date	 2013-05-09

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2018 Tobias Blomberg / SM0SVX

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

#include "HdlcFramer.h"
#include "Fcs.h"


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

HdlcFramer::HdlcFramer(void)
  : ones(0), prev_was_mark(false), start_flag_cnt(DEFAULT_START_FLAG_CNT)
{
} /* HdlcFramer::HdlcFramer */


HdlcFramer::~HdlcFramer(void)
{
} /* HdlcFramer::~HdlcFramer */


void HdlcFramer::sendBytes(const vector<uint8_t> &frame)
{
  vector<bool> bitbuf;

    // Store frame start flags
  for (size_t i=0; i<start_flag_cnt; ++i)
  {
    bitbuf.push_back(!prev_was_mark);
    bitbuf.push_back(!prev_was_mark);
    bitbuf.push_back(!prev_was_mark);
    bitbuf.push_back(!prev_was_mark);
    bitbuf.push_back(!prev_was_mark);
    bitbuf.push_back(!prev_was_mark);
    bitbuf.push_back(!prev_was_mark);
    bitbuf.push_back(prev_was_mark);
  }

    // Store frame data
  for (size_t i=0; i<frame.size(); ++i)
  {
    encodeByte(bitbuf, frame[i]);
  }

    // Calculate and store CRC (FCS)
  uint16_t crc = fcsCalc(frame);
  encodeByte(bitbuf, crc & 0xff);
  encodeByte(bitbuf, crc >> 8);

    // Store frame end flag
  bitbuf.push_back(!prev_was_mark);
  bitbuf.push_back(!prev_was_mark);
  bitbuf.push_back(!prev_was_mark);
  bitbuf.push_back(!prev_was_mark);
  bitbuf.push_back(!prev_was_mark);
  bitbuf.push_back(!prev_was_mark);
  bitbuf.push_back(!prev_was_mark);
  bitbuf.push_back(prev_was_mark);

  sendBits(bitbuf);
} /* HdlcFramer::sendBytes */


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

void HdlcFramer::encodeByte(vector<bool> &bitbuf, uint8_t data)
{
  for (size_t bit=0; bit<8; ++bit)
  {
    bool is_one = (data & 0x01);
    data >>= 1;

    bool is_mark = is_one ? prev_was_mark : !prev_was_mark;
    bitbuf.push_back(is_mark);

    if (is_one)
    {
      if (++ones == 5)
      {
        bitbuf.push_back(!prev_was_mark);
        is_mark = !prev_was_mark;
        ones = 0;
      }
    }
    else
    {
      ones = 0;
    }

    prev_was_mark = is_mark;
  }
} /* HdlcFramer::encodeByte */


/*
 * This file has not been truncated
 */
