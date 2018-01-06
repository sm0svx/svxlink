/**
@file	 HdlcDeframer.cpp
@brief   Deframe an HDLC bitstream
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

#include <iostream>
#include <iomanip>


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

#include "HdlcDeframer.h"
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

HdlcDeframer::HdlcDeframer(void)
  : state(STATE_SYNCHRONIZING), next_byte(0), bit_cnt(0), ones(0)
{
} /* HdlcDeframer::HdlcDeframer */


HdlcDeframer::~HdlcDeframer(void)
{
} /* HdlcDeframer::~HdlcDeframer */


void HdlcDeframer::bitsReceived(vector<bool> &bits)
{
  for (size_t i=0; i<bits.size(); ++i)
  {
    bool flag_detected = false;

      // Undo bitstuffing. If we receive a zero and the previous five bits
      // have been ones, the zero should be thrown away.
    if (bits[i])
    {
      ones += 1;
    }
    else
    {
      if (ones == 5)
      {
        ones = 0;
        continue;
      }
      else if (ones == 6)
      {
        flag_detected = true;
      }
      ones = 0;
    }

    next_byte >>= 1;
    next_byte |= (bits[i] << 7);
    switch (state)
    {
      case STATE_SYNCHRONIZING:
        if (next_byte == 0x7e)
        {
          state = STATE_FRAME_START_WAIT;
          bit_cnt = 0;
        }
        break;

      case STATE_FRAME_START_WAIT:
        if (++bit_cnt >= 8)
        {
          if (next_byte != 0x7e)
          {
            state = STATE_RECEIVING;
            frame.clear();
            frame.push_back(next_byte);
          }
          //frame.push_back(next_byte);
          bit_cnt = 0;
        }
        else if (next_byte == 0x7e)
        {
          bit_cnt = 0;
          //frame.clear();
          //frame.push_back(next_byte);
        }
        break;

      case STATE_RECEIVING:
        if (++bit_cnt >= 8)
        {
          if (flag_detected)
          {
            state = STATE_FRAME_START_WAIT;
            /*
            for (size_t i=0; i<frame.size(); ++i)
            {
              if (isprint(frame[i]))
              {
                cout << setw(2) << setfill(' ') << (char)frame[i];
              }
              else
              {
                cout << hex << setw(2) << setfill('0')
                     << (int)frame[i] << " ";
              }
            }
            cout << endl << endl;
            */
            if ((frame.size() > 2) && fcsOk(frame))
            {
                // Remove CRC from frame
              frame.pop_back();
              frame.pop_back();
              frameReceived(frame);
            }
          }
          else
          {
            if(frame.size() < 330)
            {
              frame.push_back(next_byte);
              next_byte = 0;
            }
            else
            {
              state = STATE_SYNCHRONIZING;
            }
          }
          bit_cnt = 0;
        }
        else if (flag_detected)
        {
          state = STATE_FRAME_START_WAIT;
          bit_cnt = 0;
        }
        break;
    }
  }
} /* HdlcDeframer::bitsReceived */


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



/*
 * This file has not been truncated
 */
