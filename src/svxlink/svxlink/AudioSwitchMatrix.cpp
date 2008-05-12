/**
@file	 AudioSwitchMatrix.cpp
@brief   A switch matrix for audio pipe objects
@author  Tobias Blomberg / SM0SVX
@date	 2005-04-17

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2004-2008 Tobias Blomberg / SM0SVX

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
#include <cassert>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncAudioSource.h>
#include <AsyncAudioSink.h>
#include <AsyncAudioSplitter.h>
#include <AsyncAudioSelector.h>
#include <AsyncAudioPassthrough.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "AudioSwitchMatrix.h"



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


void AudioSwitchMatrix::addSource(const string& source_name,
      	AudioSource *source)
{
  assert(sources.count(source_name) == 0);

  AudioSplitter *splitter = new AudioSplitter;
  source->registerSink(splitter);

  sources[source_name].source = source;
  sources[source_name].splitter = splitter;
  
  SinkMap::iterator it;
  for (it=sinks.begin(); it!=sinks.end(); ++it)
  {
    AudioPassthrough *connector = new AudioPassthrough;
    splitter->addSink(connector);
    
    AudioSelector *sel = (*it).second.selector;
    sel->addSource(connector);
    
    (*it).second.connectors[source_name] = connector;
  }
} /* AudioSwitchMatrix::addSource */


void AudioSwitchMatrix::removeSource(const string& source_name)
{
  assert(sources.count(source_name) == 1);
  
  AudioSplitter *splitter = sources[source_name].splitter;
  
  SinkMap::iterator it;
  for (it=sinks.begin(); it!=sinks.end(); ++it)
  {
    assert((*it).second.connectors.count(source_name) == 1);
    AudioPassthrough *connector = (*it).second.connectors[source_name];
    (*it).second.selector->removeSource(connector);
    (*it).second.connectors.erase(source_name);
    splitter->removeSink(connector);
    delete connector;
  }

  delete splitter;
  sources.erase(source_name);

} /* AudioSwitchMatrix::removeSource */


bool AudioSwitchMatrix::sourceIsAdded(const std::string& source_name)
{
  return sources.count(source_name) > 0;
} /* AudioSwitchMatrix::sourceIsAdded */


void AudioSwitchMatrix::addSink(const string& sink_name, AudioSink *sink)
{
  assert(sinks.count(sink_name) == 0);
  
  AudioSelector *selector = new AudioSelector;
  selector->registerSink(sink);
  
  sinks[sink_name].sink = sink;
  sinks[sink_name].selector = selector;
  
  SourceMap::iterator it;
  for (it=sources.begin(); it!=sources.end(); ++it)
  {
    AudioPassthrough *connector = new AudioPassthrough;
    (*it).second.splitter->addSink(connector);
    selector->addSource(connector);
    sinks[sink_name].connectors[(*it).first] = connector;
  }
} /* AudioSwitchMatrix::addSink */


void AudioSwitchMatrix::removeSink(const string& sink_name)
{
  assert(sinks.count(sink_name) == 1);

  AudioSelector *selector = sinks[sink_name].selector;

  map<string, AudioPassthrough *>::iterator it;
  map<string, AudioPassthrough *> &cons = sinks[sink_name].connectors;
  for (it=cons.begin(); it!=cons.end(); ++it)
  {
    const string& source_name((*it).first);
    AudioPassthrough *connector = (*it).second;
    selector->removeSource(connector);
    assert(sources.count(source_name) == 1);
    sources[source_name].splitter->removeSink(connector);
    delete connector;
  }
  
  delete selector;
  sinks.erase(sink_name);
  
} /* AudioSwitchMatrix::removeSink */


bool AudioSwitchMatrix::sinkIsAdded(const std::string& sink_name)
{
  return sinks.count(sink_name) > 0;
} /* AudioSwitchMatrix::sinkIsAdded */


void AudioSwitchMatrix::connect(const string& source_name,
      	const string& sink_name)
{
  assert(sources.count(source_name) == 1);
  assert(sinks.count(sink_name) == 1);
  assert(sinks[sink_name].connectors.count(source_name) == 1);
  
  AudioPassthrough *connector = sinks[sink_name].connectors[source_name];
  AudioSelector *selector = sinks[sink_name].selector;
  selector->enableAutoSelect(connector, 0);
} /* AudioSwitchMatrix::connect */


void AudioSwitchMatrix::disconnect(const string& source_name,
      	const string& sink_name)
{
  assert(sources.count(source_name) == 1);
  assert(sinks.count(sink_name) == 1);
  assert(sinks[sink_name].connectors.count(source_name) == 1);
  
  AudioPassthrough *connector = sinks[sink_name].connectors[source_name];
  AudioSelector *selector = sinks[sink_name].selector;
  selector->disableAutoSelect(connector);
  
} /* AudioSwitchMatrix::disconnect */


void AudioSwitchMatrix::disconnectSource(const string& source_name)
{
  assert(sources.count(source_name) == 1);

  SinkMap::iterator it;
  for (it=sinks.begin(); it!=sinks.end(); ++it)
  {
    assert((*it).second.connectors.count(source_name) == 1);
    AudioPassthrough *connector = (*it).second.connectors[source_name];
    (*it).second.selector->disableAutoSelect(connector);
  }

} /* AudioSwitchMatrix::disconnectSource */


void AudioSwitchMatrix::disconnectSink(const string& sink_name)
{
  assert(sinks.count(sink_name) == 1);

  map<string, AudioPassthrough *>::iterator it;
  map<string, AudioPassthrough *> &cons = sinks[sink_name].connectors;
  for (it=cons.begin(); it!=cons.end(); ++it)
  {
    AudioPassthrough *connector = (*it).second;
    sinks[sink_name].selector->disableAutoSelect(connector);
  }

} /* AudioSwitchMatrix::disconnectSink */


bool AudioSwitchMatrix::isConnected(const string& source_name,
      	const string& sink_name)
{
  assert(sources.count(source_name) == 1);
  assert(sinks.count(sink_name) == 1);
  assert(sinks[sink_name].connectors.count(source_name) == 1);

  AudioPassthrough *connector = sinks[sink_name].connectors[source_name];
  AudioSelector *selector = sinks[sink_name].selector;
  return selector->autoSelectEnabled(connector);  
  
} /* AudioSwitchMatrix::isConnected */



/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/



/*
 * This file has not been truncated
 */

