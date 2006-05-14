/**
@file	 SigLevDet.h
@brief   A_brief_description_for_this_file
@author  Tobias Blomberg / SM0SVX
@date	 2006-05-07

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

/** @example SigLevDet_demo.cpp
An example of how to use the SigLevDet class
*/


#ifndef SIG_LEV_DET_INCLUDED
#define SIG_LEV_DET_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <sigc++/signal_system.h>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AudioSink.h>


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
  class AudioFilter;
  class SigCAudioSink;
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
@brief	A_brief_class_description
@author Tobias Blomberg / SM0SVX
@date   2006-05-07

A_detailed_class_description

\include SigLevDet_demo.cpp
*/
class SigLevDet : public SigC::Object, public Async::AudioSink
{
  public:
    /**
     * @brief 	Default constuctor
     */
    SigLevDet(void);
  
    /**
     * @brief 	Destructor
     */
    ~SigLevDet(void);
  
    /**
     * @brief 	A_brief_member_function_description
     * @param 	param1 Description_of_param1
     * @return	Return_value_of_this_member_function
     */
     double lastSiglev(void) const { return last_siglev; }
     
     void reset(void);
     
    
  protected:
    
  private:
    Async::AudioFilter	  *filter;
    Async::SigCAudioSink  *sigc_sink;
    double    	      	  last_siglev;
    
    SigLevDet(const SigLevDet&);
    SigLevDet& operator=(const SigLevDet&);
    int processSamples(float *samples, int count);
    
};  /* class SigLevDet */


//} /* namespace */

#endif /* SIG_LEV_DET_INCLUDED */



/*
 * This file has not been truncated
 */

