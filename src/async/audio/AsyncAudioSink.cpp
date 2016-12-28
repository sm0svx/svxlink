/**
@file	 AsyncAudioSink.cpp
@brief   This file contains the base class for an audio sink
@author  Tobias Blomberg / SM0SVX
@date	 2005-04-17

\verbatim
Async - A library for programming event driven applications
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

#include "AsyncAudioSink.h"
#include "AsyncAudioSource.h"



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


AudioSink::~AudioSink(void)
{
  unregisterSource();
  clearHandler();
} /* AudioSink::~AudioSink */


bool AudioSink::registerSource(AudioSource *source)
{
  return registerSourceInternal(source, true);
} /* AudioSink::registerSource */


void AudioSink::unregisterSource(void)
{
  if (m_source == 0)
  {
    return;
  }
  
  AudioSource *source = m_source;
  m_source = 0;
  
  if (m_auto_unreg_sink)
  {
    source->unregisterSink();
  }
  
  if (m_handler != 0)
  {
    m_handler->unregisterSource();
  }
} /* AudioSink::unregisterSource */



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
void AudioSink::sourceResumeOutput(void)
{
  if (m_source != 0)
  {
    m_source->resumeOutput();
  }
} /* AudioSink::sourceResumeOutput */


void AudioSink::sourceAllSamplesFlushed(void)
{
  if (m_source != 0)
  {
    m_source->handleAllSamplesFlushed();
  }
} /* AudioSink::sourceAllSamplesFlushed */


bool AudioSink::setHandler(AudioSink *handler)
{
  clearHandler();
  
  if (handler == 0)
  {
    return true;
  }
  
  if (m_source != 0)
  {
    if (!handler->registerSourceInternal(m_source, false))
    {
      return false;
    }
  }
  
  m_handler = handler;
  
  return true;
    
} /* AudioSink::setHandler */


void AudioSink::clearHandler(void)
{
  if (m_handler == 0)
  {
    return;
  }
  
  if (m_source != 0)
  {
    m_handler->unregisterSource();
  }
  
  m_handler = 0;
} /* AudioSink::clearHandler */



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
bool AudioSink::registerSourceInternal(AudioSource *source, bool reg_sink)
{
  assert(source != 0);
  
  if (m_source != 0)
  {
    return m_source == source;
  }
  
  m_source = source;
  m_auto_unreg_sink = reg_sink;
  if (reg_sink)
  {
    if (!m_source->registerSink(this))
    {
      m_source = 0;
      return false;
    }
  }
  
  if (m_handler != 0)
  {
    if (!m_handler->registerSourceInternal(source, false))
    {
      if (reg_sink)
      {
      	m_source->unregisterSink();
      }
      m_source = 0;
      return false;
    }
  }
  
  return true;
  
} /* AudioSink::registerSourceInternal */




/*
 * This file has not been truncated
 */

