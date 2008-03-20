/**
@file	 SigCAudioSink.h
@brief   Contains an adapter class to connect to an AudioSource using SigC
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


#ifndef SIGC_AUDIO_SINK_INCLUDED
#define SIGC_AUDIO_SINK_INCLUDED


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

#include <AsyncAudioSink.h>


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
@brief	An adapter class to connect to an AudioSource class using SigC
@author Tobias Blomberg
@date   2005-04-17

This is an adapter class that can be used to interact with an AudioSource
class using SigC signals and slots.
*/
class SigCAudioSink : public AudioSink, public SigC::Object
{
  public:
    /**
     * @brief 	Default constuctor
     */
    SigCAudioSink(void) {}
  
    /**
     * @brief 	Destructor
     */
    ~SigCAudioSink(void) {}
  
    /**
     * @brief 	A_brief_member_function_description
     * @param 	param1 Description_of_param1
     * @return	Return_value_of_this_member_function
     */
    virtual int writeSamples(const float *samples, int len)
    {
      return sigWriteSamples(const_cast<float *>(samples), len);
    }
    
    virtual void flushSamples(void)
    {
      sigFlushSamples();
    }
    
    void writeBufferFull(bool is_full)
    {
      if (!is_full)
      {
      	sourceResumeOutput();
      }
    }
    
    void allSamplesFlushed(void)
    {
      sourceAllSamplesFlushed();
    }
    
    SigC::Signal2<int, float *, int>  sigWriteSamples;
    SigC::Signal0<void>       	      sigFlushSamples;
    
    
  protected:
    
  private:
    
};  /* class SigCAudioSink */


} /* namespace */

#endif /* SIGC_AUDIO_SINK_INCLUDED */



/*
 * This file has not been truncated
 */

