/**
@file	 AudioSwitchMatrix.cpp
@brief   A_brief_description_for_this_file
@author  Tobias Blomberg / SM0SVX
@date	 2004-04-17

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

#include "AudioSwitchMatrix.h"
#include "AudioSource.h"
#include "AudioSink.h"



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


void AudioSwitchMatrix::addSource(const string& source_name,
      	AudioSource *source)
{
  assert(sources.count(source_name) == 0);
  sources[source_name] = source;
} /* AudioSwitchMatrix::addSource */


void AudioSwitchMatrix::removeSource(const string& source_name)
{
  assert(sources.count(source_name) == 1);
  sources.erase(source_name);
} /* AudioSwitchMatrix::removeSource */


void AudioSwitchMatrix::addSink(const string& sink_name, AudioSink *sink)
{
  assert(sinks.count(sink_name) == 0);
  sinks[sink_name] = sink;
} /* AudioSwitchMatrix::addSink */


void AudioSwitchMatrix::removeSink(const string& sink_name)
{
  assert(sinks.count(sink_name) == 1);
  sinks.erase(sink_name);
} /* AudioSwitchMatrix::removeSink */


void AudioSwitchMatrix::connect(const string& source_name,
      	const string& sink_name)
{
  assert(sources.count(source_name) == 1);
  assert(sinks.count(sink_name) == 1);
  AudioSource *source = sources[source_name];
  if (source->isRegistered()) return;
  AudioSink *sink = sinks[sink_name];
  assert(!sink->isRegistered());
  assert(sink->registerSource(source));
} /* AudioSwitchMatrix::connect */


void AudioSwitchMatrix::disconnect(const string& source_name,
      	const string& sink_name)
{
  assert(sources.count(source_name) == 1);
  assert(sinks.count(sink_name) == 1);
  AudioSource *source = sources[source_name];
  if (!source->isRegistered()) return;
  AudioSink *sink = sinks[sink_name];
  assert(sink->isRegistered());
  sink->flushSamples();
  sink->unregisterSource();
} /* AudioSwitchMatrix::disconnect */



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

