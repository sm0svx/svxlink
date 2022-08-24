/**
@file	 DtmfEncoder.cpp
@brief   This file contains a class that implements a DTMF encoder.
@author  Tobias Blomberg / SM0SVX
@date	 2006-07-09

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2015 Tobias Blomberg / SM0SVX

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

#include <map>
#include <utility>
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

#include "DtmfEncoder.h"



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

#define BLOCK_SIZE  512U



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

static map<char, pair<int, int> > tone_map;


/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/

DtmfEncoder::DtmfEncoder(int sampling_rate)
  : sampling_rate(sampling_rate), tone_length(100 * sampling_rate / 1000),
    tone_spacing(50 * sampling_rate / 1000), tone_amp(0.5), low_tone(0),
    high_tone(0), pos(0), length(0), is_playing(false),
    is_sending_digits(false)
{
  if (tone_map.empty())
  {
    tone_map['1'] = make_pair(697, 1209);
    tone_map['2'] = make_pair(697, 1336);
    tone_map['3'] = make_pair(697, 1477);
    tone_map['A'] = make_pair(697, 1633);
    tone_map['4'] = make_pair(770, 1209);
    tone_map['5'] = make_pair(770, 1336);
    tone_map['6'] = make_pair(770, 1477);
    tone_map['B'] = make_pair(770, 1633);
    tone_map['7'] = make_pair(852, 1209);
    tone_map['8'] = make_pair(852, 1336);
    tone_map['9'] = make_pair(852, 1477);
    tone_map['C'] = make_pair(852, 1633);
    tone_map['*'] = make_pair(941, 1209);
    tone_map['0'] = make_pair(941, 1336);
    tone_map['#'] = make_pair(941, 1477);
    tone_map['D'] = make_pair(941, 1633);
  }
  
} /* DtmfEncoder::DtmfEncoder */


DtmfEncoder::~DtmfEncoder(void)
{
  
} /* DtmfEncoder::~DtmfEncoder */


void DtmfEncoder::setDigitDuration(int duration_ms)
{
  tone_length = duration_ms * sampling_rate / 1000;
} /* DtmfEncoder::setDigitDuration */


void DtmfEncoder::setDigitSpacing(int spacing_ms)
{
  tone_spacing = spacing_ms * sampling_rate / 1000;
} /* DtmfEncoder::setDigitSpacing */


void DtmfEncoder::setDigitPower(int power_db)
{
    // Subtract 3dB to keep the power at the reference level
    // (0dB = 0.5W over 1ohm)
  power_db -= 3;

    // Convert from dB to tone amplitude
  tone_amp = powf(10.0f, power_db / 20.0f);
} /* DtmfEncoder::setDigitPower */


int DtmfEncoder::digitPower(void) const
{
  return static_cast<int>(20.0f * log10f(tone_amp) + 3.0f);
} /* DtmfEncoder::digitPower */


void DtmfEncoder::send(const std::string &str, unsigned duration)
{
  is_sending_digits = true;
  //current_str += str;
  for (size_t i=0; i<str.size(); ++i)
  {
    send_queue.push_back(SendQueueItem(
                           str[i],
                           duration * sampling_rate / 1000
                         ));
  }
  playNextDigit();
} /* DtmfEncoder::send */


void DtmfEncoder::resumeOutput(void)
{
  if (isSending())
  {
    writeAudio();
  }
} /* DtmfEncoder::resumeOutput */


void DtmfEncoder::allSamplesFlushed(void)
{
  //printf("All digits sent!\n");
  is_sending_digits = false;
  allDigitsSent();
} /* DtmfEncoder::allSamplesFlushed */



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
void DtmfEncoder::playNextDigit(void)
{
  if (is_playing)
  {
    return;
  }
  
  if (low_tone > 0)
  {
    low_tone = 0;
    pos = 0;
    length = tone_spacing;
    is_playing = true;
    writeAudio();
    return;
  }
  
  if (send_queue.empty())
  {
    sinkFlushSamples();
    return;
  }
  
  //char digit = current_str[0];
  //current_str = current_str.substr(1);
  char digit = send_queue.front().digit;
  length = send_queue.front().duration;
  send_queue.pop_front();
  if (tone_map.count(digit) == 0)
  {
    playNextDigit();
    return;
  }
  
  //printf("Playing digit %c...\n", digit);
  
  low_tone = tone_map[digit].first;
  high_tone = tone_map[digit].second;
  pos = 0;
  if (length <= 0)
  {
    length = tone_length;
  }
  is_playing = true;
  
  writeAudio();
  
} /* DtmfEncoder::playNextDigit */


void DtmfEncoder::writeAudio(void)
{
  float block[BLOCK_SIZE];
  int ret;
  
  do
  {
    unsigned count = min(BLOCK_SIZE, length - pos);
    for (unsigned i=0; i<count; ++i)
    {
      if (low_tone > 0)
      {
      	block[i] = tone_amp * sin(2 * M_PI * low_tone * pos / sampling_rate) +
      		   tone_amp * sin(2 * M_PI * high_tone * pos / sampling_rate);
      }
      else
      {
      	block[i] = 0;
      }
      ++pos;
    }

    ret = sinkWriteSamples(block, count);
    pos -= (count - ret);
  } while ((ret > 0) && (pos < length));
  
  if (pos == length)
  {
    is_playing = false;
    playNextDigit();
  }
} /* DtmfEncoder::writeAudio */



/*
 * This file has not been truncated
 */
