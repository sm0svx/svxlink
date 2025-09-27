/**
@file	 AsyncAudioSource.cpp
@brief   This file contains the base class for an audio source
@author  Tobias Blomberg / SM0SVX
@date	 2005-04-17

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2003-2025 Tobias Blomberg / SM0SVX

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

#include "AsyncAudioSource.h"
#include "AsyncAudioSink.h"



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
    unregisterSinkInternal(true);
  }
  
  clearHandler();
  
} /* AudioSource::~AudioSource */


bool AudioSource::registerSink(AudioSink *sink, bool managed)
{
  return registerSinkInternal(sink, managed, true);
} /* AudioSource::registerSink */


void AudioSource::unregisterSink(void)
{
  unregisterSinkInternal(false);  
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
  assert(len > 0);

  is_flushing = false;
  
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
    is_flushing = true;
    m_sink->flushSamples();
  }
  else
  {
    handleAllSamplesFlushed();
  }
} /* AudioSource::sinkFlushSamples */


bool AudioSource::setHandler(AudioSource *handler)
{
  clearHandler();
  
  if (handler == 0)
  {
    return true;
  }
  
  if (m_sink != 0)
  {
    if (!handler->registerSinkInternal(m_sink, false, false))
    {
      return false;
    }
  }
  
  m_handler = handler;
  
  return true;
    
} /* AudioSource::setHandler */


void AudioSource::clearHandler(void)
{
  if (m_handler == 0)
  {
    return;
  }
  
  if (m_sink != 0)
  {
    m_handler->unregisterSink();
  }
  
  m_handler = 0;
} /* AudioSource::clearHandler */




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
bool AudioSource::registerSinkInternal(AudioSink *sink, bool managed, bool reg)
{
  assert(sink != 0);
  
  if (m_sink != 0)
  {
    return m_sink == sink;
  }
  
  m_sink = sink;
  m_auto_unreg_source = reg;
  if (reg)
  {
    if (!m_sink->registerSource(this))
    {
      m_sink = 0;
      return false;
    }
  }
    
  if (m_handler != 0)
  {
    if (!m_handler->registerSinkInternal(sink, false, false))
    {
      if (reg)
      {
      	m_sink->unregisterSource();
      }
      m_sink = 0;
      return false;
    }
  }
  
  m_sink_managed = managed;
  
  return true;
  
} /* AudioSource::registerSinkInternal */


void AudioSource::unregisterSinkInternal(bool is_being_destroyed)
{
  if (m_sink == 0)
  {
    return;
  }
  
  AudioSink *sink = m_sink;
  m_sink = 0;
  
  if (m_auto_unreg_source)
  {
    sink->unregisterSource();
  }
  
  m_sink_managed = false;
  
  if (m_handler != 0)
  {
    m_handler->unregisterSink();
  }
  
  if (!is_being_destroyed)
  {
    if (is_flushing)
    {
      handleAllSamplesFlushed();
    }
    else
    {
      resumeOutput();
    }
  }
    
} /* AudioSource::unregisterSinkInternal */






/*
 * This file has not been truncated
 */

