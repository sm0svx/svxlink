/**
@file	 SquelchSigLev.h
@brief   A signal level squelch
@author  Tobias Blomberg / SM0SVX
@date	 2008-04-10

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2004-2010  Tobias Blomberg / SM0SVX

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


#ifndef SQUELCH_SIG_LEV_INCLUDED
#define SQUELCH_SIG_LEV_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <iostream>
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

#include "Squelch.h"
#include "SigLevDet.h"


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
@brief	A signal level based squelch detector
@author Tobias Blomberg / SM0SVX
@date   2008-04-10

This squelch detector use a signal level detector to determine if the squelch
is open or not. The actual signal level detector is implemented outside this
class. This class only implements the squelch logic associated with a signal
level squelch.
*/
class SquelchSigLev : public Squelch
{
  public:
    /**
     * @brief 	Default constuctor
     */
    SquelchSigLev(SigLevDet *det)
      : sig_lev_det(det), open_thresh(0), close_thresh(0) {}

    /**
     * @brief 	Destructor
     */
    ~SquelchSigLev(void) {}

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
      if (!cfg.getValue(rx_name, "SIGLEV_OPEN_THRESH", value))
      {
	std::cerr << "*** ERROR: Config variable " << rx_name
	      	  << "/SIGLEV_OPEN_THRESH not set\n";
	return false;
      }
      open_thresh = atoi(value.c_str());

      if (!cfg.getValue(rx_name, "SIGLEV_CLOSE_THRESH", value))
      {
	std::cerr << "*** ERROR: Config variable " << rx_name
	      	  << "/SIGLEV_CLOSE_THRESH not set\n";
	return false;
      }
      close_thresh = atoi(value.c_str());

      return true;
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
      if (signalDetected())
      {
      	setSignalDetected(sig_lev_det->lastSiglev() >= close_thresh);
      }
      else
      {
      	setSignalDetected(sig_lev_det->lastSiglev() >= open_thresh);
      }

      return count;
    }

  private:
    SigLevDet *sig_lev_det;
    int       open_thresh;
    int       close_thresh;

    SquelchSigLev(const SquelchSigLev&);
    SquelchSigLev& operator=(const SquelchSigLev&);

};  /* class SquelchSigLev */


//} /* namespace */

#endif /* SQUELCH_SIG_LEV_INCLUDED */



/*
 * This file has not been truncated
 */

