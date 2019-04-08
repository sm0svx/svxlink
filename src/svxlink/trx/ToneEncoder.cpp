/**
@file	 ToneEncoder.cpp
@brief   A tone encoder that can be used SvxLink wide
@author  Tobias Blomberg / SM0SVX & Adi Bier / DL1HRC
@date	 2019-04-08

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2004-2019  Tobias Blomberg / SM0SVX

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

#include <cmath>


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

#include "ToneEncoder.h"



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

ToneEncoder::ToneEncoder(float tone_hz, int level)
  : m_tone(tone_hz), m_level(level), is_enabled(false),
    pos(0)
{
} /* ToneEncoder::ToneEncoder */


ToneEncoder::~ToneEncoder(void)
{
  setEnable(false);
} /* ToneEncoder::~ToneEncoder */


void ToneEncoder::setEnable(bool enable)
{
  is_enabled = enable;
  if (is_enabled && m_tone > 0)
  {
    pos = 0;
    writeSamples();
  }
} /* ToneEncoder::setEnable */


void ToneEncoder::setLevel(int level)
{
  m_level = level / 100.0;
} /* ToneEncoder::setLevel */


void ToneEncoder::setTone(float tone_hz)
{
  m_tone = tone_hz;
} /* ToneEncoder::setTone */


void ToneEncoder::resumeOutput(void)
{
  writeSamples();
} /* ToneEncoder::resumeOutput */


void ToneEncoder::allSamplesFlushed(void)
{
} /* ToneEncoder::allSamplesFlushed */



/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/

void ToneEncoder::writeSamples(void)
{
  static const int BLOCK_SIZE = 128;
  int written;
  do {
	float buf[BLOCK_SIZE];
	for (int i=0; i<BLOCK_SIZE; ++i)
	{
      buf[i] = m_level * sin(2 * M_PI * m_tone * (pos+i) / INTERNAL_SAMPLE_RATE);
	}
	written = sinkWriteSamples(buf, BLOCK_SIZE);
    pos += written;
  } while (written != 0 && is_enabled);
}
    
/*
 * This file has not been truncated
 */
