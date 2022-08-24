/**
@file	 SquelchEvDev.h
@brief   A squelch detector that read squelch state from /dev/input/eventX.
@author  Tobias Blomberg / SM0SVX
@date	 2011-02-22

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2004-2011  Tobias Blomberg / SM0SVX

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

#ifndef SQUELCH_EVDEV_INCLUDED
#define SQUELCH_EVDEV_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <iostream>
#include <string>
#include <vector>


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


/****************************************************************************
 *
 * Forward declarations
 *
 ****************************************************************************/

namespace Async
{
  class FdWatch;
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
@brief	An event input device squelch detector
@author Tobias Blomberg / SM0SVX
@date   2011-02-22

This squelch detector read the squelch indicator signal from a event input
device, that is an /dev/input/eventX device.
*/
class SquelchEvDev : public Squelch
{
  public:
      /// The name of this class when used by the object factory
    static constexpr const char* OBJNAME = "EVDEV";

    /**
     * @brief 	Default constuctor
     */
    SquelchEvDev(void);

    /**
     * @brief 	Destructor
     */
    ~SquelchEvDev(void);

    /**
     * @brief 	Initialize the squelch detector
     * @param 	cfg A previsously initialized config object
     * @param 	rx_name The name of the RX (config section name)
     * @return	Returns \em true on success or else \em false
     */
    bool initialize(Async::Config& cfg, const std::string& rx_name);

  protected:


  private:
    std::vector<int>	open_ev;
    std::vector<int>	close_ev;
    int			fd;
    Async::FdWatch	*watch;

    SquelchEvDev(const SquelchEvDev&);
    SquelchEvDev& operator=(const SquelchEvDev&);
    void readEvDevData(Async::FdWatch *w);

};  /* class SquelchEvDev */


//} /* namespace */

#endif /* SQUELCH_EVDEV_INCLUDED */



/*
 * This file has not been truncated
 */

