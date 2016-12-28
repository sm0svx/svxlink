/**
@file	 AsyncAudioClipper.h
@brief   Contains an audio pipe class to clip audio to a given maximum level
@author  Tobias Blomberg / SM0SVX
@date	 2005-08-01

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2004-2008  Tobias Blomberg / SM0SVX

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

#ifndef ASYNC_AUDIO_CLIPPER_INCLUDED
#define ASYNC_AUDIO_CLIPPER_INCLUDED


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

#include <AsyncAudioProcessor.h>



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
@brief	An audio pipe class to clip audio to a given maximum level
@author Tobias Blomberg / SM0SVX
@date   2005-08-01

This is an audio pipe class that is used to clip an audio stream to a
given maximum level.
*/
class AudioClipper : public AudioProcessor
{
  public:
    /**
     * @brief 	Default constuctor
     * @param 	clip_level  The level to clip at (1.0 is default)
     */
    explicit AudioClipper(float clip_level=1.0) : clip_level(clip_level) {}
  
    /**
     * @brief 	Destructor
     */
    ~AudioClipper(void) {}
  
    /**
     * @brief 	Set the clip level
     * @param 	level The level to set
     */
    void setClipLevel(float level) { clip_level = level; }

    
  protected:
    virtual void processSamples(float *dest, const float *src, int count)
    {
      for (int i=0; i<count; ++i)
      {
      	if (src[i] > clip_level)
	{
	  dest[i] = clip_level;
	}
	else if (src[i] < -clip_level)
	{
	  dest[i] = -clip_level;
	}
	else
	{
	  dest[i] = src[i];
	}
      }
    }
    
    
  private:
    float clip_level;
    
    AudioClipper(const AudioClipper&);
    AudioClipper& operator=(const AudioClipper&);
    
};  /* class AudioClipper */


} /* namespace */

#endif /* ASYNC_AUDIO_CLIPPER_INCLUDED */



/*
 * This file has not been truncated
 */

