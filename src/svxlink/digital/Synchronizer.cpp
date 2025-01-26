/**
@file	 Synchronizer.cpp
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

#include <iostream>


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

#include "Synchronizer.h"


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

Synchronizer::Synchronizer(unsigned baudrate, unsigned sample_rate)
  : baudrate(baudrate), sample_rate(sample_rate),
    shift_pos(sample_rate / 2), pos(0), was_mark(false),
    last_stored_was_mark(false)
{
} /* Synchronizer::Synchronizer */


Synchronizer::~Synchronizer(void)
{
} /* Synchronizer::~Synchronizer */


int Synchronizer::writeSamples(const float *samples, int len)
{
  //cout << pos << endl;
  for (int i=0; i<len; ++i)
  {
    pos += baudrate;

      // Find out if it's a mark or space
    //bool is_mark = was_mark ? (samples[i] > -0.005) : (samples[i] > 0.005);
    bool is_mark = (samples[i] > 0);

      // If it's a transition, adjust bit clock
    if (is_mark != was_mark)
    {
      err = shift_pos - pos;
      pos += err / 5;
      /*
      float err_percent = 100.0 * static_cast<float>(err) / sample_rate;
      cout << "  " << err << " " << err_percent << "%" << endl;
      */
    }
    was_mark = is_mark;

      // Extract bit if pos >= sample_rate
    if (pos >= sample_rate)
    {
      bitbuf.push_back(is_mark == last_stored_was_mark);
      last_stored_was_mark = is_mark;
      if (bitbuf.size() >= 8)
      {
        /*
        for (size_t i=0; i<bitbuf.size(); ++i)
        {
          cout << (bitbuf[i] ? '1' : '0');
        }
        float err_percent = 100.0 * static_cast<float>(err) / sample_rate;
        cout << "  " << err << " " << err_percent << "%" << endl;
        */
        bitsReceived(bitbuf);
        bitbuf.clear();
      }
      pos -= sample_rate;
    }
  }

  return len;
} /* Synchronizer::writeSamples */


void Synchronizer::flushSamples(void)
{
  sourceAllSamplesFlushed();
} /* Synchronizer::flushSamples */



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
