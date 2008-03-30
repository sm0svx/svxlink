/**
@file	 AudioSwitchMatrix.h
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

#ifndef AUDIO_SWITCH_MATRIX_INCLUDED
#define AUDIO_SWITCH_MATRIX_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <map>
#include <string>


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



/****************************************************************************
 *
 * Forward declarations
 *
 ****************************************************************************/

namespace Async
{
  class AudioSource;
  class AudioSink;
  class AudioSelector;
  class AudioSplitter;
  class AudioPassthrough;
};


/****************************************************************************
 *
 * Namespace
 *
 ****************************************************************************/

//namespace MyNameSpace
//{


/****************************************************************************
 *
 * Forward declarations of classes inside of the declared namespace
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Defines & typedefs
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Exported Global Variables
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Class definitions
 *
 ****************************************************************************/

/**
@brief	A switch matrix for audio pipe objects
@author Tobias Blomberg
@date   2005-04-17

This is an audio switch matrix for audio pipe objects. Audio can be routed
from any of the connected sources to any of the connected sinks. Multiple
sources can be routed to multiple sinks. If two different sources send audio
to the same sink at the same time, one of the sources are chosen. The samples
from the other source are thrown away.
*/
class AudioSwitchMatrix
{
  public:
    /**
     * @brief 	Default constuctor
     */
    AudioSwitchMatrix(void) {}
  
    /**
     * @brief 	Destructor
     */
    ~AudioSwitchMatrix(void) {}
  
    /**
     * @brief 	Add an audio source to the switch matrix
     * @param 	source_name The name of the source to register
     * @param 	source The pointer to the source to register
     */
    void addSource(const std::string& source_name, Async::AudioSource *source);
  
    /**
     * @brief 	Remove an audio source from the switch matrix
     * @param 	source_name The name of the source to remove
     */
    void removeSource(const std::string& source_name);

    /**
     * @brief   Check if the given source is added or not
     * @param 	source_name The name of the source
     * @return  Return \em true if the source is added or \em false if it's not
     */
    bool sourceIsAdded(const std::string& source_name);
  
    /**
     * @brief 	Add an audio sink to the switch matrix
     * @param 	source_name The name of the source to register
     * @param 	sink The pointer to the sink to register
     */
    void addSink(const std::string& sink_name, Async::AudioSink *sink);
  
    /**
     * @brief 	Remove an audio sink from the switch matrix
     * @param 	sink_name The name of the sink to remove
     */
    void removeSink(const std::string& sink_name);

    /**
     * @brief   Check if the given sink is added or not
     * @param 	sink_name The name of the sink
     * @return  Return \em true if the sink is added or \em false if it's not
     */
    bool sinkIsAdded(const std::string& sink_name);
  
    /**
     * @brief 	Connect a registered source to one of the registered sinks
     * @param 	source_name The name of the source to connect
     * @param 	sink_name The name of the sink to connect
     */
    void connect(const std::string& source_name, const std::string& sink_name);

    /**
     * @brief 	Disconnect a registered source from one of the registered sinks
     * @param 	source_name The name of the source to disconnect
     * @param 	sink_name The name of the sink to disconnect
     */
    void disconnect(const std::string& source_name,
      	    const std::string& sink_name);

    /**
     * @brief 	Disconnect all sinks from the given source
     * @param 	source_name The name of the source to disconnect
     */
    void disconnectSource(const std::string& source_name);

    /**
     * @brief 	Disconnect all sources from the given sink
     * @param 	sink_name The name of the sink to disconnect
     */
    void disconnectSink(const std::string& sink_name);

    /**
     * @brief 	Check if the given source and sink is connected
     * @param 	source_name The name of the source
     * @param 	sink_name The name of the sink
     */
    bool isConnected(const std::string& source_name,
      	    const std::string& sink_name);

    
  protected:
    
  private:
    typedef struct
    {
      Async::AudioSource      *source;
      Async::AudioSplitter    *splitter;
    } SourceInfo;
    typedef struct
    {
      Async::AudioSink       	      	      	      	*sink;
      Async::AudioSelector   	      	      	      	*selector;
      std::map<std::string, Async::AudioPassthrough *>	connectors;
    } SinkInfo;
    typedef std::map<std::string, SourceInfo> SourceMap;
    typedef std::map<std::string, SinkInfo>   SinkMap;
    
    SourceMap sources;
    SinkMap   sinks;
    
};  /* class AudioSwitchMatrix */


//} /* namespace */

#endif /* AUDIO_SWITCH_MATRIX_INCLUDED */



/*
 * This file has not been truncated
 */

