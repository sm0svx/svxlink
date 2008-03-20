/**
@file	 SigCAsyncAudioSource.h
@brief   Contains an adapter class to connect to an AudioSink using SigC
@author  Tobias Blomberg / SM0SVX
@date	 2005-04-17

\verbatim
<A brief description of the program or library this file belongs to>
Copyright (C) 2004-2005  Tobias Blomberg / SM0SVX

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


#ifndef SIGC_AUDIO_SOURCE_INCLUDED
#define SIGC_AUDIO_SOURCE_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <sigc++/sigc++.h>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncAudioSource.h>


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



/****************************************************************************
 *
 * Namespace
 *
 ****************************************************************************/

namespace Async
{


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
@brief	An adapter class to connect to an AudioSink class using SigC
@author Tobias Blomberg
@date   2005-04-17

This is an adapter class that can be used to interact with an AudioSink
class using SigC signals and slots.
*/
class SigCAudioSource : public AudioSource, public SigC::Object
{
  public:
    /**
     * @brief 	Default constuctor
     */
    SigCAudioSource(void) {}
  
    /**
     * @brief 	Destructor
     */
    ~SigCAudioSource(void) {}
  
    /**
     * @brief 	A_brief_member_function_description
     * @param 	param1 Description_of_param1
     * @return	Return_value_of_this_member_function
     */
    virtual void resumeOutput(void)
    {
      sigWriteBufferFull(false);
    }
    
    virtual void allSamplesFlushed(void)
    {
      sigAllSamplesFlushed();
    }
    
    int writeSamples(float *samples, int len)
    {
      return sinkWriteSamples(samples, len);
    }
    
    void flushSamples(void)
    {
      sinkFlushSamples();
    }

    SigC::Signal1<void, bool> sigWriteBufferFull;
    SigC::Signal0<void> sigAllSamplesFlushed;
    
    
  protected:
    
  private:
    
};  /* class SigCAudioSource */


} /* namespace */

#endif /* SIGC_AUDIO_SOURCE_INCLUDED */



/*
 * This file has not been truncated
 */

