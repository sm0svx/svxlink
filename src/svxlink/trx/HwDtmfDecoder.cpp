/**
@file	 HwDtmfDecoder.cpp
@brief   This file contains a the base class for implementing a hw DTMF decoder
@author  Tobias Blomberg / SM0SVX
@date	 2008-02-04

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


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncTimer.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "HwDtmfDecoder.h"



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



/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/

HwDtmfDecoder::HwDtmfDecoder(Config &cfg, const string &name)
  : DtmfDecoder(cfg, name), last_detected_digit('?'), state(STATE_IDLE),
    det_timestamp(), hang_timer(0), timeout_timer(0)
{
} /* HwDtmfDecoder::HwDtmfDecoder */


HwDtmfDecoder::~HwDtmfDecoder(void)
{
  delete hang_timer;
  delete timeout_timer;
} /* HwDtmfDecoder::~HwDtmfDecoder */


bool HwDtmfDecoder::initialize(void)
{
  if (!DtmfDecoder::initialize())
  {
    return false;
  }
  
  return true;
  
} /* HwDtmfDecoder::initialize */



/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/

void HwDtmfDecoder::digitActive(char digit)
{
  if (state == STATE_ACTIVE)
  {
    return;
  }
  
  if (state == STATE_HANG)
  {
    delete hang_timer;
    hang_timer = 0;
    
    if (digit == last_detected_digit)
    {
      state = STATE_ACTIVE;
      return;
    }
    else
    {
      setIdle();
    }
  }

  state = STATE_ACTIVE;
  last_detected_digit = digit;
  gettimeofday(&det_timestamp, NULL);
  digitActivated(digit);
  timeout_timer = new Timer(MAX_ACTIVE_TIME * 1000);
  timeout_timer->expired.connect(mem_fun(*this, &HwDtmfDecoder::timeout));
} /* HwDtmfDecoder::digitActive */


void HwDtmfDecoder::digitIdle(void)
{
  if (state != STATE_ACTIVE)
  {
    return;
  }
  
  if (hangtime() > 0)
  {
    hang_timer = new Timer(hangtime());
    hang_timer->expired.connect(mem_fun(*this, &HwDtmfDecoder::hangtimeExpired));
    state = STATE_HANG;
  }
  else
  {
    setIdle();
  }
} /* HwDtmfDecoder::digitIdle */



/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/

void HwDtmfDecoder::hangtimeExpired(Timer *t)
{
  setIdle();
} /* HwDtmfDecoder::hangtimeExpired */


void HwDtmfDecoder::timeout(Timer *t)
{
  cerr << "*** WARNING: No DTMF idle indication received within "
       << MAX_ACTIVE_TIME << " seconds after activation indication "
       << "for receiver " << name() << ".\n";
  setIdle();  
} /* HwDtmfDecoder::timeout */


void HwDtmfDecoder::setIdle(void)
{
  delete hang_timer;
  hang_timer = 0;
  
  delete timeout_timer;
  timeout_timer = 0;
  
  struct timeval diff, now;
  gettimeofday(&now, NULL);
  timersub(&now, &det_timestamp, &diff);
  digitDeactivated(last_detected_digit,
      diff.tv_sec * 1000 + diff.tv_usec / 1000);
  state = STATE_IDLE;
} /* HwDtmfDecoder::setIdle */



/*
 * This file has not been truncated
 */

