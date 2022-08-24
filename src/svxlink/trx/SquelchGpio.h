/**
@file	 SquelchGpio.h
@brief   A squelch detector that read squelch state from a GPIO port pin
@author  Jonny Roeker / DG9OAA
@date	 2013-09-09

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
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

#ifndef SQUELCH_GPIO_INCLUDED
#define SQUELCH_GPIO_INCLUDED


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

#include "Squelch.h"


/****************************************************************************
 *
 * Forward declarations
 *
 ****************************************************************************/

namespace Async
{
  class Timer;
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
@brief	A squelch detector that read squelch state from a GPIO port
@author Jonny Roeker / DG9OAA
@date   2013-09-09

This squelch detector read the squelch indicator signal from a GPIO input pin.
A high level (3.3V) will be interpreted as squelch open and a low level (GND)
will be interpreted as squelch close.
*/
class SquelchGpio : public Squelch
{
  public:
      /// The name of this class when used by the object factory
    static constexpr const char* OBJNAME = "GPIO";

    /**
     * @brief 	Default constuctor
     */
    SquelchGpio(void);

    /**
     * @brief 	Destructor
     */
    ~SquelchGpio(void);

    /**
     * @brief 	Initialize the squelch detector
     * @param 	cfg A previsously initialized config object
     * @param 	rx_name The name of the RX (config section name)
     * @return	Returns \em true on success or else \em false
     */
    bool initialize(Async::Config& cfg, const std::string& rx_name);

  protected:

  private:
    int           fd;
    Async::Timer  *timer;
    bool          active_low;
    std::string   gpio_path;

    SquelchGpio(const SquelchGpio&);
    SquelchGpio& operator=(const SquelchGpio&);
    void readGpioValueData(void);

};  /* class SquelchGpio */


//} /* namespace */

#endif /* SQUELCH_GPIO_INCLUDED */



/*
 * This file has not been truncated
 */

