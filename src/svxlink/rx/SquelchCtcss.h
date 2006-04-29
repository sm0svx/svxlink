/**
@file	 SquelchCtcss.h
@brief   A_brief_description_for_this_file
@author  Tobias Blomberg / SM0SVX
@date	 2005-08-02

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

/** @example SquelchCtcss_demo.cpp
An example of how to use the SquelchCtcss class
*/


#ifndef SQUELCH_CTCSS_INCLUDED
#define SQUELCH_CTCSS_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <string>
#include <iostream>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncConfig.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "ToneDetector.h"
#include "Squelch.h"


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
@date   2005-08-02

A_detailed_class_description

\include SquelchCtcss_demo.cpp
*/
class SquelchCtcss : public Squelch
{
  public:
    /**
     * @brief 	Default constuctor
     */
    explicit SquelchCtcss(void) {}
  
    /**
     * @brief 	Destructor
     */
    ~SquelchCtcss(void)
    {
      delete det;
    }
  
    /**
     * @brief 	Initialize the CTCSS squelch detector
     * @param 	cfg The configuration object to use
     * @param 	rx_name The name of the receiver (config section)
     * @return	Return_value_of_this_member_function
     */
    bool initialize(Async::Config& cfg, const std::string& rx_name)
    {
      if (!Squelch::initialize(cfg, rx_name))
      {
      	return false;
      }
      
      std::string value;
      float ctcss_fq = 0;
      if (cfg.getValue(rx_name, "CTCSS_FQ", value))
      {
	ctcss_fq = atof(value.c_str());
      }
      if (ctcss_fq <= 0)
      {
	std::cerr << "*** ERROR: Config variable " << rx_name
      	     << "/CTCSS_FQ not set or is set to an illegal value\n";
	return false;
      }
      
      float ctcss_thresh = -5.0;
      if (cfg.getValue(rx_name, "CTCSS_THRESH", value))
      {
	ctcss_thresh = atof(value.c_str());
      }
      
      det = new ToneDetector(ctcss_fq, 1000);
      det->setFilter("LpBu8/270");
      det->setSnrThresh(ctcss_thresh);
      det->activated.connect(slot(this, &SquelchCtcss::setOpen));
      
      return true;
    }
    
    /**
     * @brief 	Reset the squelch detector
     *
     *  Reset the squelch so that the detection process starts from
     *	the beginning again.
     */
    void reset(void)
    {
      det->reset();
      Squelch::reset();
    }

    
  protected:
    /**
     * @brief 	Process the incoming samples in the squelch detector
     * @param 	samples A buffer containing samples
     * @param 	count The number of samples in the buffer
     * @return	Return the number of processed samples
     */
    int processSamples(short *samples, int count)
    {
      return det->processSamples(samples, count);
    }
    
    
  private:
    ToneDetector *det;
    
    SquelchCtcss(const SquelchCtcss&);
    SquelchCtcss& operator=(const SquelchCtcss&);
    
};  /* class SquelchCtcss */


//} /* namespace */

#endif /* SQUELCH_CTCSS_INCLUDED */



/*
 * This file has not been truncated
 */

