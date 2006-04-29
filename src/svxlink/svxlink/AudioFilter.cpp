/**
@file	 AudioFilter.cpp
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

#include "AudioFilter.h"



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

AudioFilter::AudioFilter(const string &filter_spec)
  : ff(0), ff_run(0), ff_buf(0), buf_cnt(0), do_flush(false), buf_full(false)
{
  char spec_buf[256];
  strncpy(spec_buf, filter_spec.c_str(), sizeof(spec_buf));
  spec_buf[sizeof(spec_buf) - 1] = 0;
  char *spec = spec_buf;
  char *fferr = fid_parse(SAMPLE_RATE, &spec, &ff);
  if (fferr != 0)
  {
    cerr << "***ERROR: Filter creation error: " << fferr << endl;
    exit(1);
  }
  ff_run = fid_run_new(ff, &ff_func);
  ff_buf = fid_run_newbuf(ff_run);
  
} /* AudioFilter::AudioFilter */


AudioFilter::~AudioFilter(void)
{
  if (ff != 0)
  {
    fid_run_freebuf(ff_buf);
    fid_run_free(ff_run);
    free(ff);
  }  
} /* AudioFilter::~AudioFilter */


int AudioFilter::writeSamples(const short *samples, int len)
{
  //cout << "AudioFilter::writeSamples: len=" << len << endl;
  
  do_flush = false;
  buf_full = false;
  
  writeFromBuf();
  
  int buf_avail = sizeof(buf)/sizeof(*buf) - buf_cnt;
  if (buf_avail < len)
  {
    len = buf_avail;
    buf_full = true;
  }
  
  for (int i=0; i<len; ++i)
  {
    buf[buf_cnt+i] = 4 * static_cast<short int>(ff_func(ff_buf, samples[i]));
  }
  buf_cnt += len;
  
  writeFromBuf();
  
  return len;

} /* AudioFilter::writeSamples */


void AudioFilter::flushSamples(void)
{
  //cout << "AudioFilter::flushSamples" << endl;
  
  do_flush = true;
  //buf_full = false;
  if (buf_cnt == 0)
  {
    sinkFlushSamples();
  }
} /* AudioFilter::flushSamples */


void AudioFilter::resumeOutput(void)
{
  //cout << "AudioFilter::resumeOutput" << endl;
  writeFromBuf();
} /* AudioFilter::resumeOutput */


void AudioFilter::allSamplesFlushed(void)
{
  //cout << "AudioFilter::allSamplesFlushed" << endl;
  do_flush = false;
  sourceAllSamplesFlushed();
} /* AudioFilter::allSamplesFlushed */




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

void AudioFilter::writeFromBuf(void)
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
} /* AudioFilter::writeFromBuf */





/*
 * This file has not been truncated
 */

