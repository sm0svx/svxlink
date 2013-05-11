/**
@file	 AfskDemodulator.h
@brief   A_brief_description_for_this_file
@author  Tobias Blomberg / SM0SVX
@date	 2013-05-09

A_detailed_description_for_this_file

\verbatim
<A brief description of the program or library this file belongs to>
Copyright (C) 2003-2013 Tobias Blomberg / SM0SVX

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

/** @example AfskDemodulator_demo.cpp
An example of how to use the AfskDemodulator class
*/


#ifndef AFSK_DEMODULATOR_INCLUDED
#define AFSK_DEMODULATOR_INCLUDED


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

#include <AsyncAudioSink.h>
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
@date   2013-05-09

A_detailed_class_description

\include AfskDemodulator_demo.cpp
*/
class AfskDemodulator : public Async::AudioSink, public Async::AudioSource
{
  public:
    /**
     * @brief 	Constuctor
     */
    AfskDemodulator(unsigned f0, unsigned f1, unsigned baudrate,
                   unsigned sample_rate=INTERNAL_SAMPLE_RATE);
  
    /**
     * @brief 	Destructor
     */
    ~AfskDemodulator(void);
  
    /**
     * @brief 	A_brief_member_function_description
     * @param 	param1 Description_of_param1
     * @return	Return_value_of_this_member_function
     */
    
  protected:
    
  private:
    const unsigned f0;
    const unsigned f1;
    const unsigned baudrate;

    AfskDemodulator(const AfskDemodulator&);
    AfskDemodulator& operator=(const AfskDemodulator&);
    
};  /* class AfskDemodulator */


//} /* namespace */

#endif /* AFSK_DEMODULATOR_INCLUDED */



/*
 * This file has not been truncated
 */

