/**
@file	 AudioProcessor.cpp
@brief   A_brief_description_for_this_file
@author  Tobias Blomberg / SM0SVX
@date	 2006-04-23

A_detailed_description_for_this_file

\verbatim
<A brief description of the program or library this file belongs to>
Copyright (C) 2003 Tobias Blomberg / SM0SVX

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

#include "AudioProcessor.h"



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

#define SAMPLE_RATE 8000


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

AudioProcessor::AudioProcessor(void)
  : buf_cnt(0), do_flush(false), buf_full(false)
{
  
} /* AudioProcessor::AudioProcessor */


AudioProcessor::~AudioProcessor(void)
{

} /* AudioProcessor::~AudioProcessor */


int AudioProcessor::writeSamples(const float *samples, int len)
{
  //cout << "AudioProcessor::writeSamples: len=" << len << endl;
  
  do_flush = false;
  buf_full = false;
  
  writeFromBuf();
  
  int buf_avail = sizeof(buf)/sizeof(*buf) - buf_cnt;
  if (buf_avail < len)
  {
    len = buf_avail;
    buf_full = true;
  }
  
  processSamples(buf + buf_cnt, samples, len);
  buf_cnt += len;
  
  writeFromBuf();
  
  return len;

} /* AudioProcessor::writeSamples */


void AudioProcessor::flushSamples(void)
{
  //cout << "AudioProcessor::flushSamples" << endl;
  
  do_flush = true;
  //buf_full = false;
  if (buf_cnt == 0)
  {
    sinkFlushSamples();
  }
} /* AudioProcessor::flushSamples */


void AudioProcessor::resumeOutput(void)
{
  //cout << "AudioProcessor::resumeOutput" << endl;
  writeFromBuf();
} /* AudioProcessor::resumeOutput */


void AudioProcessor::allSamplesFlushed(void)
{
  //cout << "AudioProcessor::allSamplesFlushed" << endl;
  do_flush = false;
  sourceAllSamplesFlushed();
} /* AudioProcessor::allSamplesFlushed */




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

void AudioProcessor::writeFromBuf(void)
{
  if (buf_cnt == 0)
  {
    return;
  }
  
  int written;
  do
  {
    written = sinkWriteSamples(buf, buf_cnt);
    //cout << "buf_cnt=" << buf_cnt << "  written=" << written << endl;
    buf_cnt -= written;
    if (buf_cnt > 0)
    {
      memmove(buf, buf+written, buf_cnt * sizeof(*buf));
    }
  }
  while ((written > 0) && (buf_cnt > 0));
    
  if (do_flush && (buf_cnt == 0))
  {
    sinkFlushSamples();
  }
  
  if (buf_full && (buf_cnt < static_cast<int>(sizeof(buf) / sizeof(*buf))))
  {
    //cout << "Resume output!\n";
    buf_full = false;
    sourceResumeOutput();
  }
} /* AudioProcessor::writeFromBuf */





/*
 * This file has not been truncated
 */

