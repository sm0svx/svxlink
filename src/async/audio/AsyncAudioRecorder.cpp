/**
@file	 AudioRecorder.cpp
@brief   Contains a class for recording raw audio to a file
@author  Tobias Blomberg / SM0SVX
@date	 2005-08-29

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2004-2009 Tobias Blomberg / SM0SVX

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

#include "AsyncAudioRecorder.h"



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

AudioRecorder::AudioRecorder(const string& filename)
  : filename(filename), file(NULL)
{
  
} /* AudioRecorder::AudioRecorder */


AudioRecorder::~AudioRecorder(void)
{
  if (file != NULL)
  {
    fclose(file);
  }
} /* AudioRecorder::~AudioRecorder */


bool AudioRecorder::initialize(void)
{
  assert(file == NULL);
  
  file = fopen(filename.c_str(), "w");
  if (file == NULL)
  {
    perror("*** ERROR fopen");
    return false;
  }
  
  return true;
  
} /* AudioRecorder::initialize */


int AudioRecorder::writeSamples(const float *samples, int count)
{
  if (file == NULL)
  {
    return count;
  }
  
  short buf[count];
  for (int i=0; i<count; ++i)
  {
    float sample = samples[i];
    if (sample > 1)
    {
      buf[i] = 32767;
    }
    else if (sample < -1)
    {
      buf[i] = -32767;
    }
    else
    {
      buf[i] = static_cast<short>(32767.0 * sample);
    }
  }
  
  int written = fwrite(buf, sizeof(*buf), count, file);
  if ((written == 0) && ferror(file))
  {
    fclose(file);
    file = NULL;
  }
  
  return written;

} /* AudioRecorder::writeSamples */


void AudioRecorder::flushSamples(void)
{
  sourceAllSamplesFlushed();
} /* AudioRecorder::flushSamples */




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

