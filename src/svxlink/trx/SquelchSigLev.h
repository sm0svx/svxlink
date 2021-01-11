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
#include <sstream>
#include <cmath>


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
      /// The name of this class when used by the object factory
    static constexpr const char* OBJNAME = "SIGLEV";

    /**
     * @brief 	Default constuctor
     */
    SquelchSigLev(void)
      : sig_lev_det(0), open_thresh(0), close_thresh(0) {}

    /**
     * @brief 	Destructor
     */
    ~SquelchSigLev(void) {}

    /**
     * @brief 	Initialize the squelch detector
     * @param 	cfg A previsously initialized config object
     * @param 	name The name of the config section
     * @return	Returns \em true on success or else \em false
     */
    bool initialize(Async::Config& cfg, const std::string& name)
    {
      if (!Squelch::initialize(cfg, name))
      {
      	return false;
      }

      if (cfg.getValue(name, "SIGLEV_OPEN_THRESH", open_thresh))
      {
        std::cerr << "*** WARNING: Config variable SIGLEV_OPEN_THRESH has "
                     "been renamed to SQL_SIGLEV_OPEN_THRESH." << std::endl;
      }
      else if (!cfg.getValue(name, "SQL_SIGLEV_OPEN_THRESH", open_thresh))
      {
        std::cerr << "*** ERROR: Config variable " << name
                  << "/SQL_SIGLEV_OPEN_THRESH not set\n";
        return false;
      }

      if (cfg.getValue(name, "SIGLEV_CLOSE_THRESH", close_thresh))
      {
        std::cerr << "*** WARNING: Config variable SIGLEV_CLOSE_THRESH has "
                     "been renamed to SQL_SIGLEV_CLOSE_THRESH." << std::endl;
      }
      else if (!cfg.getValue(name, "SQL_SIGLEV_CLOSE_THRESH", close_thresh))
      {
	std::cerr << "*** ERROR: Config variable " << name
	      	  << "/SIGLEV_CLOSE_THRESH not set\n";
	return false;
      }

      std::string rx_name(name);
      if (cfg.getValue(name, "SIGLEV_RX_NAME", rx_name))
      {
        std::cerr << "*** WARNING: Config variable SIGLEV_RX_NAME has "
                     "been renamed to SQL_SIGLEV_RX_NAME." << std::endl;
      }
      cfg.getValue(name, "SQL_SIGLEV_RX_NAME", rx_name);

      sig_lev_det = createSigLevDet(cfg, rx_name);

      return (sig_lev_det != 0);
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
      float siglev = sig_lev_det->lastSiglev();
      bool opened = !signalDetected() && (siglev >= open_thresh);
      bool closed = signalDetected() && (siglev < close_thresh);
      if (opened || closed)
      {
        std::ostringstream ss;
        ss << static_cast<int>(std::roundf(siglev));
        setSignalDetected(opened, ss.str());
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

