/**
@file	 DtmfDecoder.cpp
@brief   This file contains a class that implements a DTMF decoder
@author  Tobias Blomberg / SM0SVX
@date	 2003-04-16

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2004-2005  Tobias Blomberg / SM0SVX

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

#include <cstdio>
#include <cstring>

#include <algorithm>
#include <cassert>
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

#include "ToneDetector.h"
#include "DtmfDecoder.h"



/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/

using namespace std;
using namespace SigC;



/****************************************************************************
 *
 * Defines & typedefs
 *
 ****************************************************************************/

#define BASETONE_N    	      330
#define OVERTONE_N    	      201
#define POWER_THRESHOLD       500000
#define POWER_DIFF_THRESHOLD  9
#define MIN_TWIST     	      -8
#define MAX_TWIST     	      4


/****************************************************************************
 *
 * Local class definitions
 *
 ****************************************************************************/

class DtmfToneDetector : public sigc::trackable
{
  public:
    DtmfToneDetector(bool is_row, int pos, int fq)
      : is_row(is_row), pos(pos),
      	basetone(fq, BASETONE_N),
	//overtone(2*fq, OVERTONE_N),
	is_active(false),
	tot_samples_processed(0)
    {
      //basetone.activated.connect(
      //	  slot(this, &DtmfToneDetector::onToneActivated));
      //overtone.activated.connect(
      //	  slot(this, &DtmfToneDetector::onOvertoneActivated));
    }
    
    float value(void)
    {
      return basetone.value();
    }
    
    /*
    float overtoneValue(void)
    {
      return overtone.value();
    }
    */
    
    int processSamples(float *samples, int count)
    {
      assert(count == max(BASETONE_N, OVERTONE_N));
      
      basetone.reset();
      //overtone.reset();
      
      basetone.writeSamples(samples, BASETONE_N);
      //overtone.processSamples(samples, OVERTONE_N);

      return count;
    }
    
    sigc::signal<void, bool, int, bool>  activated;
    
  private:
    bool      	  is_row;
    int       	  pos;
    ToneDetector  basetone;
    //ToneDetector  overtone;
    bool      	  is_active;
    int       	  tot_samples_processed;
    
}; /* class DtmfToneDetector */



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


/*
 *------------------------------------------------------------------------
 * Method:    
 * Purpose:   
 * Input:     
 * Output:    
 * Author:    
 * Created:   
 * Remarks:   
 * Bugs:      
 *------------------------------------------------------------------------
 */
DtmfDecoder::DtmfDecoder(void)
  : active_row(-1), active_col(-1), buffered_samples(0),
    last_detected_digit('?'), digit_activated(false)
{
  row[0] = new DtmfToneDetector(true, 0, 697);
  row[1] = new DtmfToneDetector(true, 1, 770);
  row[2] = new DtmfToneDetector(true, 2, 852);
  row[3] = new DtmfToneDetector(true, 3, 941);
  
  col[0] = new DtmfToneDetector(false, 0, 1209);
  col[1] = new DtmfToneDetector(false, 1, 1336);
  col[2] = new DtmfToneDetector(false, 2, 1477);
  col[3] = new DtmfToneDetector(false, 3, 1633);

} /* DtmfDecoder::DtmfDecoder */


DtmfDecoder::~DtmfDecoder(void)
{
  for (int i=0; i<4; i++)
  {
    delete row[i];
    delete col[i];
  }
} /* DtmfDecoder::~DtmfDecoder */


int DtmfDecoder::writeSamples(const float *buf, int len)
{
  //printf("len=%d\n", len);
  
  int block_len = max(BASETONE_N, OVERTONE_N);
  int orig_len = len;
  
  while (len > 0)
  {
    int samples_to_copy = min(block_len - buffered_samples, len);
    memcpy(sample_buf + buffered_samples, buf, sizeof(*buf) * samples_to_copy);
    buffered_samples += samples_to_copy;
    if (buffered_samples == block_len)
    {
      for (int n=0; n<4; n++)
      {
	row[n]->processSamples(sample_buf, block_len);
	col[n]->processSamples(sample_buf, block_len);
      }
      checkTones();
      buffered_samples = 0;
    }
    len -= samples_to_copy;
    buf += samples_to_copy;
  }
  
  return orig_len;
  
} /* DtmfDecoder::processSamples */



/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/


/*
 *------------------------------------------------------------------------
 * Method:    
 * Purpose:   
 * Input:     
 * Output:    
 * Author:    
 * Created:   
 * Remarks:   
 * Bugs:      
 *------------------------------------------------------------------------
 */






/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/


/*
 *----------------------------------------------------------------------------
 * Method:    
 * Purpose:   
 * Input:     
 * Output:    
 * Author:    
 * Created:   
 * Remarks:   
 * Bugs:      
 *----------------------------------------------------------------------------
 */
void DtmfDecoder::checkTones(void)
{
  float max_row_val = 1;
  float second_max_val = 1;
  
  int active_row = -1;
  for (int i=0; i<4; ++i)
  {
    if (row[i]->value() > max_row_val)
    {
      second_max_val = max_row_val;
      max_row_val = row[i]->value();
      active_row = i;
    }
    else if (row[i]->value() > second_max_val)
    {
      second_max_val = row[i]->value();
    }
  }
  
  double dB = 10 * log10(max_row_val / second_max_val);
  if ((max_row_val < POWER_THRESHOLD) || (dB < POWER_DIFF_THRESHOLD))
  {
    setDigitActive(false);
    last_detected_digit = '?';
    return;
  }
  //printf("row max=%.0f second_max=%.0f diff=%.1lfdB\n",
  //   	 max_row_val, second_max_val, dB);
  
  int active_col = -1;
  float max_col_val = 1;
  second_max_val = 1;
  for (int i=0; i<4; ++i)
  {
    if (col[i]->value() > max_col_val)
    {
      second_max_val = max_col_val;
      max_col_val = col[i]->value();
      active_col = i;
    }
    else if (col[i]->value() > second_max_val)
    {
      second_max_val = col[i]->value();
    }
  }
  
  dB = 10 * log10(max_col_val / second_max_val);
  if ((max_col_val < POWER_THRESHOLD) || (dB < POWER_DIFF_THRESHOLD))
  {
    setDigitActive(false);
    last_detected_digit = '?';
    return;
  }
  //printf("col max=%.0f second_max=%.0f diff=%.1lf\n",
  //    	 max_col_val, second_max_val, dB);
  
  dB = 10 * log10(max_col_val / max_row_val);
  //printf("Twist=%.1lf\n", dB);
  if ((dB < MIN_TWIST) || (dB > MAX_TWIST))
  {
    setDigitActive(false);
    last_detected_digit = '?';
    return;
  }
  
  //printf("Active row=%d col=%d\n", active_row, active_col);
  
  const char digit_map[4][4] =
  {
    { '1', '2', '3', 'A' },
    { '4', '5', '6', 'B' },
    { '7', '8', '9', 'C' },
    { '*', '0', '#', 'D' },
  };
  
  char digit = digit_map[active_row][active_col];
  
  //printf("DTMF digit detected: %c\n", digit);
  
  if (digit == last_detected_digit)
  {
    setDigitActive(true);
  }
  
  last_detected_digit = digit;
  
} /* DtmfDecoder::checkTones */


void DtmfDecoder::setDigitActive(bool is_active)
{
  /*
  printf("DtmfDecoder::setDigitActive: is_active=%s\n",
    is_active ? "TRUE" : "FALSE");
  */
  
  if (is_active)
  {
    if (!digit_activated)
    {
      gettimeofday(&det_time, NULL);
      digitActivated(last_detected_digit);
    }
  }
  else
  {
    if (digit_activated)
    {
      struct timeval tv, diff;
      gettimeofday(&tv, NULL);
      timersub(&tv, &det_time, &diff);
      int diff_ms = diff.tv_sec * 1000 + diff.tv_usec / 1000;
      digitDeactivated(last_detected_digit, diff_ms);
    }
  }
  
  digit_activated = is_active;
  
} /* DtmfDecoder::setDigitActive */




/*
 * This file has not been truncated
 */

