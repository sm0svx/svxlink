/**
@file	 SpanDtmfDecoder.cpp
@brief   This file contains a class that implements a sw DTMF decoder
@author  Tobias Blomberg / SM0SVX
@date	 2003-04-16

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
#include <algorithm>

#include <math.h>
#include <stdlib.h>
#include <stdint.h>
#include <spandsp.h>


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

#include "SpanDtmfDecoder.h"



/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/

using namespace std;
using namespace SigC;
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

class SpanDtmfDecoder::PrivateData
{
  public:
    dtmf_rx_state_t rx_state;
    
};


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

SpanDtmfDecoder::SpanDtmfDecoder(Config &cfg, const string &name)
  : DtmfDecoder(cfg, name), last_detected_digit('?'), active_sample_cnt(0),
    hang_sample_cnt(0), state(STATE_IDLE)
{
} /* SpanDtmfDecoder::SpanDtmfDecoder */


SpanDtmfDecoder::~SpanDtmfDecoder(void)
{
  delete p;
} /* SpanDtmfDecoder::~SpanDtmfDecoder */


bool SpanDtmfDecoder::initialize(void)
{
  if (!DtmfDecoder::initialize())
  {
    return false;
  }
  
  string value;

  int max_fwd_twist = 8;
  if  (cfg().getValue(name(), "DTMF_MAX_FWD_TWIST", value))
  {
    max_fwd_twist = atoi(value.c_str());
  }
  
  int max_rev_twist = 8;
  if  (cfg().getValue(name(), "DTMF_MAX_REV_TWIST", value))
  {
    max_rev_twist = atoi(value.c_str());
  }
  
  p = new PrivateData;
  if (p == 0)
  {
    return false;
  }
  
  dtmf_rx_init(&p->rx_state, NULL, this);
  dtmf_rx_parms(&p->rx_state, FALSE, max_fwd_twist, max_rev_twist);
  dtmf_rx_set_realtime_callback(&p->rx_state, SpanDtmfDecoder::toneReportCb,
      	      	      	        this);
  
  return true;
  
} /* SpanDtmfDecoder::initialize */


int SpanDtmfDecoder::writeSamples(const float *buf, int len)
{
  //printf("len=%d\n", len);
  
  int samples_left = len;
  while (samples_left > 0)
  {
    int samples_to_write = min(samples_left, 200);
    
    int16_t amp[200];
    for (int i=0; i<samples_to_write; i++)
    {
      amp[i] = (int16_t)(32767.0 * buf[i]);
    }
    
    int ret = dtmf_rx(&p->rx_state, amp, samples_to_write);
    int samples_written = samples_to_write - ret;
    
    if (state == STATE_ACTIVE)
    {
      active_sample_cnt += samples_written;
    }
    
    if (state == STATE_HANG)
    {
      hang_sample_cnt += samples_written;
      if (hang_sample_cnt > 8 * hangtime())
      {
      	state = STATE_IDLE;
      	digitDeactivated(last_detected_digit, 1000 * active_sample_cnt / 8000);
      }
    }
    
    samples_left -= samples_written;
    
    if (ret != 0)
    {
      break;
    }
    
    buf += samples_written;
  }
  
  return len - samples_left;
  
} /* SpanDtmfDecoder::processSamples */



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
 *----------------------------------------------------------------------------
 * Method:    SpanDtmfDecoder::toneReportCb
 * Purpose:   Static callback method called when a DTMF digit is activated
 *    	      or deactivated.
 * Input:     user_data - Contains the SpanDtmfDecoder object (void *)
 *    	      code    	- The detected DTMF code (int)
 *    	      level   	- The level of the detected DTMF code (int)
 * Output:    None
 * Author:    Tobias Blomberg, SM0SVX
 * Created:   2007-04-22
 * Remarks:   
 * Bugs:      
 *----------------------------------------------------------------------------
 */
void SpanDtmfDecoder::toneReportCb(SPANDSP_TONE_REPORT_FUNC_ARGS)
{
  SpanDtmfDecoder *dtmf_dec = reinterpret_cast<SpanDtmfDecoder*>(user_data);
  assert(dtmf_dec != 0);
  dtmf_dec->toneReport(code);
} /* SpanDtmfDecoder::toneReport */


/*
 *----------------------------------------------------------------------------
 * Method:    SpanDtmfDecoder::toneReport
 * Purpose:   Callback method called by the static callback method when a
 *    	      DTMF digit is activated or deactivated. It's this function that
 *    	      do the real work.
 * Input:     code    	- The detected DTMF code
 * Output:    None
 * Author:    Tobias Blomberg, SM0SVX
 * Created:   2007-04-22
 * Remarks:   
 * Bugs:      
 *----------------------------------------------------------------------------
 */
void SpanDtmfDecoder::toneReport(int code)
{
  if (code > 0)
  {
    if (state == STATE_HANG)
    {
      if (code == last_detected_digit)
      {
      	active_sample_cnt += hang_sample_cnt;
      	state = STATE_ACTIVE;
	return;
      }
      else
      {
      	digitDeactivated(last_detected_digit, 1000 * active_sample_cnt / 8000);
      }
    }
    
    active_sample_cnt = 0;
    state = STATE_ACTIVE;
    last_detected_digit = static_cast<char>(code);
    digitActivated(last_detected_digit);
  }
  else
  {
    assert(state == STATE_ACTIVE);
    
    if (hangtime() == 0)
    {
      digitDeactivated(last_detected_digit, 1000 * active_sample_cnt / 8000);
      state = STATE_IDLE;
    }
    else
    {
      hang_sample_cnt = 0;
      state = STATE_HANG;
    }
  }
} /* SpanDtmfDecoder::toneReport */



/*
 * This file has not been truncated
 */

