/**
@file	 SquelchCtcss.h
@brief   A CTCSS squelch detector
@author  Tobias Blomberg / SM0SVX
@date	 2005-08-02

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2004-2010 Tobias Blomberg / SM0SVX

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
@brief	A CTCSS squelch detector
@author Tobias Blomberg / SM0SVX
@date   2005-08-02

This squelch detector use a tone detector to detect the presence of a CTCSS
squelch tone. The actual tone detector is implemented outside of this class.
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
     * @brief 	Initialize the squelch detector
     * @param 	cfg A previsously initialized config object
     * @param 	rx_name The name of the RX (config section name)
     * @return	Returns \em true on success or else \em false
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

      float ctcss_thresh = 10;
      if (cfg.getValue(rx_name, "CTCSS_THRESH", value))
      {
	ctcss_thresh = atof(value.c_str());
      }

      det = new ToneDetector(ctcss_fq, 8.0f);
      det->setSnrThresh(ctcss_thresh);
      det->activated.connect(slot(*this, &SquelchCtcss::setSignalDetected));

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
    int processSamples(const float *samples, int count)
    {
      return det->writeSamples(samples, count);
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

