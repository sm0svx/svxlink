/**
@file	 AudioSource.cpp
@brief   This file contains the base class for an audio source
@author  Tobias Blomberg / SM0SVX
@date	 2005-04-17

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

#include "AudioSource.h"
#include "AudioSink.h"



/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/

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


AudioSource::~AudioSource(void)
{
  if (m_sink_managed)
  {
    AudioSink *sink = m_sink;
    m_sink = 0;
    delete sink;
  }
  else
  {
    unregisterSink();
  }
} /* AudioSource::~AudioSource */


bool AudioSource::registerSink(AudioSink *sink, bool managed)
{
  assert(sink != 0);
  
  if (m_sink != 0)
  {
    return m_sink == sink;
  }
  
  m_sink = sink;
  if (!m_sink->registerSource(this))
  {
    m_sink = 0;
    return false;
  }
  
  m_sink_managed = managed;
  
  return true;
  
} /* AudioSource::registerSink */


void AudioSource::unregisterSink(void)
{
  if (m_sink == 0)
  {
    return;
  }
  
  AudioSink *sink = m_sink;
  m_sink = 0;
  sink->unregisterSource();
  m_sink_managed = false;
  
} /* AudioSource::unregisterSink */





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
int AudioSource::sinkWriteSamples(const float *samples, int len)
{
  if (m_sink != 0)
  {
    len = m_sink->writeSamples(samples, len);
  }
  
  return len;
  
} /* AudioSource::sinkWriteSamples */


void AudioSource::sinkFlushSamples(void)
{
  if (m_sink != 0)
  {
    m_sink->flushSamples();
  }
} /* AudioSource::sinkFlushSamples */






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

