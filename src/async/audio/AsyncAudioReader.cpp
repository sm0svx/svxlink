/**
@file	 AsyncAudioReader.h
@brief   An audio pipe component for on-demand reading samples
@author  Tobias Blomberg / SM0SVX
@date	 2008-02-22

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2003-2008 Tobias Blomberg / SM0SVX

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

#include <algorithm>
#include <cstring>
#include <cassert>


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

#include "AsyncAudioReader.h"



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

AudioReader::AudioReader(void)
  : buf(0), buf_size(0), input_stopped(false), samples_in_buf(0)
{
  
} /* AudioReader::AudioReader */


AudioReader::~AudioReader(void)
{
  
} /* AudioReader::~AudioReader */


int AudioReader::readSamples(float *samples, int count)
{
  assert(count > 0);

  buf = samples;
  buf_size = count;
  samples_in_buf = 0;
  if (input_stopped)
  {
    input_stopped = false;
    sourceResumeOutput();
  }
  buf = 0;
  buf_size = 0;
  
  //printf("AudioReader::readSamples: samples_in_buf=%d\n", samples_in_buf);
  
  return samples_in_buf;
  
} /* AudioReader::readSamples */


int AudioReader::writeSamples(const float *samples, int count)
{
  int samples_to_read = 0;
  if (buf != 0)
  {
    samples_to_read = min(count, buf_size - samples_in_buf);
    memcpy(buf + samples_in_buf, samples, samples_to_read * sizeof(*buf));
    samples_in_buf += samples_to_read;
  }
  
  input_stopped = (samples_to_read == 0);
  
  return samples_to_read;
  
} /* AudioReader::writeSamples */


void AudioReader::flushSamples(void)
{
  sourceAllSamplesFlushed();
} /* AudioReader::flushSamples */




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







/*
 * This file has not been truncated
 */

