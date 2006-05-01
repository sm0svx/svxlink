/**
@file	 AudioFilter.h
@brief   A_brief_description_for_this_file
@author  Tobias Blomberg / SM0SVX
@date	 2006-04-23

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

/** @example AudioFilter_demo.cpp
An example of how to use the AudioFilter class
*/


#ifndef AUDIO_FILTER_INCLUDED
#define AUDIO_FILTER_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

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

extern "C" {
#include <fidlib.h>
};

#include "AudioSource.h"
#include "AudioSink.h"



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
@date   2006-04-23

A_detailed_class_description

\include AudioFilter_demo.cpp
*/
class AudioFilter : public AudioSink, public AudioSource
{
  public:
    /**
     * @brief 	Default constuctor
     */
    AudioFilter(const std::string &filter_spec);
  
    /**
     * @brief 	Destructor
     */
    ~AudioFilter(void);
  
    void setOutputGain(float gain) { output_gain = gain; }
    
    /**
     * @brief 	A_brief_member_function_description
     * @param 	param1 Description_of_param1
     * @return	Return_value_of_this_member_function
     */
    int writeSamples(const float *samples, int len);
    
    void flushSamples(void);

    void resumeOutput(void);
    
    void allSamplesFlushed(void);

    
  protected:
    
  private:
    FidFilter 	*ff;
    FidRun    	*ff_run;
    FidFunc   	*ff_func;
    void      	*ff_buf;
    float     	buf[1024];
    int       	buf_cnt;
    bool      	do_flush;
    bool      	buf_full;
    float     	output_gain;
    
    AudioFilter(const AudioFilter&);
    AudioFilter& operator=(const AudioFilter&);
    void writeFromBuf(void);

};  /* class AudioFilter */


//} /* namespace */

#endif /* AUDIO_FILTER_INCLUDED */



/*
 * This file has not been truncated
 */

