/**
@file	 AudioClipper.h
@brief   A_brief_description_for_this_file
@author  Tobias Blomberg / SM0SVX
@date	 2005-08-

A_detailed_description_for_this_file

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

/** @example AudioClipper_demo.cpp
An example of how to use the AudioClipper class
*/


#ifndef AUDIO_CLIPPER_INCLUDED
#define AUDIO_CLIPPER_INCLUDED


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

#include "AudioProcessor.h"


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
@brief	A_brief_class_description
@author Tobias Blomberg / SM0SVX
@date   2005-08-

A_detailed_class_description

\include AudioClipper_demo.cpp
*/
class AudioClipper : public AudioProcessor
{
  public:
    /**
     * @brief 	Default constuctor
     */
    AudioClipper(void) : clip_level(1) {}
  
    /**
     * @brief 	Destructor
     */
    ~AudioClipper(void) {}
  
    /**
     * @brief 	A_brief_member_function_description
     * @param 	param1 Description_of_param1
     * @return	Return_value_of_this_member_function
     */
    void setClipLevel(float level) { clip_level = level; }
    
  protected:
    void processSamples(float *dest, const float *src, int count)
    {
      for (int i=0; i<count; ++i)
      {
      	if (src[i] > clip_level)
	{
	  dest[i] = clip_level;
	  //printf("Clipping!\n");
	}
	else if (src[i] < -clip_level)
	{
	  dest[i] = -clip_level;
	  //printf("Clipping!\n");
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


//} /* namespace */

#endif /* AUDIO_CLIPPER_INCLUDED */



/*
 * This file has not been truncated
 */

